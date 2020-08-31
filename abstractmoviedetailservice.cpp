#include "abstractmoviedetailservice.h"

#include <QDebug>
#include <QJsonObject>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "torrentsqltablemodel.h"
#include "utils/fs.h"

AbstractMovieDetailService::AbstractMovieDetailService(TorrentSqlTableModel *const model,
                                                       QObject *parent)
    : QObject(parent)
    , m_model(model)
{}

AbstractMovieDetailService::~AbstractMovieDetailService()
{}

MovieDetail AbstractMovieDetailService::getMovieDetail(const QSqlRecord &torrent) const
{
    const auto torrentId = torrent.value("id").toULongLong();

    // Try to fetch from DB
    {
        const auto movieDetail = selectMovieDetailByTorrentId(torrentId);
        if (!movieDetail.isEmpty())
            return movieDetail;
    }

    const auto movieDetail = searchMovieDetail(torrent);

    // Save movie detail to cache
    m_movieDetailsCache.insert(torrentId, movieDetail);
    // Also save to obtained movie detail cache make sense
    m_obtainedMovieDetailsCache.insert(
                movieDetail["detail"]["id"].toInt(),
                QJsonDocument(movieDetail["detail"].toObject()));

    // Save to DB
    saveSearchedMovieDetailToDb(torrent, movieDetail);

    return movieDetail;
}

MovieDetail AbstractMovieDetailService::getMovieDetail(quint64 filmId) const
{
    // Return from cache
    if (m_obtainedMovieDetailsCache.contains(filmId)) {
        qDebug() << "Obtained movie detail obtained from cache, film :" << filmId;
        return m_obtainedMovieDetailsCache.value(filmId);
    }

    const auto movieDetail = obtainMovieDetail(filmId);

    // Save movie detail to cache
    m_obtainedMovieDetailsCache.insert(movieDetail["id"].toInt(),
                                       movieDetail);

    return movieDetail;
}

// TODO run operation like this in thread, QtConcurent::run() is ideal silverqx
int AbstractMovieDetailService::updateObtainedMovieDetailInDb(
    const QSqlRecord &torrent,
    const QJsonObject &movieDetail,
    const QJsonArray &movieSearchResult,
    const int movieDetailComboBoxIndex
) const
{
    const auto torrentId = torrent.value("id").toULongLong();
    // Prepare movie detail json as QString
    auto movieDetailNew = QJsonObject({
        {"detail", movieDetail},
        {"search", movieSearchResult}
    });
    const auto movieDetailDocument = QJsonDocument(movieDetailNew);
    const auto movieDetailString = QString::fromStdString(
                                       movieDetailDocument.toJson(QJsonDocument::Compact)
                                       .toStdString());

    // Save to cache
    m_movieDetailsCache.insert(torrentId, movieDetailDocument);

    const auto torrentRow = m_model->getTorrentRowByInfoHash(torrent.value("hash").toString());
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_MOVIE_DETAIL_INDEX),
                     movieDetailComboBoxIndex);
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_CSFD_MOVIE_DETAIL),
                     movieDetailString);

    const auto ok = m_model->submit();
    if (!ok) {
        qDebug() << QStringLiteral("Update of a movie detail for the torrent(ID%1) failed :")
                    .arg(torrentId)
                 << m_model->lastError().text();
        return 1;
    }

    qDebug() << "Searched movie detail updated in db, torrent :" << torrentId;

    return 0;
}

MovieDetail AbstractMovieDetailService::parseMovieDetail(const QByteArray &movieDetailRaw) const
{
    QJsonDocument movieDetail = QJsonDocument::fromJson(movieDetailRaw);
    if (movieDetail.isEmpty() || movieDetail.isNull()
        || !movieDetail.isObject())
        return {};

    return movieDetail;
}

MovieDetail AbstractMovieDetailService::parseSearchedMovieDetail(const QByteArray &movieDetailRaw) const
{
    QJsonDocument movieDetail = QJsonDocument::fromJson(movieDetailRaw);
    if (movieDetail.isEmpty() || movieDetail.isNull()
        || !movieDetail.isObject() || !movieDetail.object().contains("search")
        || !movieDetail.object().contains("detail"))
        return {};

    return movieDetail;
}

QString AbstractMovieDetailService::getMovieScrapperPath() const
{
#ifdef QT_DEBUG
    // TODO use every where toNativePath() silverqx
    return Utils::Fs::toNativePath("E:/c/qMedia/qMedia/movies_scrapper/src/index.js");
#else
    return Utils::Fs::toNativePath(QCoreApplication::applicationDirPath() +
                                   "/movies_scrapper/src/index.js");
#endif
}

QString AbstractMovieDetailService::prepareSearchQueryString(const QSqlRecord &torrent) const
{
    // TODO tune search query string regexp silverqx
    const auto queryString = torrent.value("name").toString()
                          .replace(("."), " ")
                          .replace(QRegularExpression(" {2,}"), " ");
    return queryString;
}

MovieDetail AbstractMovieDetailService::selectMovieDetailByTorrentId(const quint64 id) const
{
    // Return from cache
    if (m_movieDetailsCache.contains(id)) {
        qDebug() << "Searched movie detail obtained from cache, torrent :" << id;
        return m_movieDetailsCache.value(id);
    }

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QStringLiteral("SELECT CAST(%1 AS CHAR) FROM torrents WHERE id = ?")
                  .arg(getMovieDetailColumnName()));
    query.addBindValue(id);

    const bool ok = query.exec();
    if (!ok) {
        qDebug() << QStringLiteral("Select of a movie detail for the torrent(ID%1) failed :").arg(id)
                 << query.lastError().text();
        return {};
    }

    const auto querySize = query.size();
    if (querySize != 1) {
        // TODO decide how to handle this type of situations, asserts vs exceptions, ... silverqx
        qWarning() << QStringLiteral("Select of a movie detail for the torrent(ID%1) doesn't have "
                                     "size of 1, current size is :")
                      .arg(id, querySize);
        return {};
    }

    // Get movie detail
    query.first();
    const auto movieDetailRaw = query.record().value(0);
    if (!movieDetailRaw.isValid() || movieDetailRaw.isNull())
        return {};

    // In db is saved searched movie detail
    const auto movieDetail = parseSearchedMovieDetail(movieDetailRaw.toByteArray());

    // Save movie detail to cache
    m_movieDetailsCache.insert(id, movieDetail);
    // Also save to obtained movie detail cache make sense
    m_obtainedMovieDetailsCache.insert(
                movieDetail["detail"]["id"].toInt(),
                QJsonDocument(movieDetail["detail"].toObject()));

    qDebug() << "Searched movie detail obtained from db, torrent :" << id;

    return movieDetail;
}

void AbstractMovieDetailService::saveSearchedMovieDetailToDb(
        const QSqlRecord &torrent, const MovieDetail &movieDetail) const
{
    const auto movieDetailString = QString::fromStdString(
                                       movieDetail.toJson(QJsonDocument::Compact)
                                       .toStdString());

    const auto torrentRow = m_model->getTorrentRowByInfoHash(torrent.value("hash").toString());
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_MOVIE_DETAIL_INDEX),
                     0);
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_CSFD_MOVIE_DETAIL),
                     movieDetailString);

    const auto torrentId = torrent.value("id").toULongLong();

    const bool ok = m_model->submit();
    if (!ok) {
        qDebug() << QStringLiteral("Update of a movie detail for the torrent(ID%1) failed :")
                    .arg(torrentId)
                 << m_model->lastError().text();
        return;
    }

    qDebug() << "Searched movie detail saved to db, torrent :" << torrentId;
}
