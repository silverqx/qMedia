QT += core gui widgets sql network

# Configuration
# ---
CONFIG += c++2a strict_c++ silent utf8_source
CONFIG -= c++11
# Enable stack trace support
#CONFIG += stacktrace

# Some info output
# ---
CONFIG(debug, debug|release): message( "Project is built in DEBUG mode." )
CONFIG(release, debug|release): message( "Project is built in RELEASE mode." )

# Disable debug output in release mode
CONFIG(release, debug|release) {
    message( "Disabling debug output." )
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# qMedia defines
# ---
DEFINES += PROJECT_QMEDIA

# Qt defines
# ---

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
# Disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#DEFINES *= QT_ASCII_CAST_WARNINGS
#DEFINES += QT_NO_CAST_FROM_ASCII
#DEFINES += QT_RESTRICTED_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += QT_NO_CAST_FROM_BYTEARRAY
DEFINES += QT_USE_QSTRINGBUILDER
DEFINES += QT_STRICT_ITERATORS

# WinApi
# ---
# All have to be defined because of checks at the beginning of the qt_windows.h
# Windows 10 1903 "19H1" - 0x0A000007
DEFINES += WINVER=_WIN32_WINNT_WIN10
DEFINES += NTDDI_VERSION=NTDDI_WIN10_19H1
DEFINES += _WIN32_WINNT=_WIN32_WINNT_WIN10
# Internet Explorer 11
DEFINES += _WIN32_IE=_WIN32_IE_IE110
DEFINES += UNICODE
DEFINES += _UNICODE
DEFINES += WIN32
DEFINES += _WIN32
# Exclude unneeded header files
DEFINES += WIN32_LEAN_AND_MEAN
DEFINES += NOMINMAX

win32-msvc* {
    # MySQL C library is used by ORM and it uses mysql_ping()
    INCLUDEPATH += $$quote(C:/Program Files/MySQL/MySQL Server 8.0/include)
    LIBS += $$quote(-LC:/Program Files/MySQL/MySQL Server 8.0/lib)

    LIBS += libmysql.lib
    LIBS += User32.lib

    # I don't use -MP flag, because using jom
    QMAKE_CXXFLAGS += -guard:cf
    QMAKE_LFLAGS += /guard:cf
    QMAKE_LFLAGS_RELEASE += /OPT:REF /OPT:ICF=5
}

# File version and windows manifest
# ---
win32:VERSION = 0.1.0.0
else:VERSION = 0.1.0

win32-msvc* {
    QMAKE_TARGET_PRODUCT = qMedia
    QMAKE_TARGET_DESCRIPTION = qMedia media library for qBittorrent
    QMAKE_TARGET_COMPANY = Crystal Studio
    QMAKE_TARGET_COPYRIGHT = Copyright (©) 2020 Crystal Studio
    RC_ICONS = images/qMedia.ico
    RC_LANG = 1033
}

# Stacktrace support
# ---
stacktrace {
    DEFINES += STACKTRACE
    win32 {
        DEFINES += STACKTRACE_WIN_PROJECT_PATH=$$PWD
        DEFINES += STACKTRACE_WIN_MAKEFILE_PATH=$$OUT_PWD
    }
    win32-g++* {
        contains(QMAKE_HOST.arch, x86) {
            # i686 arch requires frame pointer preservation
            QMAKE_CXXFLAGS += -fno-omit-frame-pointer
        }

        QMAKE_LFLAGS += -Wl,--export-all-symbols

        LIBS += libdbghelp
    }
    else:win32-msvc* {
        contains(QMAKE_HOST.arch, x86) {
            # i686 arch requires frame pointer preservation
            QMAKE_CXXFLAGS += -Oy-
        }

        QMAKE_CXXFLAGS *= -Zi
        QMAKE_LFLAGS *= /DEBUG

        LIBS += dbghelp.lib
    }
}

# Use Precompiled headers (PCH)
# ---

PRECOMPILED_HEADER = $$quote($$PWD/pch.h)
HEADERS += $$PRECOMPILED_HEADER

precompile_header: \
    DEFINES *= USING_PCH

# qMediaCommon project
# ---

include(../qMediaCommon/qmediacommon.pri)

# Application files
# ---
SOURCES += \
    abstractmoviedetailservice.cpp \
    csfddetailservice.cpp \
    main.cpp \
    maineventfilter_win.cpp \
    mainwindow.cpp \
    moviedetaildialog.cpp \
    previewlistdelegate.cpp \
    previewselectdialog.cpp \
    torrentsqltablemodel.cpp \
    torrentstatus.cpp \
    torrenttabledelegate.cpp \
    torrenttablesortmodel.cpp \
    torrenttransfertableview.cpp \
    utils/fs.cpp \
    utils/gui.cpp \
    utils/misc.cpp \
    utils/string.cpp \

HEADERS += \
    abstractmoviedetailservice.h \
    common.h \
    csfddetailservice.h \
    maineventfilter_win.h \
    mainwindow.h \
    moviedetaildialog.h \
    pch.h \
    previewlistdelegate.h \
    previewselectdialog.h \
    torrentsqltablemodel.h \
    torrentstatus.h \
    torrenttabledelegate.h \
    torrenttablesortmodel.h \
    torrenttransfertableview.h \
    utils/fs.h \
    utils/gui.h \
    utils/misc.h \
    utils/string.h \

FORMS += \
    mainwindow.ui \
    moviedetaildialog.ui \
    previewselectdialog.ui \

RESOURCES += \
    images/qMedia.qrc

# For Qt Creator beautifier
# not using for now
#DISTFILES += \
#    uncrustify.cfg

# Default rules for deployment.
# ---
release {
    win32-msvc*: target.path = c:/optx64/$${TARGET}
    !isEmpty(target.path): INSTALLS += target
}
