#include "torrenttablesortmodel.h"

#include <QDateTime>

#include "common.h"
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
        || ((leftRawValue == rightRawValue) && (sortColumn != TorrentSqlTableModel::TR_ADDED_ON)
            && (sortColumn != TorrentSqlTableModel::TR_ETA)))
        return invokeLessThanForColumn(TorrentSqlTableModel::TR_ADDED_ON);

    switch (sortColumn) {
    case TorrentSqlTableModel::TR_ADDED_ON:
        return leftRawValue.toDateTime() < rightRawValue.toDateTime();
    case TorrentSqlTableModel::TR_SIZE:
    case TorrentSqlTableModel::TR_AMOUNT_LEFT:
        return leftRawValue.toULongLong() < rightRawValue.toULongLong();
    case TorrentSqlTableModel::TR_PROGRESS:
        return leftRawValue.toUInt() < rightRawValue.toUInt();
    case TorrentSqlTableModel::TR_ETA:
        auto leftValue = leftRawValue.toLongLong();
        auto rightValue = rightRawValue.toLongLong();
        /* Interpret values larger that max ETA cap as 0, to avoid always being sorted
           on top / bottom. */
        if (leftValue >= MAX_ETA)
            leftValue = 0;
        if (rightValue >= MAX_ETA)
            rightValue = 0;
        /* Here for performance reason, it could be all above, after QVariant declaration,
           but I don't want to do this MAX_ETA cap checks for every sort, so this second sorting
           by TR_ADDED_ON column have to be here again. */
        if (leftRawValue == rightRawValue)
            return invokeLessThanForColumn(TorrentSqlTableModel::TR_ADDED_ON);
        return leftValue < rightValue;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
