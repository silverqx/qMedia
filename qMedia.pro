QT += core gui widgets sql network

TEMPLATE = app
TARGET = qMedia

# Configuration
# ---

CONFIG += c++2a strict_c++ silent utf8_source warn_on
CONFIG -= c++11

# Enable stack trace support
#CONFIG += stacktrace

# qMedia defines
# ---

DEFINES += PROJECT_QMEDIA

# Release build
CONFIG(release, debug|release): DEFINES *= QMEDIA_NO_DEBUG
# Debug build
CONFIG(debug, debug|release): DEFINES *= QMEDIA_DEBUG

# Qt defines
# ---

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
# Disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

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

# qMediaCommon project
# ---

include(../qMediaCommon/qmediacommon.pri)

# qMedia header and source files
# ---

include(src.pri)

# File version
# ---

# Find version numbers in the version header file and assign them to the
# <TARGET>_VERSION_<MAJOR,MINOR,PATCH,TWEAK> and also to the VERSION variable.
load(tiny_version_numbers)
tiny_version_numbers()

# Windows resource and manifest files
# ---

# Find version.h
tinyRcIncludepath = $$quote($$PWD/)
# Find Windows manifest
mingw: tinyRcIncludepath += $$quote($$PWD/resources/)

load(tiny_resource_and_manifest)
tiny_resource_and_manifest($$tinyRcIncludepath)

# Use Precompiled headers (PCH)
# ---

include(pch.pri)

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

# Deployment
# ---

release {
    win32-msvc*: target.path = c:/optx64/$${TARGET}
    !isEmpty(target.path): INSTALLS += target
}

# For Qt Creator beautifier
# not using for now
#DISTFILES += \
#    uncrustify.cfg

# Some info output
# ---

CONFIG(debug, debug|release):!build_pass: message( "Project is built in DEBUG mode." )
CONFIG(release, debug|release):!build_pass: message( "Project is built in RELEASE mode." )

# Disable debug output in release mode
CONFIG(release, debug|release): \
    DEFINES *= QT_NO_DEBUG_OUTPUT
