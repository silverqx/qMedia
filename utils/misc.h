#pragma once
#ifndef UTILS_MISC_H
#define UTILS_MISC_H

#include <QString>

namespace Utils
{

    /*! Miscellaneous library class. */
    class Misc
    {
        Q_DISABLE_COPY(Misc)

    public:
        /*! Deleted default constructor, this is a pure library class. */
        Misc() = delete;
        /*! Deleted destructor. */
        ~Misc() = delete;

        /*! Binary prefix standards from IEC 60027-2.
            See http://en.wikipedia.org/wiki/Kilobyte. */
        enum struct SizeUnit
        {
            Byte,       // 1024^0,
            KibiByte,   // 1024^1,
            MebiByte,   // 1024^2,
            GibiByte,   // 1024^3,
            TebiByte,   // 1024^4,
            PebiByte,   // 1024^5,
            ExbiByte    // 1024^6,
            // int64 is used for sizes and thus the next units can not be handled
            // ZebiByte,   // 1024^7,
            // YobiByte,   // 1024^8
        };

        static QString unitString(SizeUnit unit, bool isSpeed = false);

        /*! Return the best user friendly storage unit (B, KiB, MiB, GiB, TiB),
            value must be given in bytes. */
        static QString friendlyUnit(qint64 bytesValue, bool isSpeed = false);
        static int friendlyUnitPrecision(SizeUnit unit);

        static bool isPreviewable(const QString &extension);

        enum struct DURATION_INPUT
        {
            SECONDS,
            MINUTES,
        };
        /*! Take a number of seconds and return a user-friendly time duration
            like '1d 2h 10m'. */
        static QString
        userFriendlyDuration(qlonglong duration, qlonglong maxCap = -1,
                             DURATION_INPUT input = DURATION_INPUT::SECONDS);
        static QString
        userFriendlyDuration(qlonglong duration,
                             DURATION_INPUT input = DURATION_INPUT::SECONDS);
    };

} // namespace Utils

#endif // UTILS_MISC_H
