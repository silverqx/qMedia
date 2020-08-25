#ifndef TORRENTTABLEDELEGATE_H
#define TORRENTTABLEDELEGATE_H

#include <QStyledItemDelegate>

class TorrentTableDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentTableDelegate)

public:
    explicit TorrentTableDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
    void paintSelectedItem(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintProgressBar(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // TORRENTTABLEDELEGATE_H
