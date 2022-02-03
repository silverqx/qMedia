#include "utils/misc.h"

#include <QSet>

#include <array>

#include "common.h"
#include "utils/string.h"

namespace Utils
{

namespace
{
    struct units_t {
        const char *name;
        const char *comment;
    };

    std::array<units_t, 7> units {{
        {"B",   "bytes"},
        {"KiB", "kibibytes (1024 bytes)"},
        {"MiB", "mebibytes (1024 kibibytes)"},
        {"GiB", "gibibytes (1024 mibibytes)"},
        {"TiB", "tebibytes (1024 gibibytes)"},
        {"PiB", "pebibytes (1024 tebibytes)"},
        {"EiB", "exbibytes (1024 pebibytes)"},
    }};

    /*! Return best userfriendly storage unit (B, KiB, MiB, GiB, TiB, ...).
        Use Binary prefix standards from IEC 60027-2
        see http://en.wikipedia.org/wiki/Kilobyte
        Value must be given in bytes. */
    bool splitToFriendlyUnit(const qint64 sizeInBytes, qreal &value, Misc::SizeUnit &unit)
    {
        if (sizeInBytes < 0)
            return false;

        int i = 0;
        value = static_cast<qreal>(sizeInBytes);

        while ((value >= 1024.) && (i <= static_cast<int>(Misc::SizeUnit::ExbiByte))) {
            value /= 1024.;
            ++i;
        }

        unit = static_cast<Misc::SizeUnit>(i);
        return true;
    }
} // namespace

QString Misc::friendlyUnit(const qint64 bytesValue, const bool isSpeed)
{
    SizeUnit unit;
    qreal friendlyValue = 0.0;

    if (!splitToFriendlyUnit(bytesValue, friendlyValue, unit))
        return QStringLiteral("Unknown");

    return Utils::String::fromDouble(friendlyValue, friendlyUnitPrecision(unit)) +
           ::C_NON_BREAKING_SPACE +
           unitString(unit, isSpeed);
}

int Misc::friendlyUnitPrecision(const SizeUnit unit)
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

QString Misc::unitString(const SizeUnit unit, const bool isSpeed)
{
    const auto &unitName = units.at(static_cast<int>(unit)).name;

    if (!isSpeed)
        return unitName;

    return QStringLiteral("%1/s").arg(unitName);
}

bool Misc::isPreviewable(const QString &extension)
{
    static const QSet<QString> multimediaExtensions = {
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

QString Misc::userFriendlyDuration(const qlonglong duration, const qlonglong maxCap,
                                   const DURATION_INPUT input)
{
    if (duration == 0)
        return ::C_INFINITY;
    if ((maxCap >= 0) && (duration >= maxCap))
        return ::C_INFINITY;

    qlonglong seconds = 0;
    qlonglong minutes = 0;

    switch (input) {
    case DURATION_INPUT::SECONDS: {
        seconds = duration;
        if (seconds < 60)
            return QStringLiteral("< 1m");

        minutes = (seconds / 60);
        if (minutes < 60)
            return QStringLiteral("%1m").arg(QString::number(minutes));
        break;
    }
    case DURATION_INPUT::MINUTES:
        minutes = duration;
        break;
    }

    qlonglong hours = (minutes / 60);
    if (hours < 24) {
        minutes -= (hours * 60);
        return QStringLiteral("%1h %2m").arg(QString::number(hours),
                                             QString::number(minutes));
    }

    qlonglong days = (hours / 24);
    if (days < 365) {
        hours -= (days * 24);
        return QStringLiteral("%1d %2h").arg(QString::number(days),
                                             QString::number(hours));
    }

    const qlonglong years = (days / 365);
    days -= (years * 365);
    return QStringLiteral("%1y %2d").arg(QString::number(years), QString::number(days));
}

QString Misc::userFriendlyDuration(const qlonglong duration,
                                   const Misc::DURATION_INPUT input)
{
    return userFriendlyDuration(duration, -1, input);
}

} // namespace Utils
