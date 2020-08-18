#include "gui.h"

#include <QDesktopServices>
#include <QGuiApplication>
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
    // TODO detect active screen silverqx
    const auto screenGeometry = QGuiApplication::screens()[0]->geometry();
    const int x = (screenGeometry.width() - widget->frameGeometry().width()) / 2;
    const int y = (screenGeometry.height() - widget->frameGeometry().height()) / 2;
    widget->move(x, y);
}
