INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/abstractmoviedetailservice.h \
    $$PWD/common.h \
    $$PWD/csfddetailservice.h \
    $$PWD/macros/likely.h \
    $$PWD/maineventfilter_win.h \
    $$PWD/mainwindow.h \
    $$PWD/moviedetaildialog.h \
    $$PWD/pch.h \
    $$PWD/previewlistdelegate.h \
    $$PWD/previewselectdialog.h \
    $$PWD/torrentsqltablemodel.h \
    $$PWD/torrentstatus.h \
    $$PWD/torrenttabledelegate.h \
    $$PWD/torrenttablesortmodel.h \
    $$PWD/torrenttransfertableview.h \
    $$PWD/types/moviedetail.h \
    $$PWD/utils/fs.h \
    $$PWD/utils/gui.h \
    $$PWD/utils/misc.h \
    $$PWD/utils/string.h \
    $$PWD/version.h \

SOURCES += \
    $$PWD/abstractmoviedetailservice.cpp \
    $$PWD/csfddetailservice.cpp \
    $$PWD/main.cpp \
    $$PWD/maineventfilter_win.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/moviedetaildialog.cpp \
    $$PWD/previewlistdelegate.cpp \
    $$PWD/previewselectdialog.cpp \
    $$PWD/torrentsqltablemodel.cpp \
    $$PWD/torrentstatus.cpp \
    $$PWD/torrenttabledelegate.cpp \
    $$PWD/torrenttablesortmodel.cpp \
    $$PWD/torrenttransfertableview.cpp \
    $$PWD/utils/fs.cpp \
    $$PWD/utils/gui.cpp \
    $$PWD/utils/misc.cpp \
    $$PWD/utils/string.cpp \

FORMS += \
    $$PWD/mainwindow.ui \
    $$PWD/moviedetaildialog.ui \
    $$PWD/previewselectdialog.ui \

RESOURCES += \
    $$PWD/images/qMedia.qrc \
