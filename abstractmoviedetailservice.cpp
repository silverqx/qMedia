#include "abstractmoviedetailservice.h"

#include <QDebug>
#include <QJsonObject>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "torrentsqltablemodel.h"
#include "types/moviedetail.h"
#include "utils/fs.h"

AbstractMovieDetailService::AbstractMovieDetailService(TorrentSqlTableModel *const model,
                                                       QObject *const parent)
    : QObject(parent)
    , m_model(model)
{}

SearchMovieResult
AbstractMovieDetailService::getSearchMovieDetail(const QSqlRecord &torrent) const
{
    const auto torrentId = torrent.value("id").toULongLong();

    // Try to fetch from DB
    {
        auto movieDetail = selectSearchedMovieDetailByTorrentId(torrentId);
        if (!movieDetail.isEmpty())
            return movieDetail;
    }

    auto movieDetail = searchMovieDetail(torrent);

    // Save movie detail to the cache
    m_searchMovieCache.insert(torrentId, movieDetail);
    // Also save to obtained movie detail cache make sense
    m_movieDetailsCache.insert(
                movieDetail["detail"]["id"].toInt(),
                QJsonDocument(movieDetail["detail"].toObject()));

    // Save to DB
    saveSearchedMovieDetailToDb(torrent, movieDetail);

    return movieDetail;
}

MovieDetailResult AbstractMovieDetailService::getMovieDetail(quint64 filmId) const
{
    // Return from cache
    if (m_movieDetailsCache.contains(filmId)) {
        qDebug() << "Obtained movie detail obtained from cache, film :" << filmId;

        return m_movieDetailsCache.value(filmId);
    }

    auto movieDetail = obtainMovieDetail(filmId);

    // Save movie detail to cache
    m_movieDetailsCache.insert(movieDetail["id"].toInt(), movieDetail);

    return movieDetail;
}

// TODO run operation like this in thread, QtConcurent::run() is ideal silverqx
int AbstractMovieDetailService::updateSearchMovieDetailInDb(
    const QSqlRecord &torrent, const MovieDetail &movieDetail,
    const MovieSearchResults &movieSearchResult, const int movieDetailComboBoxIndex) const
{
    const auto torrentId = torrent.value("id").toULongLong();
    // Prepare movie detail json as QString
    auto movieDetailNew = SearchMovieResult({
        {"detail", movieDetail},
        {"search", movieSearchResult}
    });
    const auto movieDetailDocument = QJsonDocument(movieDetailNew);
    const auto movieDetailString = QString::fromStdString(
                                       movieDetailDocument.toJson(QJsonDocument::Compact)
                                       .toStdString());

    // Save to cache
    m_searchMovieCache.insert(torrentId, movieDetailDocument);

    const auto torrentRow = m_model->getTorrentRowByInfoHash(torrent.value("hash").toString());
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_MOVIE_DETAIL_INDEX),
                     movieDetailComboBoxIndex);
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_CSFD_MOVIE_DETAIL),
                     movieDetailString);

    const auto ok = m_model->submit();
    if (!ok) {
        qDebug("Update of a movie detail for the torrent(ID%llu) failed : %s",
               torrentId, qUtf8Printable(m_model->lastError().text()));
        return 1;
    }

    qDebug() << "Searched movie detail updated in db, torrent :"
             << torrentId;

    return 0;
}

MovieDetailResult
AbstractMovieDetailService::parseMovieDetail(const QByteArray &movieDetailRaw) const
{
    auto movieDetail = QJsonDocument::fromJson(movieDetailRaw);

    if (movieDetail.isEmpty() || movieDetail.isNull() || !movieDetail.isObject())
        return {};

    return movieDetail;
}

SearchMovieResult
AbstractMovieDetailService::parseSearchedMovieDetail(const QByteArray &movieDetailRaw) const
{
    auto movieDetail = QJsonDocument::fromJson(movieDetailRaw);

    if (movieDetail.isEmpty() || movieDetail.isNull() || !movieDetail.isObject() ||
        !movieDetail.object().contains("search") ||
        !movieDetail.object().contains("detail")
    )
        return {};

    return movieDetail;
}

QString AbstractMovieDetailService::getMovieScrapperPath() const
{
#ifdef QMEDIA_DEBUG
    return Utils::Fs::toNativePath("E:/c/qMedia/qMedia/movies_scrapper/src/index.js");
#else
    return Utils::Fs::toNativePath(QCoreApplication::applicationDirPath() +
                                   "/movies_scrapper/src/index.js");
#endif
}

QString AbstractMovieDetailService::prepareSearchQueryString(const QSqlRecord &torrent) const
{
    // TODO tune search query string regexp silverqx
    return torrent.value("name").toString()
            .replace(QChar('.'), QChar(' '))
            .replace(QRegularExpression(" {2,}"), QLatin1String(" "));
}

SearchMovieResult
AbstractMovieDetailService::selectSearchedMovieDetailByTorrentId(const quint64 id) const
{
    // Return from the cache
    if (m_searchMovieCache.contains(id)) {
        qDebug() << "Searched movie detail obtained from cache, torrent :" << id;
        return m_searchMovieCache.value(id);
    }

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QStringLiteral("SELECT CAST(%1 AS CHAR) as detail FROM torrents WHERE id = ?")
                  .arg(getMovieDetailColumnName()));
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug("Select a movie detail from the database for the torrent(ID%llu) failed : %s",
               id, qUtf8Printable(query.lastError().text()));
        return {};
    }

    // Check return query size
    if (const auto querySize = query.size(); querySize != 1) {
        // TODO decide how to handle this type of situations, asserts vs exceptions, ... silverqx
        qWarning().noquote()
                << QStringLiteral(
                       "Select a movie detail from the databasefor the torrent(ID%1) "
                       "doesn't have size 1, current size is : %2")
                   .arg(id, querySize);
        return {};
    }

    // Get movie detail
    query.first();
    const auto movieDetailRaw = query.record().value("detail");
    if (!movieDetailRaw.isValid() || movieDetailRaw.isNull())
        return {};

    // In db is saved searched movie detail
    auto movieDetail = parseSearchedMovieDetail(movieDetailRaw.toByteArray());

    // Save movie detail to cache
    m_searchMovieCache.insert(id, movieDetail);
    // Also save to obtained movie detail cache make sense
    m_movieDetailsCache.insert(
                movieDetail["detail"]["id"].toInt(),
                MovieDetailResult(movieDetail["detail"].toObject()));

    qDebug() << "Searched movie detail obtained from db, torrent :" << id;

    return movieDetail;
}

void AbstractMovieDetailService::saveSearchedMovieDetailToDb(
        const QSqlRecord &torrent, const SearchMovieResult &movieDetail) const
{
    const auto movieDetailString = QString::fromStdString(
                                       movieDetail.toJson(QJsonDocument::Compact)
                                       .toStdString());

    const auto torrentRow =
            m_model->getTorrentRowByInfoHash(torrent.value("hash").toString());

    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_MOVIE_DETAIL_INDEX),
                     0);
    m_model->setData(m_model->index(torrentRow,
                                    TorrentSqlTableModel::TR_CSFD_MOVIE_DETAIL),
                     movieDetailString);

    const auto torrentId = torrent.value("id").toULongLong();

    if (!m_model->submit()) {
        qDebug("Update of a movie detail for the torrent(ID%llu) failed : %s",
               torrentId, qUtf8Printable(m_model->lastError().text()));
        return;
    }

    qDebug() << "Searched movie detail saved to db, torrent :" << torrentId;
}
