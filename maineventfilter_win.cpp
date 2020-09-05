#include "maineventfilter_win.h"

#include <QDebug>

#include "common.h"
#include "config.h"
#include "mainwindow.h"

MainEventFilter::MainEventFilter(MainWindow *const mainWindow)
    : QAbstractNativeEventFilter()
    , m_mainWindow(mainWindow)
{}

bool MainEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);

    if (eventType != QByteArrayLiteral("windows_generic_MSG"))
        return false;

    const MSG *msg = static_cast<const MSG *>(message);

    switch (msg->message) {
    case MSG_QBITTORRENT_UP:
        qDebug() << "IPC qBittorrent : qBittorrent started";
        emit m_mainWindow->qBittorrentHwndChanged(reinterpret_cast<HWND>(msg->wParam));
        return true;
    case MSG_QBITTORRENT_DOWN:
        qDebug() << "IPC qBittorrent : qBittorrent closed";
        // TODO keep track of hwnd and emit only when changed silverqx
        emit m_mainWindow->qBittorrentHwndChanged(nullptr);
        return true;
    case MSG_QBT_TORRENT_REMOVED:
        qDebug() << "IPC qBittorrent : Torrent removed";
        emit m_mainWindow->torrentsAddedOrRemoved();
        return true;
    case MSG_QBT_TORRENTS_ADDED:
        qDebug() << "IPC qBittorrent : Torrents added";
        emit m_mainWindow->torrentsAddedOrRemoved();
        return true;
    }

    // WM_COPYDATA section
    if (msg->message != WM_COPYDATA)
        return false;

    const COPYDATASTRUCT copyData = *reinterpret_cast<PCOPYDATASTRUCT>(msg->lParam);
    switch (static_cast<int>(msg->wParam)) {
    case MSG_QBT_TORRENTS_CHANGED:
        // Put together QVector of torrent info hashes
        const int dataSize = static_cast<int>(copyData.cbData);
        const char *const begin = static_cast<const char *>(copyData.lpData);
        QVector<QString> torrentInfoHashes;
        // One info hash has 40 bytes
        for (int i = 0; i < dataSize / INFOHASH_SIZE ; ++i)
            torrentInfoHashes << QString::fromLatin1(begin + (i * INFOHASH_SIZE),
                                                     INFOHASH_SIZE);

#if LOG_CHANGED_TORRENTS
        qDebug() << "IPC qBittorrent : Changed torrents copyData size :" << dataSize;
        qDebug() << "IPC qBittorrent : Changed torrents info hashes :" << torrentInfoHashes;
#endif

        emit m_mainWindow->torrentsChanged(torrentInfoHashes);
        return true;
    }

    return false;
}
