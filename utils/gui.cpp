#include "gui.h"

#include <QDesktopServices>
#include <QUrl>

#include "utils/fs.h"

bool Utils::Gui::openPath(const QString &absolutePath)
{
    const QString path = Utils::Fs::toUniformPath(absolutePath);
    // Hack to access samba shares with QDesktopServices::openUrl
    if (path.startsWith(QStringLiteral("//")))
        return QDesktopServices::openUrl(Utils::Fs::toNativePath("file:" + path));

    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
