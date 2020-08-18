#ifndef CSFDDETAILSERVICE_H
#define CSFDDETAILSERVICE_H

#include "abstractmoviedetailservice.h"

class CsfdDetailService final : public AbstractMovieDetailService
{
    Q_OBJECT
    Q_DISABLE_COPY(CsfdDetailService)

public:
    static CsfdDetailService *instance();
    static void freeInstance();

protected:
    MovieDetail obtainMovieDetail(const QSqlRecord &torrent) override;
    inline QString getMovieDetailColumnName() const override { return QStringLiteral("csfd_movie_detail"); }

private:
    explicit CsfdDetailService();
    ~CsfdDetailService();

    QString m_movieScrapperPath;

    static CsfdDetailService *m_instance;
};

#endif // CSFDDETAILSERVICE_H
