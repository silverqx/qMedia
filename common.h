#ifndef COMMON_H
#define COMMON_H

#include <QString>

#include <qt_windows.h>

enum UserMessages
{
    MSG_QBITTORRENT_UP       = WM_USER + 1,
    MSG_QBITTORRENT_DOWN     = WM_USER + 2,
    MSG_QBT_TORRENTS_CHANGED = WM_USER + 3,
    MSG_QBT_TORRENT_REMOVED  = WM_USER + 4,
    MSG_QBT_TORRENTS_ADDED   = WM_USER + 5,

    MSG_QMEDIA_UP            = WM_USER + 100,
    MSG_QMEDIA_DOWN          = WM_USER + 101,
    MSG_QMD_DELETE_TORRENT   = WM_USER + 102,
};

// TODO In qBittorent was this comment, investigaste: Make it inline in C++17 silverqx
static const QString QB_EXT {QStringLiteral(".!qB")};
/*! Info hash size in bytes. */
static const int INFOHASH_SIZE = 40;
/*! Above this cap show ∞ symbol. */
static const qint64 MAX_ETA = 8640000;

static const char C_NON_BREAKING_SPACE[] = " ";
static const char C_THIN_SPACE[] = " ";
static const char C_INFINITY[] = "∞";

#endif // COMMON_H
