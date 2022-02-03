#pragma once
#ifndef TORRENTTABLESORTMODEL_H
#define TORRENTTABLESORTMODEL_H

#include <QSortFilterProxyModel>

class TorrentTableSortModel final : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentTableSortModel)

public:
    explicit TorrentTableSortModel(QObject *parent = nullptr);

private:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // TORRENTTABLESORTMODEL_H
