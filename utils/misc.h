#ifndef MISC_H
#define MISC_H

#include <QString>

namespace Utils {
    namespace Misc {
        /**
         * Use binary prefix standards from IEC 60027-2
         * see http://en.wikipedia.org/wiki/Kilobyte
         */
        enum class SizeUnit
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

        QString unitString(SizeUnit unit, bool isSpeed = false);

        /**
         * Return the best user friendly storage unit (B, KiB, MiB, GiB, TiB),
         * value must be given in bytes.
         */
        QString friendlyUnit(qint64 bytesValue, bool isSpeed = false);
        int friendlyUnitPrecision(SizeUnit unit);

        bool isPreviewable(const QString &extension);
    }
}

#endif // MISC_H
