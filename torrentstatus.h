#ifndef TORRENTSTATUS_H
#define TORRENTSTATUS_H

#include <QHash>
#include <QIcon>

// Starts from 1 because of MySQL enums starts from 1
enum struct TorrentStatus
{
    Allocating = 1,
    Checking,
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

struct StatusProperties
{
    TorrentStatus status;
    QColor color;
    std::function<QIcon()> getIcon;
    QString text;

    inline bool isPaused() const       { return status == TorrentStatus::Paused; }
    inline bool isForced() const       { return status == TorrentStatus::ForcedDownloading; }
    inline bool isAllocating() const   { return status == TorrentStatus::Allocating; }
    inline bool isMoving() const       { return status == TorrentStatus::Moving; }
    inline bool isFinished() const     { return status == TorrentStatus::Finished; }
    inline bool isMissingFiles() const { return status == TorrentStatus::MissingFiles; }
    inline bool isError() const        { return status == TorrentStatus::Error; }

    inline bool isDownloading() const
    {
        return (status == TorrentStatus::Downloading)
                || (status == TorrentStatus::ForcedDownloading)
                || (status == TorrentStatus::Stalled)
                || (status == TorrentStatus::Queued);
    }

    inline bool isChecking() const
    {
        return (status == TorrentStatus::Checking)
                || (status == TorrentStatus::CheckingResumeData);
    }
};

class StatusHash final
{
public:
    inline const StatusProperties &operator[](const QString &key) const
    {
        // Cached statusHash
        static auto statusHash {getStatusHash()};
        return statusHash[key];
    }

private:
    const QHash<QString, StatusProperties> &getStatusHash() const;
};

Q_DECL_UNUSED
static StatusHash g_statusHash;

#endif // TORRENTSTATUS_H
