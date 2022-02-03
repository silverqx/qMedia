#include "utils/gui.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QRect>
#include <QScreen>
#include <QUrl>
#include <QWidget>

#include "utils/fs.h"

namespace Utils
{

bool Gui::openPath(const QString &absolutePath)
{
    const auto path = Fs::toUniformPath(absolutePath);

    // Hack to access samba shares with QDesktopServices::openUrl
    if (path.startsWith(QStringLiteral("//")))
        return QDesktopServices::openUrl(
                    Fs::toNativePath(QStringLiteral("file:%1").arg(path)));

    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void Gui::centerDialog(QWidget *const widget)
{
    const auto currentScreen = QApplication::desktop()->screenNumber();
    const auto screens = QApplication::screens();
    const auto screenGeometry = screens[currentScreen]->geometry();

    // X position to center
    const int x = (screenGeometry.width() - widget->frameGeometry().width()) / 2;
#ifdef QMEDIA_DEBUG
    // Move little up in the development, to see console output
    const auto yCoeficient = 4;
#else
    // Y position to center in the production
    const auto yCoeficient = 2;
#endif

    // Compute a Y position
    auto y = (screenGeometry.height() - widget->frameGeometry().height()) / yCoeficient;
#ifdef QMEDIA_NO_DEBUG
    // Move a little to the top, better for an eye
    if (y > 30)
        y -= 14;
#endif

    // Prevent to move out of the screen
    widget->move(x < 0 ? 0 : x, y < 0 ? 0 : y);
}

} // namespace Utils
