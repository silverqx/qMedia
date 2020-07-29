#include "torrenttabledelegate.h"

#include <QPainter>

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#include <QProxyStyle>
#else
#include <QApplication>
#endif

#include "torrentsqltablemodel.h"

TorrentTableDelegate::TorrentTableDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void TorrentTableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != TorrentSqlTableModel::TR_PROGRESS)
        return QStyledItemDelegate::paint(painter, option, index);

    QStyleOptionProgressBar newopt;
    newopt.rect = option.rect;
    newopt.text = index.data().toString();
    newopt.progress = static_cast<int>(index.data(TorrentSqlTableModel::UnderlyingDataRole).toReal() / 10);
    newopt.maximum = 100;
    newopt.minimum = 0;
    newopt.state = option.state;
    newopt.textVisible = true;

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    QProxyStyle fusionStyle {"fusion"};
    QStyle *style = &fusionStyle;
#else
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
#endif

    painter->save();
    // Draw progressbar without border / groove
    style->drawControl(QStyle::CE_ProgressBarContents, &newopt, painter);
    style->drawControl(QStyle::CE_ProgressBarLabel, &newopt, painter);
    painter->restore();
}

QWidget *TorrentTableDelegate::createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const
{
    return nullptr;
}
