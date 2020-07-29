#include "torrentsqltablemodel.h"

#include <QDateTime>
#include <QLocale>
#include <QSqlField>
#include <QSqlRecord>

#include "utils/misc.h"

TorrentSqlTableModel::TorrentSqlTableModel(QObject *parent, const QSqlDatabase db)
    : QSqlTableModel(parent, db)
{}

QVariant TorrentSqlTableModel::data(const QModelIndex &idx, const int role) const
{
    if (!idx.isValid())
        return {};

    const int column = idx.column();

    switch (role) {
    case Qt::DisplayRole:
        return displayValue(idx, column);
    case UnderlyingDataRole:
        return record(idx.row()).value(column);
    case Qt::TextAlignmentRole:
        switch (column) {
        case TR_ID:
        case TR_PROGRESS:
            return QVariant {Qt::AlignCenter};
        case TR_SIZE:
        case TR_ADDED_ON:
        case TR_HASH:
            return QVariant {Qt::AlignRight | Qt::AlignVCenter};
        }
        break;
    case Qt::ToolTipRole:
        switch (column) {
        case TR_NAME:
            return displayValue(idx, column);
        }
        break;
    }

    return QSqlTableModel::data(idx, role);
}

QString TorrentSqlTableModel::displayValue(const QModelIndex &modelIndex, const int column) const
{
    bool hideValues = false;
    const auto unitString = [hideValues](const qint64 value, const bool isSpeedUnit = false) -> QString
    {
        return ((value == 0) && hideValues)
            ? QString {} : Utils::Misc::friendlyUnit(value, isSpeedUnit);
    };

    // Get value from underlying model
    const QVariant rawData = record(modelIndex.row()).value(column);
    if (!rawData.isValid() || rawData.isNull())
        return {};

    switch (column) {
    case TR_ID:
    case TR_NAME:
    case TR_HASH:
        return rawData.toString();
    case TR_PROGRESS:
        return QString::number(rawData.toReal() / 10) + '%';
    case TR_ADDED_ON:
        return QLocale::system().toString(rawData.toDateTime(), QLocale::ShortFormat);
    case TR_SIZE:
        return unitString(rawData.toLongLong());
    }

    return {};
}
