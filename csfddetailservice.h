#pragma once
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
    MovieDetail searchMovieDetail(const QSqlRecord &torrent) const final;
    MovieDetail obtainMovieDetail(quint64 filmId) const final;
    QString getMovieDetailColumnName() const noexcept final;

private:
    explicit CsfdDetailService(TorrentSqlTableModel *const model);
    // WARNING debug changing access of virtual destructor silverqx
    // https://en.cppreference.com/w/cpp/language/virtual#Virtual_destructor
    // A useful guideline is that the destructor of any base class must be public and virtual or protected and non-virtual.
    // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rc-dtor-virtual
    inline ~CsfdDetailService() final = default;

    const QString m_movieScrapperPath;

    static CsfdDetailService *m_instance;
};

#endif // CSFDDETAILSERVICE_H
