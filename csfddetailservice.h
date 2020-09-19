#ifndef CSFDDETAILSERVICE_H
#define CSFDDETAILSERVICE_H

#include "abstractmoviedetailservice.h"

class CsfdDetailService final : public AbstractMovieDetailService
{
    Q_OBJECT
    Q_DISABLE_COPY(CsfdDetailService)

public:
    static void initInstance(TorrentSqlTableModel *const model);
    static CsfdDetailService *instance();
    static void freeInstance();

protected:
    MovieDetail searchMovieDetail(const QSqlRecord &torrent) const override;
    MovieDetail obtainMovieDetail(quint64 filmId) const override;
    inline QString getMovieDetailColumnName() const noexcept override
    { return QStringLiteral("csfd_movie_detail"); }

private:
    explicit CsfdDetailService(TorrentSqlTableModel *const model);
    ~CsfdDetailService();

    const QString m_movieScrapperPath;

    static CsfdDetailService *m_instance;
};

#endif // CSFDDETAILSERVICE_H
