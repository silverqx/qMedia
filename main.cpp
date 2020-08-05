#include <QApplication>

#include <qt_windows.h>

#ifdef Q_OS_WIN32
#include "maineventfilter_win.h"
#endif
#include "mainwindow.h"

void enableDarkTheme(QApplication &a);

int main(int argc, char *argv[])
{
    HANDLE ghMutex = ::CreateMutex(NULL, false, L"Global\\CsQMediaMainApp");
    // Can run only one instance
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
        return 1;

    QApplication app(argc, argv);

    // Set font size bigger by 1pt
    QFont font = app.font();
    font.setPointSize(9);
    app.setFont(font);

    enableDarkTheme(app);

    MainWindow w;
    w.show();

#ifdef Q_OS_WIN32
    app.installNativeEventFilter(new MainEventFilter(&w));
#endif

    int retVal = app.exec();
    CloseHandle(ghMutex);
    return retVal;
}

void enableDarkTheme(QApplication &a)
{
#ifdef Q_OS_WIN
    QPalette darkPalette;
    const QColor darkColor = QColor(45, 45, 45);
    const QColor disabledColor = QColor(127, 127, 127);
    const QColor textColor = QColor(212, 212, 212);
    darkPalette.setColor(QPalette::Window, darkColor);
    darkPalette.setColor(QPalette::WindowText, QColor(190, 190, 190));
    darkPalette.setColor(QPalette::Base, QColor(26, 27, 28));
    darkPalette.setColor(QPalette::AlternateBase, darkColor);
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, textColor);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
    darkPalette.setColor(QPalette::Button, darkColor);
    darkPalette.setColor(QPalette::ButtonText, textColor);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
    darkPalette.setColor(QPalette::BrightText, textColor);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));

    // Blue
//    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
//    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    // Dark pastel green
//    darkPalette.setColor(QPalette::Highlight, QColor(29, 84, 92));
//    darkPalette.setColor(QPalette::HighlightedText, textColor);
    // Dark orange
//    darkPalette.setColor(QPalette::Highlight, QColor(255, 139, 0));
//    darkPalette.setColor(QPalette::HighlightedText, textColor);
    // Dark pastel orange
    darkPalette.setColor(QPalette::Highlight, QColor(138, 96, 44));
    darkPalette.setColor(QPalette::HighlightedText, textColor);
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

    a.setStyle("fusion");
    a.setPalette(darkPalette);
    a.setStyleSheet("QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }");
#endif
}
