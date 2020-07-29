#ifndef COMMON_H
#define COMMON_H

#include <QString>

#include <qt_windows.h>

class QString;

enum UserMessages
{
    MSG_QBITTORRENT_UP       = WM_USER + 1,
    MSG_QBT_TORRENTS_CHANGED = WM_USER + 2,
    MSG_QBITTORRENT_DOWN     = WM_USER + 3,

    MSG_QMEDIA_UP            = WM_USER + 100,
    MSG_QMD_DELETE_TORRENT   = WM_USER + 101,
    MSG_QMEDIA_DOWN          = WM_USER + 102,
};

static const QString QB_EXT {QStringLiteral(".!qB")};

static const char C_NON_BREAKING_SPACE[] = " ";
static const char C_THIN_SPACE[] = " ";

#endif // COMMON_H
