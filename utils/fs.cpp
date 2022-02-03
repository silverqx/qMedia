#include "utils/fs.h"

#include <QDir>

#include "common.h"

namespace Utils
{

QString Fs::toNativePath(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

QString Fs::toUniformPath(const QString &path)
{
    return QDir::fromNativeSeparators(path);
}

QString Fs::fileExtension(const QString &filename)
{
    const auto ext = QString(filename).remove(::QB_EXT);
    const auto pointIndex = ext.lastIndexOf(QChar('.'));

    if (pointIndex == -1)
        return {};

    return ext.mid(pointIndex + 1);
}

QString Fs::fileName(const QString &filePath)
{
    auto path = toUniformPath(filePath);
    const auto slashIndex = path.lastIndexOf(QChar('/'));

    if (slashIndex == -1)
        return path;

    return path.mid(slashIndex + 1);
}

QString Fs::folderName(const QString &filePath)
{
    auto path = toUniformPath(filePath);
    const auto slashIndex = path.lastIndexOf(QChar('/'));

    if (slashIndex == -1)
        return path;

    return path.left(slashIndex);
}

QString Fs::expandPath(const QString &path)
{
    auto result = path.trimmed();

    if (result.isEmpty())
        return result;

    return QDir::cleanPath(result);
}

QString Fs::expandPathAbs(const QString &path)
{
    return QDir(expandPath(path)).absolutePath();
}

} // namespace Utils
