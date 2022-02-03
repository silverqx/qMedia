#include <QApplication>
#include <QStandardPaths>

#include <qt_windows.h>

#include "csfddetailservice.h"
#include "mainwindow.h"
#include "torrentstatus.h"

#ifdef Q_OS_WIN32
#include "maineventfilter_win.h"
#endif

/*! Allow only one application instance. */
bool oneAppInstanceConstrain();
/*! Create and initialize QApplication. */
QApplication &createApplication(int argc, char *argv[]); // NOLINT(modernize-avoid-c-arrays)
/*! Enable dark theme. */
void enableDarkTheme(QApplication &app);
/*! Install main native event filter. */
void installMainEventFilter(QApplication &app, MainWindow &mainWindow);
/*! Cleanup the qMedia application. */
void cleanupApplication(QApplication &app);

namespace
{
    HANDLE hMutex;
    std::unique_ptr<MainEventFilter> mainEventFilter;
}

int main(int argc, char *argv[])
{
    // Allow only one instance
    if (oneAppInstanceConstrain())
        return 1;

    auto &app = createApplication(argc, argv);

    enableDarkTheme(app);

    MainWindow mainWindow;
    mainWindow.show();

    installMainEventFilter(app, mainWindow);

    // Fire it up ✨
    auto retVal = QApplication::exec();

    cleanupApplication(app);

    return retVal;
}

bool oneAppInstanceConstrain()
{
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    hMutex = ::CreateMutex(nullptr, false, L"Global\\CsQMediaMainApp");

    return ::GetLastError() == ERROR_ALREADY_EXISTS;
}

QApplication &createApplication(int argc, char *argv[]) // NOLINT(modernize-avoid-c-arrays)
{
#ifdef QMEDIA_DEBUG
    // Redirect writable locations to %APPDATA%/qttest
    QStandardPaths::setTestModeEnabled(true);
#endif

    // TODO support High DPI scaling, it's all wrong now, I think I postpone this until QT6 will be out silverqx
//    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
//    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // TODO High DPI pixmaps silverqx
    // https://doc.qt.io/qt-5/qpainter.html#drawing-high-resolution-versions-of-pixmaps-and-images
//    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // TODO remove after upgrade to Qt6, this is the default in Qt6 silverqx
    QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    static QApplication app(argc, argv);

    // Set font size bigger by 1pt
    QFont font = QApplication::font();
    font.setPointSize(9);
    QApplication::setFont(font);

    return app;
}

void enableDarkTheme(QApplication &app)
{
#ifdef Q_OS_WIN
    QPalette darkPalette;
    const auto baseColor     = QColor(26, 27, 28);
    const auto darkColor     = QColor(45, 45, 45);
    const auto disabledColor = QColor(127, 127, 127);
    const auto textColor     = QColor(212, 212, 212);
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

    QApplication::setStyle("fusion");
    QApplication::setPalette(darkPalette);
    app.setStyleSheet("QToolTip { color: black; background-color: #ffffe1; "
                      "border: 1px solid black; }");
#endif
}

void installMainEventFilter(QApplication &app, MainWindow &mainWindow)
{
#ifdef Q_OS_WIN32
    mainEventFilter = std::make_unique<MainEventFilter>(&mainWindow);
    app.installNativeEventFilter(mainEventFilter.get());
#endif
}

void cleanupApplication(QApplication &app)
{
    CsfdDetailService::freeInstance();
    StatusHash::freeInstance();

#ifdef Q_OS_WIN32
    app.removeNativeEventFilter(mainEventFilter.get());
    mainEventFilter.reset();
#endif

    CloseHandle(hMutex);
}

// TODO check portable msvc compiler options /permissive- and /Za silverqx
// TODO enable DEFINES += QT_USE_QSTRINGBUILDER, qBittorent has it enabled byd default silverqx
// TODO disable copy constructors when appropriate, see pattern in qBittorrent silverqx
// TODO enable QT_NO_CAST_FROM/TO, code is prepared for this silverqx
// TODO unify viewbox and width / height for all svg icons silverqx
// TODO remove version from all svg icons silverqx
// TODO detect qBittorrent crashes and do something like TorrentExporter::correctTorrentStatusesOnExit() silverqx
// TODO sort includes in pch.h, look at TinyOrm pch.h file silverqx
