#include <QApplication>
//#include <QSettings>

#include <qt_windows.h>

#ifdef Q_OS_WIN32
#include "maineventfilter_win.h"
#endif
#include "mainwindow.h"

//void enableDarkTheme(QApplication &a);

int main(int argc, char *argv[])
{
    HANDLE ghMutex = ::CreateMutex(NULL, false, L"Global\\CsQMediaMainApp");
    // Can run only one instance
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
        return 1;

    QApplication app(argc, argv);

//    enableDarkTheme(a);

    MainWindow w;
    w.show();

#ifdef Q_OS_WIN32
    app.installNativeEventFilter(new MainEventFilter(&w));
#endif

    int retVal = app.exec();
    CloseHandle(ghMutex);
    return retVal;
}

//void enableDarkTheme(QApplication &a)
//{
//#ifdef Q_OS_WIN
//    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\"
//                       "Themes\\Personalize",
//                       QSettings::NativeFormat);
//    if (settings.value("AppsUseLightTheme") == 0){
//        QPalette darkPalette;
//        QColor darkColor = QColor(45, 45, 45);
//        QColor disabledColor = QColor(127, 127, 127);
//        darkPalette.setColor(QPalette::Window, darkColor);
//        darkPalette.setColor(QPalette::WindowText, Qt::white);
//        darkPalette.setColor(QPalette::Base, QColor(18, 18, 18));
//        darkPalette.setColor(QPalette::AlternateBase, darkColor);
//        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
//        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
//        darkPalette.setColor(QPalette::Text, Qt::white);
//        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
//        darkPalette.setColor(QPalette::Button, darkColor);
//        darkPalette.setColor(QPalette::ButtonText, Qt::white);
//        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
//        darkPalette.setColor(QPalette::BrightText, Qt::red);
//        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

//        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
//        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
//        darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

//        a.setStyle("fusion");
//        a.setPalette(darkPalette);
//        a.setStyleSheet("QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }");
//    }
//#endif
//}
