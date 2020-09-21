#include "torrentstatus.h"

namespace
{
    // Icons section
    const QIcon &getPausedIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/paused.svg"));
        return cached;
    };

    const QIcon &getQueuedIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/queued.svg"));
        return cached;
    }

    const QIcon &getDownloadingIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/downloading.svg"));
        return cached;
    }

    const QIcon &getStalledIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/stalled.svg"));
        return cached;
    }

    const QIcon &getFinishedIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/finished.svg"));
        return cached;
    }

    const QIcon &getCheckingIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/checking.svg"));
        return cached;
    }

    const QIcon &getErrorIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/error.svg"));
        return cached;
    }

    // Colors section
    // yellow
    const QColor &getCheckingColor()
    {
        static const QColor cached(214, 197, 64);
        return cached;
    }

    // green
    const QColor &getDownloadingColor()
    {
        static const QColor cached(111, 172, 61);
        return cached;
    }

    // orange
    const QColor &getErrorColor()
    {
        static const QColor cached(214, 86, 69);
        return cached;
    }

    // blue
    const QColor &getFinishedColor()
    {
        static const QColor cached(69, 198, 214);
        return cached;
    }

    // green
    const QColor &getForcedDownloadingColor()
    {
        static const QColor cached {getDownloadingColor()};
        return cached;
    }

    // orange
    const QColor &getMissingFilesColor()
    {
        static const QColor cached {getErrorColor()};
        return cached;
    }

    // yellow
    const QColor &getMovingColor()
    {
        static const QColor cached {getCheckingColor()};
        return cached;
    }

    // purple
    const QColor &getPausedColor()
    {
        static const QColor cached(154, 167, 214);
        return cached;
    }

    // pink
    const QColor &getQueuedColor()
    {
        static const QColor cached(255, 106, 173);
        return cached;
    }

    // salmon
    const QColor &getStalledColor()
    {
        static const QColor cached(255, 128, 128);
        return cached;
    }
}

StatusHash *StatusHash::m_instance = nullptr;

StatusHash *StatusHash::instance()
{
    if (!m_instance)
        m_instance = new StatusHash();
    return m_instance;
}

void StatusHash::freeInstance()
{
    delete m_instance;
    m_instance = nullptr;
}

QHash<QString, StatusProperties> &StatusHash::getStatusHash() const
{
    static QHash<QString, StatusProperties> cached {
        {QStringLiteral("Allocating"),
            {TorrentStatus::Allocating, getStalledColor, getStalledIcon,
                QStringLiteral("Allocating Files Storage")}},
        {QStringLiteral("Checking"),
            {TorrentStatus::Checking, getCheckingColor, getCheckingIcon,
                QStringLiteral("Checking Torrent Files")}},
        {QStringLiteral("CheckingResumeData"),
            {TorrentStatus::CheckingResumeData, getCheckingColor, getCheckingIcon,
                QStringLiteral("Checking Resume Data")}},
        {QStringLiteral("Downloading"),
            {TorrentStatus::Downloading, getDownloadingColor, getDownloadingIcon,
                QStringLiteral("Downloading")}},
        {QStringLiteral("Error"),
            {TorrentStatus::Error, getErrorColor, getErrorIcon,
                QStringLiteral("Error")}},
        {QStringLiteral("Finished"),
            {TorrentStatus::Finished, getFinishedColor, getFinishedIcon,
                QStringLiteral("Finished")}},
        {QStringLiteral("ForcedDownloading"),
            {TorrentStatus::ForcedDownloading, getForcedDownloadingColor, getDownloadingIcon,
                QStringLiteral("Forced Downloading")}},
        {QStringLiteral("MissingFiles"),
            {TorrentStatus::MissingFiles, getMissingFilesColor, getErrorIcon,
                QStringLiteral("Missing Files")}},
        {QStringLiteral("Moving"),
            {TorrentStatus::Moving, getMovingColor, getCheckingIcon,
                QStringLiteral("Moving")}},
        {QStringLiteral("Paused"),
            {TorrentStatus::Paused, getPausedColor, getPausedIcon,
                QStringLiteral("Paused")}},
        {QStringLiteral("Queued"),
            {TorrentStatus::Queued, getQueuedColor, getQueuedIcon,
                QStringLiteral("Queued")}},
        {QStringLiteral("Stalled"),
            {TorrentStatus::Stalled, getStalledColor, getStalledIcon,
                QStringLiteral("Stalled")}},
        {QStringLiteral("Unknown"),
            {TorrentStatus::Unknown, getErrorColor, getErrorIcon,
                QStringLiteral("Unknown")}},
    };
    return cached;
}
