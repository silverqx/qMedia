#pragma once
#ifndef PREVIEWLISTDELEGATE_H
#define PREVIEWLISTDELEGATE_H

#include <QItemDelegate>

class PreviewListDelegate final : public QItemDelegate
{
    Q_OBJECT
    Q_DISABLE_COPY(PreviewListDelegate)

public:
    /*! Inherit constructors. */
    using QItemDelegate::QItemDelegate;

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const final;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const final;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const final;
};

#endif // PREVIEWLISTDELEGATE_H
