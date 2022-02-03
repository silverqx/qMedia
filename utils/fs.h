#pragma once
#ifndef UTILS_FS_H
#define UTILS_FS_H

#include <QtGlobal>

class QString;

namespace Utils
{

    /*! Filesystem library class. */
    class Fs
    {
        Q_DISABLE_COPY(Fs)

    public:
        /*! Deleted default constructor, this is a pure library class. */
        Fs() = delete;
        /*! Deleted destructor. */
        ~Fs() = delete;

        /*! Converts a path to a string suitable for display. */
        static QString toNativePath(const QString &path);
        /*! Converts a path to a string suitable for processing. */
        static QString toUniformPath(const QString &path);

        /*! Returns the file extension part of a file name. */
        static QString fileExtension(const QString &filename);
        static QString fileName(const QString &filePath);
        static QString folderName(const QString &filePath);
        static QString expandPath(const QString &path);
        static QString expandPathAbs(const QString &path);
    };

} // namespace Utils

#endif // UTILS_FS_H
