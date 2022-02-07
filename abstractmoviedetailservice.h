#pragma once
#ifndef ABSTRACTMOVIEDETAILSERVICE_H
#define ABSTRACTMOVIEDETAILSERVICE_H

#include <QJsonDocument>
#include <QtSql/QSqlRecord>

class QSqlTableModel;
class TorrentSqlTableModel;

// TODO have hard feeling that this have to be class, normal value object silverqx
using MovieDetailResult = QJsonDocument;
using SearchMovieResult = QJsonDocument;

class AbstractMovieDetailService : public QObject
{
    Q_OBJECT

public:
    /*! Constructor. */
    explicit AbstractMovieDetailService(TorrentSqlTableModel *model,
                                        QObject *parent = nullptr);
    /*! Virtual destructor. */
    inline ~AbstractMovieDetailService() override = default;

    SearchMovieResult getSearchMovieDetail(const QSqlRecord &torrent) const;
    MovieDetailResult getMovieDetail(quint64 filmId) const;

    int updateSearchMovieDetailInDb(
            const QSqlRecord &torrent, const QJsonObject &movieDetail,
            const QJsonArray &movieSearchResult, int movieDetailComboBoxIndex) const;

protected:
    /*! Search a movie detail, it obtains search results and also movie detail at once,
        calls node-csfd-api program, ... */
    virtual SearchMovieResult searchMovieDetail(const QSqlRecord &torrent) const = 0;
    /*! Obtain a movie detail, calls node-csfd-api program, ... */
    virtual MovieDetailResult obtainMovieDetail(quint64 filmId) const = 0;
    /*! Column name for a movie detail in torrents table, currently csfd_movie_detail and
        imdb_movie_detail. */
    virtual QString getMovieDetailColumnName() const noexcept = 0;

    /*! String used to search a movie detail on the internet. */
    QString prepareSearchQueryString(const QSqlRecord &torrent) const;
    MovieDetailResult parseMovieDetail(const QByteArray &movieDetailRaw) const;
    SearchMovieResult parseSearchedMovieDetail(const QByteArray &movieDetailRaw) const;
    QString getMovieScrapperPath() const;

    // TODO persist both movie caches, to prevent requesting apis and for better performance silverqx
    /*! Search movie request result cache (csfd-search), obtained during initial populate,
        keyed by torrent id, saved in the csfd_movie_detail column in db. */
    mutable QHash<quint64, SearchMovieResult> m_searchMovieCache;
    /*! Movie details cache (csfd-get), obtained through movie detail combobox and also
        stores movies from search movie requests (csfd-search), keyed by film id. */
    mutable QHash<quint64, MovieDetailResult> m_movieDetailsCache;

private:
    /*! Fetch searched movie detail from the database. */
    SearchMovieResult selectSearchedMovieDetailByTorrentId(quint64 id) const;
    void saveSearchedMovieDetailToDb(const QSqlRecord &torrent,
                                     const SearchMovieResult &movieDetail) const;

    TorrentSqlTableModel *const m_model;
};

#endif // ABSTRACTMOVIEDETAILSERVICE_H
