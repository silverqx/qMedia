#include "torrentstatus.h"

namespace
{
    // Icons section
    const QIcon &pausedIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/paused.svg"));
        return cached;
    };

    const QIcon &queuedIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/queued.svg"));
        return cached;
    }

    const QIcon &downloadingIcon()
    {
        static const QIcon cached(QStringLiteral(
                                      ":/icons/torrent_statuses/downloading.svg"));
        return cached;
    }

    const QIcon &stalledIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/stalled.svg"));
        return cached;
    }

    const QIcon &finishedIcon()
    {
        static const QIcon cached(QStringLiteral(
                                      ":/icons/torrent_statuses/finished.svg"));
        return cached;
    }

    const QIcon &checkingIcon()
    {
        static const QIcon cached(QStringLiteral(
                                      ":/icons/torrent_statuses/checking.svg"));
        return cached;
    }

    const QIcon &errorIcon()
    {
        static const QIcon cached(QStringLiteral(":/icons/torrent_statuses/error.svg"));
        return cached;
    }

    // Colors section
    // yellow
    const QColor &checkingColor()
    {
        static const QColor cached(214, 197, 64);
        return cached;
    }

    // green
    const QColor &downloadingColor()
    {
        static const QColor cached(111, 172, 61);
        return cached;
    }

    // orange
    const QColor &errorColor()
    {
        static const QColor cached(214, 86, 69);
        return cached;
    }

    // blue
    const QColor &finishedColor()
    {
        static const QColor cached(69, 198, 214);
        return cached;
    }

    // green
    const QColor &forcedDownloadingColor()
    {
        static const QColor cached {downloadingColor()};
        return cached;
    }

    // orange
    const QColor &missingFilesColor()
    {
        static const QColor cached {errorColor()};
        return cached;
    }

    // yellow
    const QColor &movingColor()
    {
        static const QColor cached {checkingColor()};
        return cached;
    }

    // purple
    const QColor &pausedColor()
    {
        static const QColor cached(154, 167, 214);
        return cached;
    }

    // pink
    const QColor &queuedColor()
    {
        static const QColor cached(255, 106, 173);
        return cached;
    }

    // salmon
    const QColor &stalledColor()
    {
        static const QColor cached(255, 128, 128);
        return cached;
    }
} // namespace

std::shared_ptr<StatusHash> StatusHash::m_instance;

std::shared_ptr<StatusHash> StatusHash::instance()
{
    if (m_instance)
        return m_instance;

    m_instance = std::shared_ptr<StatusHash>(new StatusHash());

    return m_instance;
}

void StatusHash::freeInstance()
{
    if (m_instance)
        m_instance.reset();
}

const QHash<QString, StatusProperties> &StatusHash::getStatusHash() const
{
    static const QHash<QString, StatusProperties> cached {
        {QStringLiteral("Checking"),
            {TorrentStatus::Checking, checkingColor, checkingIcon,
                QStringLiteral("Checking Torrent Files")}},

        {QStringLiteral("CheckingResumeData"),
            {TorrentStatus::CheckingResumeData, checkingColor, checkingIcon,
                QStringLiteral("Checking Resume Data")}},

        {QStringLiteral("Downloading"),
            {TorrentStatus::Downloading, downloadingColor, downloadingIcon,
                QStringLiteral("Downloading")}},

        {QStringLiteral("Error"),
            {TorrentStatus::Error, errorColor, errorIcon,
                QStringLiteral("Error")}},

        {QStringLiteral("Finished"),
            {TorrentStatus::Finished, finishedColor, finishedIcon,
                QStringLiteral("Finished")}},

        {QStringLiteral("ForcedDownloading"),
            {TorrentStatus::ForcedDownloading, forcedDownloadingColor,
                downloadingIcon,
                QStringLiteral("Forced Downloading")}},

        {QStringLiteral("MissingFiles"),
            {TorrentStatus::MissingFiles, missingFilesColor, errorIcon,
                QStringLiteral("Missing Files")}},

        {QStringLiteral("Moving"),
            {TorrentStatus::Moving, movingColor, checkingIcon,
                QStringLiteral("Moving")}},

        {QStringLiteral("Paused"),
            {TorrentStatus::Paused, pausedColor, pausedIcon,
                QStringLiteral("Paused")}},

        {QStringLiteral("Queued"),
            {TorrentStatus::Queued, queuedColor, queuedIcon,
                QStringLiteral("Queued")}},

        {QStringLiteral("Stalled"),
            {TorrentStatus::Stalled, stalledColor, stalledIcon,
                QStringLiteral("Stalled")}},

        {QStringLiteral("Unknown"),
            {TorrentStatus::Unknown, errorColor, errorIcon,
                QStringLiteral("Unknown")}},
    };

    return cached;
}

const StatusProperties &StatusHash::operator[](const QString &key) const
{
    // Cached reference to statusHash, wtf 😂
    static const auto &statusHash {getStatusHash()};

    return statusHash.find(key).value();
}
