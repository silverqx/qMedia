QT += core gui widgets sql network

# Configuration
# ---
CONFIG += c++17
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

# Qt defines
# ---

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#DEFINES += QT_NO_CAST_TO_ASCII
#DEFINES += QT_NO_CAST_FROM_BYTEARRAY
#DEFINES += QT_USE_QSTRINGBUILDER
DEFINES += QT_STRICT_ITERATORS

# WinApi
# ---
# Windows 10 1903 "19H1" - 0x0A000007
DEFINES += NTDDI_VERSION=0x0A000007
# Windows 10 - 0x0A00
DEFINES += _WIN32_WINNT=0x0A00
DEFINES += _WIN32_IE=0x0A00
DEFINES += UNICODE
DEFINES += _UNICODE
DEFINES += WIN32
DEFINES += _WIN32
DEFINES += WIN32_LEAN_AND_MEAN
DEFINES += NOMINMAX

win32-msvc* {
    LIBS += User32.lib

    QMAKE_CXXFLAGS += /guard:cf /utf-8
    QMAKE_LFLAGS += /guard:cf
    QMAKE_LFLAGS_RELEASE += /OPT:REF /OPT:ICF
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
    config.h \
    csfddetailservice.h \
    maineventfilter_win.h \
    mainwindow.h \
    moviedetaildialog.h \
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
