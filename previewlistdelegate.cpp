#include "previewlistdelegate.h"

#include <QPainter>
#include <QStyleOptionProgressBar>

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#include <QProxyStyle>
#else
#include <QApplication>
#endif

#include "previewselectdialog.h"
#include "utils/misc.h"
#include "utils/string.h"

void PreviewListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    painter->save();

    QStyleOptionViewItem opt = QItemDelegate::setOptions(index, option);
    drawBackground(painter, opt, index);

    switch (index.column()) {
    case PreviewSelectDialog::TR_SIZE:
        opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
        QItemDelegate::drawDisplay(painter, opt, option.rect,
                                   Utils::Misc::friendlyUnit(index.data().toLongLong()));
        break;

    case PreviewSelectDialog::TR_PROGRESS: {
        const auto progress = index.data().toReal() / 10;
        const auto progressText =
                progress == 100 ? QStringLiteral("100")
                                : Utils::String::fromDouble(progress, 1);

        QStyleOptionProgressBar newopt;
        newopt.rect = opt.rect;
        newopt.text = QStringLiteral("%1%").arg(progressText);
        newopt.progress = static_cast<int>(progress);
        newopt.maximum = 100;
        newopt.minimum = 0;
        newopt.textVisible = true;
        // Progressbar color
        QColor progressBarColor(54, 115, 157, 235);
        newopt.palette.setColor(QPalette::Highlight, progressBarColor);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        QProxyStyle fusionStyle("fusion");
        const QStyle *const style = &fusionStyle;
#else
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
#endif
        // Draw progressbar without border / groove
        style->drawControl(QStyle::CE_ProgressBarContents, &newopt, painter);
        style->drawControl(QStyle::CE_ProgressBarLabel, &newopt, painter);
        break;
    }

    default:
        QItemDelegate::paint(painter, option, index);
    }

    painter->restore();
}

QWidget *PreviewListDelegate::createEditor(
        QWidget */*unused*/, const QStyleOptionViewItem &/*unused*/,
        const QModelIndex &/*unused*/) const
{
    // No editor here
    return nullptr;
}

QSize PreviewListDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    // Increase line size to 28px, looks much nicer
    auto size = QItemDelegate::sizeHint(option, index);

    size.setHeight(28);

    return size;
}
