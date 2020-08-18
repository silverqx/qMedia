#include "abstractmoviedetailservice.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrl>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

#include "utils/fs.h"

AbstractMovieDetailService::AbstractMovieDetailService(QObject *parent)
    : QObject(parent)
{}

AbstractMovieDetailService::~AbstractMovieDetailService()
{}

MovieDetail AbstractMovieDetailService::getMovieDetail(const QSqlRecord &torrent)
{
    // Try to fetch from DB
    {
        const auto movieDetail = selectMovieDetailByTorrentId(torrent.value("id").toULongLong());
        if (!movieDetail.isEmpty())
            return movieDetail;
    }

    const auto movieDetail = obtainMovieDetail(torrent);

    // Save to DB
    insertMovieDetailToDb(torrent, movieDetail);

    return movieDetail;
}

MovieDetail AbstractMovieDetailService::parseMovieDetail(const QByteArray &movieDetailRaw) const
{
    QJsonDocument movieDetail = QJsonDocument::fromJson(movieDetailRaw);
    if (movieDetail.isEmpty() || movieDetail.isNull()
        || !movieDetail.isObject())
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

MovieDetail AbstractMovieDetailService::selectMovieDetailByTorrentId(const quint64 id)
{
    // Return from cache
    if (m_movieDetailsCache.contains(id))
        return m_movieDetailsCache.value(id);

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM torrents WHERE id = ?")
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

    const auto movieDetail = QJsonDocument::fromVariant(movieDetailRaw);
    if (movieDetail.isNull() || movieDetail.isEmpty()
        || !movieDetail.isObject())
        return {};

    // Save movie detail to cache
    m_movieDetailsCache.insert(id, movieDetail);

    return movieDetail;
}

void AbstractMovieDetailService::insertMovieDetailToDb(const QSqlRecord &torrent,
                                                       const MovieDetail &movieDetail) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE torrents SET %1 = ? WHERE id = ?")
                  .arg(getMovieDetailColumnName()));

    const auto movieDetailString = QString::fromStdString(
                                       movieDetail.toJson(QJsonDocument::Compact).toStdString());
    query.addBindValue(movieDetailString);
    const auto torrentId = torrent.value("id").toULongLong();
    query.addBindValue(torrentId);

    const bool ok = query.exec();
    if (!ok) {
        qDebug() << QStringLiteral("Update of a movie detail for the torrent(ID%1) failed :")
                    .arg(torrentId)
                 << query.lastError().text();
        return;
    }
}
