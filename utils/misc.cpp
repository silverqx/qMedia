#include "misc.h"

#include <QSet>

#include "common.h"
#include "utils/string.h"

namespace {
    const struct { const char *source; const char *comment; } units[] = {
        { "B", "bytes" },
        { "KiB", "kibibytes (1024 bytes)" },
        { "MiB", "mebibytes (1024 kibibytes)" },
        { "GiB", "gibibytes (1024 mibibytes)" },
        { "TiB", "tebibytes (1024 gibibytes)" },
        { "PiB", "pebibytes (1024 tebibytes)" },
        { "EiB", "exbibytes (1024 pebibytes)" }
    };

    /**
     * Return best userfriendly storage unit (B, KiB, MiB, GiB, TiB, ...).
     * Use Binary prefix standards from IEC 60027-2
     * see http://en.wikipedia.org/wiki/Kilobyte
     * Value must be given in bytes.
     * to send numbers instead of strings with suffixes
     */
    bool splitToFriendlyUnit(const qint64 sizeInBytes, qreal &val, Utils::Misc::SizeUnit &unit)
    {
        if (sizeInBytes < 0) return false;

        int i = 0;
        val = static_cast<qreal>(sizeInBytes);

        while ((val >= 1024.) && (i <= static_cast<int>(Utils::Misc::SizeUnit::ExbiByte))) {
            val /= 1024.;
            ++i;
        }

        unit = static_cast<Utils::Misc::SizeUnit>(i);
        return true;
    }
}

QString Utils::Misc::friendlyUnit(const qint64 bytesValue, const bool isSpeed)
{
    SizeUnit unit;
    qreal friendlyVal;
    if (!splitToFriendlyUnit(bytesValue, friendlyVal, unit))
        return QStringLiteral("Unknown");

    return Utils::String::fromDouble(friendlyVal, friendlyUnitPrecision(unit))
           + QString::fromUtf8(C_NON_BREAKING_SPACE)
           + unitString(unit, isSpeed);
}

int Utils::Misc::friendlyUnitPrecision(const SizeUnit unit)
{
    // friendlyUnit's number of digits after the decimal point
    switch (unit) {
    case SizeUnit::Byte:
        return 0;
    case SizeUnit::KibiByte:
    case SizeUnit::MebiByte:
        return 1;
    case SizeUnit::GibiByte:
        return 2;
    default:
        return 3;
    }
}

QString Utils::Misc::unitString(const SizeUnit unit, const bool isSpeed)
{
    const auto &unitString = units[static_cast<int>(unit)];
    QString ret = unitString.source;
    if (isSpeed)
        ret += QStringLiteral("/s");

    return ret;
}

bool Utils::Misc::isPreviewable(const QString &extension)
{
    static const QSet<QString> multimediaExtensions = {
        // TODO really investigate QStringLiteral and QLatin1String and defines which controls behaviors, make some note soomewhere about investigation result silverqx
        QStringLiteral("3GP"),
        QStringLiteral("AAC"),
        QStringLiteral("AC3"),
        QStringLiteral("AIF"),
        QStringLiteral("AIFC"),
        QStringLiteral("AIFF"),
        QStringLiteral("ASF"),
        QStringLiteral("AU"),
        QStringLiteral("AVI"),
        QStringLiteral("FLAC"),
        QStringLiteral("FLV"),
        QStringLiteral("M3U"),
        QStringLiteral("M4A"),
        QStringLiteral("M4P"),
        QStringLiteral("M4V"),
        QStringLiteral("MID"),
        QStringLiteral("MKV"),
        QStringLiteral("MOV"),
        QStringLiteral("MP2"),
        QStringLiteral("MP3"),
        QStringLiteral("MP4"),
        QStringLiteral("MPC"),
        QStringLiteral("MPE"),
        QStringLiteral("MPEG"),
        QStringLiteral("MPG"),
        QStringLiteral("MPP"),
        QStringLiteral("OGG"),
        QStringLiteral("OGM"),
        QStringLiteral("OGV"),
        QStringLiteral("QT"),
        QStringLiteral("RA"),
        QStringLiteral("RAM"),
        QStringLiteral("RM"),
        QStringLiteral("RMV"),
        QStringLiteral("RMVB"),
        QStringLiteral("SWA"),
        QStringLiteral("SWF"),
        QStringLiteral("TS"),
        QStringLiteral("VOB"),
        QStringLiteral("WAV"),
        QStringLiteral("WMA"),
        QStringLiteral("WMV"),
    };
    return multimediaExtensions.contains(extension.toUpper());
}

QString Utils::Misc::userFriendlyDuration(const qlonglong seconds, const qlonglong maxCap)
{
    if (seconds == 0)
        return QString::fromUtf8(C_INFINITY);
    if ((maxCap >= 0) && (seconds >= maxCap))
        return QString::fromUtf8(C_INFINITY);

    if (seconds < 60)
        return QStringLiteral("< 1m");

    qlonglong minutes = (seconds / 60);
    if (minutes < 60)
        return QStringLiteral("%1m").arg(QString::number(minutes));

    qlonglong hours = (minutes / 60);
    if (hours < 24) {
        minutes -= (hours * 60);
        return QStringLiteral("%1h %2m").arg(QString::number(hours), QString::number(minutes));
    }

    qlonglong days = (hours / 24);
    if (days < 365) {
        hours -= (days * 24);
        return QStringLiteral("%1d %2h").arg(QString::number(days), QString::number(hours));
    }

    const qlonglong years = (days / 365);
    days -= (years * 365);
    return QStringLiteral("%1y %2d").arg(QString::number(years), QString::number(days));
}
