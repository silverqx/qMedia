#include "fs.h"

#include <QDir>

#include "common.h"

QString Utils::Fs::toNativePath(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

QString Utils::Fs::toUniformPath(const QString &path)
{
    return QDir::fromNativeSeparators(path);
}

QString Utils::Fs::fileExtension(const QString &filename)
{
    const auto ext = QString(filename).remove(::QB_EXT);
    const auto pointIndex = ext.lastIndexOf('.');
    if (pointIndex == -1)
        return {};

    return ext.mid(pointIndex + 1);
}

QString Utils::Fs::fileName(const QString &filePath)
{
    const auto path = toUniformPath(filePath);
    const auto slashIndex = path.lastIndexOf('/');
    if (slashIndex == -1)
        return path;

    return path.mid(slashIndex + 1);
}

QString Utils::Fs::folderName(const QString &filePath)
{
    const auto path = toUniformPath(filePath);
    const auto slashIndex = path.lastIndexOf('/');
    if (slashIndex == -1)
        return path;

    return path.left(slashIndex);
}

QString Utils::Fs::expandPath(const QString &path)
{
    const auto ret = path.trimmed();
    if (ret.isEmpty())
        return ret;

    return QDir::cleanPath(ret);
}

QString Utils::Fs::expandPathAbs(const QString &path)
{
    return QDir(expandPath(path)).absolutePath();
}
