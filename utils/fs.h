#ifndef UTILS_FS_H
#define UTILS_FS_H

/**
 * Utility functions related to file system.
 */

class QString;

namespace Utils
{
    namespace Fs
    {
        /**
         * Converts a path to a string suitable for display.
         * This function makes sure the directory separator used is consistent
         * with the OS being run.
         */
        QString toNativePath(const QString &path);
        /**
         * Converts a path to a string suitable for processing.
         * This function makes sure the directory separator used is independent
         * from the OS being run so it is the same on all supported platforms.
         * Slash ('/') is used as "uniform" directory separator.
         */
        QString toUniformPath(const QString &path);

        /**
         * Returns the file extension part of a file name.
         */
        QString fileExtension(const QString &filename);
        QString fileName(const QString &filePath);
        QString folderName(const QString &filePath);
    }
}

#endif // UTILS_FS_H
