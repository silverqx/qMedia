#include "torrenttablesortmodel.h"

#include <QDateTime>

#include "common.h"

namespace
{
    /*! Check, whether the torrent transfer table can be sorted by added on column,
        when left and right values in lessThan() are the same.
        Columns defined in the vector have their own special sorting rules. */
    const auto sortByAddedOn = [](const auto column)
    {
        static const QVector<int> cached {
            TorrentSqlTableModel::TR_ADDED_ON,
            TorrentSqlTableModel::TR_ETA,
            TorrentSqlTableModel::TR_SEEDS,
            TorrentSqlTableModel::TR_LEECHERS,
        };

        // Exclude sorting columns defined in the vector
        return !cached.contains(column);
    };
} // namespace

bool TorrentTableSortModel::lessThan( // NOLINT(misc-no-recursion)
        const QModelIndex &left, const QModelIndex &right) const
{
    const auto sortColumn = left.column();

    // UnderlyingDataRole contains raw value from db
    const auto leftRawValue = left.data(TorrentSqlTableModel::UnderlyingDataRole);
    const auto rightRawValue = right.data(TorrentSqlTableModel::UnderlyingDataRole);

    /* If not valid or is the same value, then sort by added on column.
       It's something like a second sort, but only if items have the same values. */
    if (!leftRawValue.isValid() || !rightRawValue.isValid() ||
        (sortByAddedOn(sortColumn) && leftRawValue == rightRawValue)
    )
        return invokeLessThanForColumn(TorrentSqlTableModel::TR_ADDED_ON, left, right);

    switch (sortColumn) {
    case TorrentSqlTableModel::TR_ADDED_ON:
        return leftRawValue.toDateTime() < rightRawValue.toDateTime();

    case TorrentSqlTableModel::TR_SIZE:
    case TorrentSqlTableModel::TR_AMOUNT_LEFT:
        return leftRawValue.toULongLong() < rightRawValue.toULongLong();

    case TorrentSqlTableModel::TR_PROGRESS:
        return leftRawValue.toUInt() < rightRawValue.toUInt();

    case TorrentSqlTableModel::TR_ETA:
        return sortETA(left, right, leftRawValue, rightRawValue);

    case TorrentSqlTableModel::TR_SEEDS:
    case TorrentSqlTableModel::TR_LEECHERS:
        return sortSeedsAndLeechers(left, right, leftRawValue, rightRawValue);
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

bool TorrentTableSortModel::sortETA( // NOLINT(misc-no-recursion)
        const QModelIndex &left, const QModelIndex &right,
        const QVariant &leftRawValue, const QVariant &rightRawValue) const
{
    auto leftValue = leftRawValue.toLongLong();
    auto rightValue = rightRawValue.toLongLong();

    /* Interpret values larger than max. ETA cap as 0, to avoid always being sorted
       on top / bottom. */
    if (leftValue >= ::MAX_ETA)
        leftValue = 0;
    if (rightValue >= ::MAX_ETA)
        rightValue = 0;

    /* Here for performance reason, it could be all above, after QVariant declaration,
       but I don't want to do this MAX_ETA cap checks for every sort, so this second
       sorting by TR_ADDED_ON column have to be here again. */
    if (leftRawValue == rightRawValue)
        return invokeLessThanForColumn(TorrentSqlTableModel::TR_ADDED_ON, left, right);

    return leftValue < rightValue;
}

bool TorrentTableSortModel::sortSeedsAndLeechers( // NOLINT(misc-no-recursion)
        const QModelIndex &left, const QModelIndex &right,
        const QVariant &leftRawValue, const QVariant &rightRawValue) const
{
    const auto leftValue = leftRawValue.toInt();
    const auto rightValue = rightRawValue.toInt();

    // Active seeds/leechers take precedence over total seeds/leechers
    if (leftValue != rightValue)
        return leftValue < rightValue;

    /* Second sort by total seeds/leechers. Obtain Total seeds/leechers from
       a sibling column. */
    const auto sortColumn = left.column();

    const auto leftValueTotal =
            left.sibling(left.row(),
                         TorrentSqlTableModel::mapToTotal.value(sortColumn))
            .data(TorrentSqlTableModel::UnderlyingDataRole).toInt();
    const auto rightValueTotal =
            right.sibling(right.row(),
                          TorrentSqlTableModel::mapToTotal.value(sortColumn))
            .data(TorrentSqlTableModel::UnderlyingDataRole).toInt();

    if (leftValueTotal != rightValueTotal)
        return leftValueTotal < rightValueTotal;

    // Third sort by added on
    return invokeLessThanForColumn(TorrentSqlTableModel::TR_ADDED_ON, left, right);
}
