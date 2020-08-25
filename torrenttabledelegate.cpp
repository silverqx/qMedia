#include "torrenttabledelegate.h"

#include <QApplication>
#include <QPainter>

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#include <QProxyStyle>
#endif

#include "torrentsqltablemodel.h"

TorrentTableDelegate::TorrentTableDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void TorrentTableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    // Selected item
    if ((index.column() != TorrentSqlTableModel::TR_PROGRESS) &&
        (option.state & QStyle::State_Selected))
        return paintSelectedItem(painter, option, index);

    // Default behavior
    if (index.column() != TorrentSqlTableModel::TR_PROGRESS)
        return QStyledItemDelegate::paint(painter, option, index);

    // Custom progressbar
    return paintProgressBar(painter, option, index);
}

void TorrentTableDelegate::paintSelectedItem(QPainter *painter, const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);

    itemOption.palette.setColor(QPalette::HighlightedText,
                                index.data(Qt::ForegroundRole).value<QColor>());

    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    painter->save();
    style->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter, widget);
    painter->restore();
}

void TorrentTableDelegate::paintProgressBar(QPainter *painter, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    QStyleOptionProgressBar newopt;
    newopt.rect = option.rect;
    newopt.text = index.data().toString();
    newopt.progress = static_cast<int>(index.data(TorrentSqlTableModel::UnderlyingDataRole).toReal() / 10);
    newopt.maximum = 100;
    newopt.minimum = 0;
    newopt.state = option.state;
    newopt.textVisible = true;
    // Setup color for progressbar
    auto progressBarColor = newopt.palette.color(QPalette::Highlight);
    // Remove transparency
    progressBarColor.setAlpha(255);
    newopt.palette.setColor(QPalette::Highlight, progressBarColor);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    QProxyStyle fusionStyle("fusion");
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

QWidget *TorrentTableDelegate::createEditor(QWidget *, const QStyleOptionViewItem &,
                                            const QModelIndex &) const
{
    return nullptr;
}
