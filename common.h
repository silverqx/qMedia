#ifndef COMMON_H
#define COMMON_H

#include <QString>

#include <qt_windows.h>

// TODO prepend all code references to global symbols with :: silverqx
enum UserMessages
{
    MSG_QBITTORRENT_UP            = WM_USER + 1,
    MSG_QBITTORRENT_DOWN          = WM_USER + 2,
    MSG_QBT_TORRENTS_CHANGED      = WM_USER + 3,
    MSG_QBT_TORRENT_REMOVED       = WM_USER + 4,
    MSG_QBT_TORRENTS_ADDED        = WM_USER + 5,
    MSG_QBT_TORRENT_MOVED         = WM_USER + 6,

    MSG_QMEDIA_UP                 = WM_USER + 100,
    MSG_QMEDIA_DOWN               = WM_USER + 101,
    MSG_QMD_DELETE_TORRENT        = WM_USER + 102,
    MSG_QMD_APPLICATION_ACTIVE    = WM_USER + 103,
    MSG_QMD_APPLICATION_DEACTIVE  = WM_USER + 104,
    MSG_QMD_PAUSE_TORRENT         = WM_USER + 105,
    MSG_QMD_RESUME_TORRENT        = WM_USER + 106,
    MSG_QMD_FORCE_RESUME_TORRENT  = WM_USER + 107,
    MSG_QMD_FORCE_RECHECK_TORRENT = WM_USER + 108,
};

// TODO In qBittorent was this comment, investigaste: Make it inline in C++17 silverqx
static const QString QB_EXT {QStringLiteral(".!qB")};
/*! Info hash size in bytes. */
static const int INFOHASH_SIZE = 40;
/*! Above this cap show ∞ symbol. */
static const qint64 MAX_ETA = 8640000;
/*! Hide zero values in the main transfer view. */
static const auto HIDE_ZERO_VALUES = true;

static const char C_NON_BREAKING_SPACE[] = " ";
static const char C_THIN_SPACE[] = " ";
static const char C_INFINITY[] = "∞";

static inline const auto IpcSendByteArray =
        [](const HWND hwnd, const UserMessages message, const QByteArray &byteArray)
{
    COPYDATASTRUCT copyDataStruct;
    copyDataStruct.lpData = static_cast<void *>(const_cast<char *>(byteArray.data()));
    copyDataStruct.cbData = byteArray.size();
    copyDataStruct.dwData = NULL;

    return ::SendMessage(hwnd, WM_COPYDATA,
                  (WPARAM) message,
                  (LPARAM) (LPVOID) &copyDataStruct);
};

Q_DECL_UNUSED static inline const auto IpcSendString =
        [](const HWND hwnd, const UserMessages message, const QString &string)
{
    return ::IpcSendByteArray(hwnd, message, string.toUtf8().constData());
};

#endif // COMMON_H
