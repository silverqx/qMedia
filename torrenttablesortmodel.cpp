#include "torrenttablesortmodel.h"

#include <QDateTime>

#include "torrentsqltablemodel.h"

TorrentTableSortModel::TorrentTableSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{}

bool TorrentTableSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto invokeLessThanForColumn = [this, &left, &right](const int column) -> bool
    {
        return lessThan(left.sibling(left.row(), column), right.sibling(right.row(), column));
    };

    const int sortColumn = left.column();

    const QVariant leftRawValue = left.data(TorrentSqlTableModel::UnderlyingDataRole);
    const QVariant rightRawValue = right.data(TorrentSqlTableModel::UnderlyingDataRole);
    // If not valid or is the same value, than sort by added on column
    // It is like second sort, but only if items have the same values
    if (!leftRawValue.isValid() || !rightRawValue.isValid()
        || ((leftRawValue == rightRawValue) && (sortColumn != TorrentSqlTableModel::TR_ADDED_ON)))
        return invokeLessThanForColumn(TorrentSqlTableModel::TR_ADDED_ON);

    switch (sortColumn) {
    case TorrentSqlTableModel::TR_ADDED_ON:
        return leftRawValue.toDateTime() < rightRawValue.toDateTime();
    case TorrentSqlTableModel::TR_SIZE:
        return leftRawValue.toULongLong() < rightRawValue.toULongLong();
    case TorrentSqlTableModel::TR_PROGRESS:
        return leftRawValue.toUInt() < rightRawValue.toUInt();
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
