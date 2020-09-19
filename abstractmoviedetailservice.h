#ifndef ABSTRACTMOVIEDETAILSERVICE_H
#define ABSTRACTMOVIEDETAILSERVICE_H

#include <QJsonDocument>
#include <QtSql/QSqlRecord>

class QSqlTableModel;
class TorrentSqlTableModel;

// TODO have hard feeling that this have to be class, normal value object silverqx
typedef QJsonDocument MovieDetail;

class AbstractMovieDetailService : public QObject
{
    Q_OBJECT

public:
    explicit AbstractMovieDetailService(TorrentSqlTableModel *const model,
                                        QObject *parent = nullptr);
    virtual ~AbstractMovieDetailService();

    MovieDetail getMovieDetail(const QSqlRecord &torrent) const;
    MovieDetail getMovieDetail(quint64 filmId) const;
    int updateObtainedMovieDetailInDb(const QSqlRecord &torrent,
                                       const QJsonObject &movieDetail,
                                       const QJsonArray &movieSearchResult,
                                       int movieDetailComboBoxIndex) const;

protected:
    /*! Search a movie detail and obtain search results and also movie detail at once,
        like descendat wants, json api, call other program, ... */
    virtual MovieDetail searchMovieDetail(const QSqlRecord &torrent) const = 0;
    /*! Obtain only a movie detail like descendat wants, json api, call other program, ... */
    virtual MovieDetail obtainMovieDetail(quint64 filmId) const = 0;
    /*! Column name for a movie detail in torrents table, currently csfd_movie_detail and
        imdb_movie_detail. */
    virtual QString getMovieDetailColumnName() const = 0;

    /*! String used to search a movie detail on the internet. */
    QString prepareSearchQueryString(const QSqlRecord &torrent) const;
    MovieDetail parseMovieDetail(const QByteArray &movieDetailRaw) const;
    MovieDetail parseSearchedMovieDetail(const QByteArray &movieDetailRaw) const;
    QString getMovieScrapperPath() const;

    // TODO persist both movie caches, to prevent requesting apis and for better performance silverqx
    /*! Movie details obtained during initial populate, keyed by torrent id. */
    mutable QHash<quint64, MovieDetail> m_movieDetailsCache;
    /*! Movie details obtained through movie detail combobox, keyed by film id. */
    mutable QHash<quint64, MovieDetail> m_obtainedMovieDetailsCache;

private:
    /*! Fetch movie detail from the db. */
    MovieDetail selectMovieDetailByTorrentId(quint64 id) const;
    void saveSearchedMovieDetailToDb(const QSqlRecord &torrent,
                                     const MovieDetail &movieDetail) const;

    TorrentSqlTableModel *const m_model;
};

#endif // ABSTRACTMOVIEDETAILSERVICE_H
