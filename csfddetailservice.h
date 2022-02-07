#pragma once
#ifndef CSFDDETAILSERVICE_H
#define CSFDDETAILSERVICE_H

#include "abstractmoviedetailservice.h"

class CsfdDetailService final : public AbstractMovieDetailService
{
    Q_OBJECT
    Q_DISABLE_COPY(CsfdDetailService)

public:
    explicit CsfdDetailService(TorrentSqlTableModel *model);
    /*! Virtual destructor. */
    inline ~CsfdDetailService() final = default;

protected:
    MovieDetail searchMovieDetail(const QSqlRecord &torrent) const final;
    MovieDetail obtainMovieDetail(quint64 filmId) const final;
    QString getMovieDetailColumnName() const noexcept final;

private:
    const QString m_movieScrapperPath;
};

#endif // CSFDDETAILSERVICE_H
