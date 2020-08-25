#ifndef TORRENTSTATUS_H
#define TORRENTSTATUS_H

#include <QHash>
#include <QIcon>

struct StatusProperties
{
    enum struct TorrentStatus
    {
        Downloading,
        Error,
        Finished,
        MissingFiles,
        Moving,
        Paused,
        Queued,
        Stalled,
        Unknown = -1,
    };

    TorrentStatus state;
    QColor color;
    std::function<QIcon()> getIcon;
    QString text;
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

static StatusHash g_statusHash;

#endif // TORRENTSTATUS_H
