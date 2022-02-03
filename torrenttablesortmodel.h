#pragma once
#ifndef TORRENTTABLESORTMODEL_H
#define TORRENTTABLESORTMODEL_H

#include <QSortFilterProxyModel>

#include "torrentsqltablemodel.h"

class TorrentTableSortModel final : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentTableSortModel)

public:
    /*! Inherit constructors. */
    using QSortFilterProxyModel::QSortFilterProxyModel;

private:
    /*! Main comparison method. */
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const final;

    bool sortETA(
            const QModelIndex &left, const QModelIndex &right,
            const QVariant &leftRawValue, const QVariant &rightRawValue) const;
    bool sortSeedsAndLeechers(
            const QModelIndex &left, const QModelIndex &right,
            const QVariant &leftRawValue, const QVariant &rightRawValue) const;

    /*! Invoke lessThan() for a passed column, used in the second sort (recursion). */
    inline bool
    invokeLessThanForColumn(
            TorrentSqlTableModel::Column column,
            const QModelIndex &left, const QModelIndex &right) const;
};

bool TorrentTableSortModel::invokeLessThanForColumn( // NOLINT(misc-no-recursion)
        const TorrentSqlTableModel::Column column,
        const QModelIndex &left, const QModelIndex &right) const
{
    return lessThan(left.sibling(left.row(), column),
                    right.sibling(right.row(), column));
}

#endif // TORRENTTABLESORTMODEL_H
