#ifndef PREVIEWLISTDELEGATE_H
#define PREVIEWLISTDELEGATE_H

#include <QItemDelegate>

class PreviewListDelegate final : public QItemDelegate
{
    Q_OBJECT
    Q_DISABLE_COPY(PreviewListDelegate)

public:
    explicit PreviewListDelegate(QObject *parent = nullptr);

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // PREVIEWLISTDELEGATE_H
