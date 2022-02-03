#include "torrenttabledelegate.h"

#include <QApplication>
#include <QPainter>

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#include <QProxyStyle>
#endif

#include "macros/likely.h"
#include "torrentsqltablemodel.h"

void TorrentTableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    // Selected item
    if (index.column() != TorrentSqlTableModel::TR_PROGRESS &&
        option.state.testFlag(QStyle::State_Selected)
    )
        paintSelectedItem(painter, option, index);

    // Default behavior
    else if (index.column() != TorrentSqlTableModel::TR_PROGRESS) T_LIKELY
        QStyledItemDelegate::paint(painter, option, index);

    // Custom progressbar
    else
        paintProgressBar(painter, option, index);
}

void TorrentTableDelegate::paintSelectedItem(
        QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);

    itemOption.palette.setColor(QPalette::HighlightedText,
                                index.data(Qt::ForegroundRole).value<QColor>());

    const auto *const widget = option.widget;
    const auto *const style =
            widget != nullptr ? widget->style()
                              : QApplication::style();

    painter->save();
    style->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter, widget);
    painter->restore();
}

void TorrentTableDelegate::paintProgressBar(
        QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const
{
    QStyleOptionProgressBar newopt;
    newopt.rect = option.rect;
    newopt.text = index.data().toString();
    // UnderlyingDataRole contains a raw value from db
    newopt.progress =
            static_cast<int>(
                index.data(TorrentSqlTableModel::UnderlyingDataRole).toReal() / 10);
    newopt.maximum = 100;
    newopt.minimum = 0;
    newopt.state = option.state;
    newopt.textVisible = true;
    // Progressbar color
    QColor progressBarColor(54, 115, 157, 235);
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

QWidget *TorrentTableDelegate::createEditor(
        QWidget */*unused*/, const QStyleOptionViewItem &/*unused*/,
        const QModelIndex &/*unused*/) const
{
    // No editor here
    return nullptr;
}
