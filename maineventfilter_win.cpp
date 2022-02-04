#include "maineventfilter_win.h"

#include <QDebug>

#include "common.h"
#include "commonglobal.h"
#include "mainwindow.h"

MainEventFilter::MainEventFilter(MainWindow *const mainWindow)
    : m_mainWindow(mainWindow)
{}

bool MainEventFilter::nativeEventFilter(
        const QByteArray &eventType, void *message, long */*unused*/) // NOLINT(google-runtime-int)
{
    if (eventType != QByteArrayLiteral("windows_generic_MSG"))
        return false;

    const MSG *msg = static_cast<const MSG *>(message);

    switch (msg->message) {
    case ::MSG_QBITTORRENT_UP: {
        qDebug() << "IPC qBittorrent : qBittorrent started";
        HWND qBittorrentHwndNew = reinterpret_cast<HWND>(msg->wParam); // NOLINT(performance-no-int-to-ptr)
        if (m_mainWindow->qBittorrentHwnd() != qBittorrentHwndNew)
            emit m_mainWindow->qBittorrentHwndChanged(qBittorrentHwndNew);
        // WARNING Qt docs: We recommend to only emit them from the class that defines the signal and its subclasses silverqx
        emit m_mainWindow->qBittorrentUp();
        return true;
    }
    case ::MSG_QBITTORRENT_DOWN:
        qDebug() << "IPC qBittorrent : qBittorrent closed";
        if (m_mainWindow->qBittorrentHwnd() != nullptr)
            emit m_mainWindow->qBittorrentHwndChanged(nullptr);
        emit m_mainWindow->qBittorrentDown();
        return true;
    case ::MSG_QBT_TORRENT_REMOVED:
        qDebug() << "IPC qBittorrent : Torrent removed";
        emit m_mainWindow->torrentsAddedOrRemoved();
        return true;
    case ::MSG_QBT_TORRENTS_ADDED:
        qDebug() << "IPC qBittorrent : Torrents added";
        emit m_mainWindow->torrentsAddedOrRemoved();
        return true;
    }

    // WM_COPYDATA section
    if (msg->message != WM_COPYDATA)
        return false;

    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const COPYDATASTRUCT copyData = *reinterpret_cast<PCOPYDATASTRUCT>(msg->lParam);

    switch (static_cast<int>(msg->wParam)) {
    case ::MSG_QBT_TORRENTS_CHANGED:
    case ::MSG_QBT_TORRENT_MOVED:
        // Put together QVector of torrent info hashes
        const auto dataSize = static_cast<int>(copyData.cbData);
        const auto hashesCount = dataSize / ::INFOHASH_SIZE;
        const auto *const begin = static_cast<const char *>(copyData.lpData);

        QVector<QString> torrentInfoHashes;
        torrentInfoHashes.reserve(hashesCount);

        // One info hash has 40 bytes
        for (int i = 0; i < hashesCount ; ++i)
            torrentInfoHashes << QString::fromLatin1(
                                     begin + (i * ::INFOHASH_SIZE), ::INFOHASH_SIZE);

#ifdef LOG_CHANGED_TORRENTS
        qDebug() << "IPC qBittorrent : Changed torrents copyData size :"
                 << dataSize;
        qDebug() << "IPC qBittorrent : Changed torrents info hashes :"
                 << torrentInfoHashes;
#endif

        emit m_mainWindow->torrentsChanged(torrentInfoHashes);
        return true;
    }

    return false;
}
