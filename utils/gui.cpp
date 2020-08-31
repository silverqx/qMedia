#include "gui.h"

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QApplication>
#include <QRect>
#include <QScreen>
#include <QUrl>
#include <QWidget>

#include "utils/fs.h"

bool Utils::Gui::openPath(const QString &absolutePath)
{
    const QString path = Utils::Fs::toUniformPath(absolutePath);
    // Hack to access samba shares with QDesktopServices::openUrl
    if (path.startsWith(QStringLiteral("//")))
        return QDesktopServices::openUrl(Utils::Fs::toNativePath("file:" + path));

    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void Utils::Gui::centerDialog(QWidget *const widget)
{
    const auto currentScreen = QApplication::desktop()->screenNumber();
    const auto screens = QApplication::screens();
    const auto screenGeometry = screens[currentScreen]->geometry();
    const int x = (screenGeometry.width() - widget->frameGeometry().width()) / 2;
    int y = (screenGeometry.height() - widget->frameGeometry().height()) / 2;
#ifdef QT_DEBUG
    // Move little up, so I can see console output
    if (y > 130)
        y -= 100;
#else
    // Move a little to the top, better for an eye
    if (y > 40)
        y -= 14;
#endif
    // Also prevent to move out of the screen
    widget->move(x < 0 ? 0 : x, y < 0 ? 0 : y);
}
