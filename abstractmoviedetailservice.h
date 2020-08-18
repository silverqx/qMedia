#ifndef ABSTRACTMOVIEDETAILSERVICE_H
#define ABSTRACTMOVIEDETAILSERVICE_H

#include <QJsonDocument>
#include <QtSql/QSqlRecord>

// TODO have hard feeling that this have to be class, normal value object silverqx
typedef QJsonDocument MovieDetail;

class AbstractMovieDetailService : public QObject
{
    Q_OBJECT

public:
    explicit AbstractMovieDetailService(QObject *parent = nullptr);
    virtual ~AbstractMovieDetailService();

    MovieDetail getMovieDetail(const QSqlRecord &torrent);

protected:
    /*! Obtain a movie detail like descendat wants, json api, call other program, ... */
    virtual MovieDetail obtainMovieDetail(const QSqlRecord &torrent) = 0;
    /*! Column name for a movie detail in torrents table, currently csfd_movie_detail and imdb_movie_detail. */
    virtual QString getMovieDetailColumnName() const = 0;

    /*! String used to search a movie detail on the internet. */
    QString prepareSearchQueryString(const QSqlRecord &torrent) const;
    MovieDetail parseMovieDetail(const QByteArray &movieDetailRaw) const;
    QString getMovieScrapperPath() const;

    QHash<quint64, MovieDetail> m_movieDetailsCache;

private:
    /*! Fetch movie detail from the db. */
    MovieDetail selectMovieDetailByTorrentId(quint64 id);
    void insertMovieDetailToDb(const QSqlRecord &torrent, const MovieDetail &movieDetail) const;
};

#endif // ABSTRACTMOVIEDETAILSERVICE_H
