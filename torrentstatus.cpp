#include "torrentstatus.h"

namespace
{
    static QIcon getPausedIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/paused.svg"));
        return cached;
    }

    static QIcon getQueuedIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/queued.svg"));
        return cached;
    }

    static QIcon getDownloadingIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/downloading.svg"));
        return cached;
    }

    static QIcon getStalledIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/stalled.svg"));
        return cached;
    }

    static QIcon getFinishedIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/finished.svg"));
        return cached;
    }

    static QIcon getMovingIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/moving.svg"));
        return cached;
    }

    static QIcon getErrorIcon()
    {
        static QIcon cached =
            QIcon(QStringLiteral(":/icons/torrent_statuses/error.svg"));
        return cached;
    }

    // green
    static const QColor colorDownloading(111, 172, 61);
    // orange
    static const QColor colorError(214, 86, 69);
    // blue
    static const QColor colorFinished(69, 198, 214);
    // orange
    static const QColor colorMissingFiles {colorError};
    // yellow
    static const QColor colorMoving(214, 197, 64);
    // purple
    static const QColor colorPaused(154, 167, 214);
    // pink
    static const QColor colorQueued(255, 106, 173);
    // salmon
    static const QColor colorStalled(255, 128, 128);

    // TODO check Q_GLOBAL_STATIC_WITH_ARGS for static maps initialization silverqx
    static const QHash<QString, StatusProperties> l_statusHash
    {
        {QStringLiteral("Downloading"),
            {StatusProperties::TorrentStatus::Downloading,
                colorDownloading, getDownloadingIcon,
                QStringLiteral("Downloading")}},
        {QStringLiteral("Error"),
            {StatusProperties::TorrentStatus::Error,
                colorError, getErrorIcon,
                QStringLiteral("Error")}},
        {QStringLiteral("Finished"),
            {StatusProperties::TorrentStatus::Finished,
                colorFinished, getFinishedIcon,
                QStringLiteral("Finished")}},
        {QStringLiteral("MissingFiles"),
            {StatusProperties::TorrentStatus::MissingFiles,
                colorMissingFiles, getErrorIcon,
                QStringLiteral("Missing Files")}},
        {QStringLiteral("Moving"),
            {StatusProperties::TorrentStatus::Moving,
                colorMoving, getMovingIcon,
                QStringLiteral("Moving")}},
        {QStringLiteral("Paused"),
            {StatusProperties::TorrentStatus::Paused,
                colorPaused, getPausedIcon,
                QStringLiteral("Paused")}},
        {QStringLiteral("Queued"),
            {StatusProperties::TorrentStatus::Queued,
                colorQueued, getQueuedIcon,
                QStringLiteral("Queued")}},
        {QStringLiteral("Stalled"),
            {StatusProperties::TorrentStatus::Stalled,
                colorStalled, getStalledIcon,
                QStringLiteral("Stalled")}},
        {QStringLiteral("Unknown"),
            {StatusProperties::TorrentStatus::Unknown,
                colorError, getErrorIcon,
                QStringLiteral("Unknown")}},
    };
}

const QHash<QString, StatusProperties> &StatusHash::getStatusHash() const
{
    return l_statusHash;
}
