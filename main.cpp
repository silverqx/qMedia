#include <QApplication>
#include <QStandardPaths>

#include <qt_windows.h>

#include "csfddetailservice.h"

#ifdef Q_OS_WIN32
#include "maineventfilter_win.h"
#endif
#include "mainwindow.h"

// TODO unify viewbox and width / height for all svg icons silverqx
// TODO remove version from all svg icons silverqx
// TODO detect qBittorrent crashes and do something like TorrentExporter::correctTorrentStatusesOnExit() silverqx
void enableDarkTheme(QApplication &a);
void applicationCleanup();

int main(int argc, char *argv[])
{
    HANDLE ghMutex = ::CreateMutex(NULL, false, L"Global\\CsQMediaMainApp");
    // Can run only one instance
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
        return 1;

#ifdef QT_DEBUG
    // Redirect writable locations to %APPDATA%/qttest
    QStandardPaths::setTestModeEnabled(true);
#endif

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        // TODO remove after upgrade to Qt6, this is the default in Qt6 silverqx
        app.setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

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

    applicationCleanup();
    CloseHandle(ghMutex);

    return retVal;
}

void enableDarkTheme(QApplication &a)
{
#ifdef Q_OS_WIN
    QPalette darkPalette;
    const auto baseColor = QColor(26, 27, 28);
    const auto darkColor = QColor(45, 45, 45);
    const auto disabledColor = QColor(127, 127, 127);
    const auto textColor = QColor(212, 212, 212);
    darkPalette.setColor(QPalette::Window, darkColor);
    darkPalette.setColor(QPalette::WindowText, QColor(190, 190, 190));
    darkPalette.setColor(QPalette::Base, baseColor);
    darkPalette.setColor(QPalette::AlternateBase, darkColor);
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, textColor);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
    darkPalette.setColor(QPalette::Button, darkColor);
    darkPalette.setColor(QPalette::ButtonText, textColor);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
    darkPalette.setColor(QPalette::BrightText, textColor);
    darkPalette.setColor(QPalette::Link, QColor(204, 128, 56));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));

    // Dark pastel orange
    darkPalette.setColor(QPalette::Highlight, QColor(138, 96, 44, 70));
    darkPalette.setColor(QPalette::HighlightedText, textColor);
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

    // Shadow for disabled text
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, baseColor);

    a.setStyle("fusion");
    a.setPalette(darkPalette);
    a.setStyleSheet("QToolTip { color: black; background-color: #ffffe1; "
                    "border: 1px solid black; }");
#endif
}

void applicationCleanup()
{
    CsfdDetailService::freeInstance();
}
