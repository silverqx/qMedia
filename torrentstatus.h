#ifndef TORRENTSTATUS_H
#define TORRENTSTATUS_H

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
    std::function<const QColor &()> getColor;
    std::function<const QIcon &()> getIcon;
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
    Q_DISABLE_COPY(StatusHash);

public:
    static StatusHash *instance();
    static void freeInstance();

    inline const StatusProperties &operator[](const QString &key) const
    {
        // Cached reference to statusHash, wtf 😂
        static auto &statusHash {getStatusHash()};
        return statusHash[key];
    }

private:
    StatusHash() = default;
    ~StatusHash() = default;

    QHash<QString, StatusProperties> &getStatusHash() const;

    static StatusHash *m_instance;
};

#endif // TORRENTSTATUS_H
