#pragma once
#ifndef TORRENTTABLEDELEGATE_H
#define TORRENTTABLEDELEGATE_H

#include <QStyledItemDelegate>

class TorrentTableDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentTableDelegate)

public:
    /*! Inherit constructors. */
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const final;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const final;

private:
    void paintSelectedItem(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    void paintProgressBar(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
};

#endif // TORRENTTABLEDELEGATE_H
