#ifndef TORRENTSTATUS_H
#define TORRENTSTATUS_H

#include <QHash>
#include <QIcon>

enum struct TorrentStatus
{
    // Starts from 1 because of MySQL enums starts from 1
    Checking = 1,
    CheckingResumeData,
    Downloading,
    Error,
    Finished,
    ForcedDownloading,
    MissingFiles,
    Moving,
    Paused,
    Queued,
    Stalled,
    Unknown,
};

/*! Torrent status properties. */
struct StatusProperties
{
    TorrentStatus status {TorrentStatus::Unknown};
    std::function<const QColor &()> color;
    std::function<const QIcon &()> icon;
    QString title;

    inline bool isPaused() const noexcept;
    inline bool isForced() const noexcept;
    inline bool isMoving() const noexcept;
    inline bool isFinished() const noexcept;
    inline bool isMissingFiles() const noexcept;
    inline bool isError() const noexcept;
    inline bool isDownloading() const noexcept;
    inline bool isChecking() const noexcept;
};

/*! Maps a TorrentStatus string representation to StatusProperties. */
class StatusHash final
{
    Q_DISABLE_COPY(StatusHash)

public:
    inline ~StatusHash() = default;

    static std::shared_ptr<StatusHash> instance();
    static void freeInstance();

    const StatusProperties &operator[](const QString &key) const;

private:
    StatusHash() = default;

    const QHash<QString, StatusProperties> &getStatusHash() const;

    static std::shared_ptr<StatusHash> m_instance;
};

/* StatusProperties */

bool StatusProperties::isPaused() const noexcept
{
    return status == TorrentStatus::Paused;
}

bool StatusProperties::isForced() const noexcept
{
    return status == TorrentStatus::ForcedDownloading;
}

bool StatusProperties::isMoving() const noexcept
{
    return status == TorrentStatus::Moving;
}

bool StatusProperties::isFinished() const noexcept
{
    return status == TorrentStatus::Finished;
}

bool StatusProperties::isMissingFiles() const noexcept
{
    return status == TorrentStatus::MissingFiles;
}

bool StatusProperties::isError() const noexcept
{
    return status == TorrentStatus::Error;
}

bool StatusProperties::isDownloading() const noexcept
{
    return status == TorrentStatus::Downloading ||
           status == TorrentStatus::ForcedDownloading ||
           status == TorrentStatus::Stalled ||
           status == TorrentStatus::Queued;
}

bool StatusProperties::isChecking() const noexcept
{
    return status == TorrentStatus::Checking ||
           status == TorrentStatus::CheckingResumeData;
}

#endif // TORRENTSTATUS_H
