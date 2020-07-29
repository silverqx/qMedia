#include "maineventfilter_win.h"

#include <QDebug>

#include "common.h"
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
        m_mainWindow->setQBittorrentHwnd(reinterpret_cast<HWND>(msg->wParam));
        return true;
    case MSG_QBT_TORRENTS_CHANGED:
        qDebug() << "IPC qBittorrent : Torrents changed";
        emit m_mainWindow->torrentsChanged();
        return true;
    case MSG_QBITTORRENT_DOWN:
        qDebug() << "IPC qBittorrent : qBittorrent closed";
        m_mainWindow->setQBittorrentHwnd(nullptr);
        return true;
    }

    return false;
}
