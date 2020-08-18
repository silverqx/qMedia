#include "csfddetailservice.h"

#include <QProcess>

CsfdDetailService *CsfdDetailService::m_instance = nullptr;

CsfdDetailService::CsfdDetailService()
    : m_movieScrapperPath(getMovieScrapperPath())
{}

CsfdDetailService::~CsfdDetailService()
{}

CsfdDetailService *CsfdDetailService::instance()
{
    if (!m_instance)
        m_instance = new CsfdDetailService();

    return m_instance;
}

void CsfdDetailService::freeInstance()
{
    if (!m_instance)
        return;

    delete m_instance;
    m_instance = nullptr;
}

MovieDetail CsfdDetailService::obtainMovieDetail(const QSqlRecord &torrent)
{
    // TODO handle errors in std::cerr silverqx
    QProcess movieScrapper;
    QStringList arguments;
    arguments << m_movieScrapperPath
              << QStringLiteral("csfd")
              << prepareSearchQueryString(torrent);
    movieScrapper.start(QStringLiteral("node.exe"), arguments);

    // TODO check all paths, when obtaining movie detail was not successful silverqx
    if (!movieScrapper.waitForStarted())
        return {};
    if (!movieScrapper.waitForFinished())
        return {};

    QByteArray movieDetailRaw = movieScrapper.readAll();

    const auto movieDetail = parseMovieDetail(movieDetailRaw);

    // Save movie detail to cache
    m_movieDetailsCache.insert(torrent.value("id").toULongLong(), movieDetail);

    return movieDetail;
}
