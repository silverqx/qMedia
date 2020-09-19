#include "csfddetailservice.h"

#include <QProcess>

CsfdDetailService *CsfdDetailService::m_instance = nullptr;

CsfdDetailService::CsfdDetailService(TorrentSqlTableModel *const model)
    : AbstractMovieDetailService(model)
    , m_movieScrapperPath(getMovieScrapperPath())
{}

CsfdDetailService::~CsfdDetailService()
{}

void CsfdDetailService::initInstance(TorrentSqlTableModel *const model)
{
    if (!m_instance)
        m_instance = new CsfdDetailService(model);
}

CsfdDetailService *CsfdDetailService::instance()
{
    return m_instance;
}

void CsfdDetailService::freeInstance()
{
    if (!m_instance)
        return;

    delete m_instance;
    m_instance = nullptr;
}

MovieDetail CsfdDetailService::searchMovieDetail(const QSqlRecord &torrent) const
{
    // TODO handle errors in std::cerr silverqx
    QProcess movieScrapper;
    QStringList arguments;
    arguments << m_movieScrapperPath
              << QStringLiteral("csfd-search")
              << prepareSearchQueryString(torrent);
    movieScrapper.start(QStringLiteral("node.exe"), arguments);

    // TODO check all paths, when obtaining movie detail was not successful silverqx
    if (!movieScrapper.waitForStarted())
        return {};
    if (!movieScrapper.waitForFinished())
        return {};

    const auto movieDetailRaw = movieScrapper.readAll();

    qDebug() << "Searched movie detail obtained from čsfd.cz";

    return parseSearchedMovieDetail(movieDetailRaw);
}

MovieDetail CsfdDetailService::obtainMovieDetail(const quint64 filmId) const
{
    // TODO handle errors in std::cerr silverqx
    QProcess movieScrapper;
    QStringList arguments;
    arguments << m_movieScrapperPath
              << QStringLiteral("csfd-get")
              << QString::number(filmId);
    movieScrapper.start(QStringLiteral("node.exe"), arguments);

    // TODO check all paths, when obtaining movie detail was not successful silverqx
    if (!movieScrapper.waitForStarted())
        return {};
    if (!movieScrapper.waitForFinished())
        return {};

    const auto movieDetailRaw = movieScrapper.readAll();

    qDebug() << "Obtained movie detail obtained from čsfd.cz";

    return parseMovieDetail(movieDetailRaw);
}
