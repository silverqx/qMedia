#include "torrentsqltablemodel.h"

#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QLocale>
#include <QSqlField>
#include <QSqlRecord>
#include <QTableView>

#include "common.h"
#include "torrentstatus.h"
#include "torrenttransfertableview.h"
#include "utils/misc.h"

const QHash<int, int> TorrentSqlTableModel::mapToTotal = {
    {TorrentSqlTableModel::TR_SEEDS,    TorrentSqlTableModel::TR_TOTAL_SEEDS},
    {TorrentSqlTableModel::TR_LEECHERS, TorrentSqlTableModel::TR_TOTAL_LEECHERS},
};

TorrentSqlTableModel::TorrentSqlTableModel(TorrentTransferTableView *parent,
                                           const QSqlDatabase db)
    : QSqlTableModel(parent, db)
    , m_torrentTableView(parent)
    , m_statusHash(StatusHash::instance())
{}

QVariant TorrentSqlTableModel::data(const QModelIndex &modelIndex,
                                    const int role) const
{
    if (!modelIndex.isValid())
        return {};

    const auto amountString = [](const auto value, const auto total)
    {
        return ((value == 0) && (total == 0) && ::HIDE_ZERO_VALUES)
                ? QString {}
                : QString::number(value) + " (" + QString::number(total) + ')';
    };

    const auto column = modelIndex.column();
    const auto row = modelIndex.row();

    const auto getTooltipForNameColumn = [this, &modelIndex, &column, &row]()
    {
        // Relative cursor position from left side of TorrentTransferTableView
        const auto postitionX =
                (dynamic_cast<QTableView *>(parent())->
                horizontalHeader()->mapFromGlobal(QCursor::pos())).x();
        // TODO get decorationSize from QStyledItemDelegate ( see NOTES.txt ) silverqx
        static const auto iconWidth = 24;
        if (postitionX <= iconWidth)
            return (*m_statusHash)[record(row).value("status").toString()].text;

        return displayValue(modelIndex, column);
    };

    const auto getPeersTooltip = [this, &row, &amountString]()
    {
        // Compute peers count
        const auto peers = record(row).value("leechers").toInt() +
                           record(row).value("seeds").toInt();
        const auto peersTotal = record(row).value("total_leechers").toInt() +
                                record(row).value("total_seeds").toInt();
        return QStringLiteral("Peers: %1").arg(amountString(peers, peersTotal));
    };

    switch (role) {
    case Qt::ForegroundRole:
        return (*m_statusHash)[record(row).value("status").toString()].getColor();
    case Qt::DisplayRole:
        return displayValue(modelIndex, column);
    case UnderlyingDataRole:
        return record(row).value(column);
    case Qt::TextAlignmentRole:
        switch (column) {
        case TR_ID:
        case TR_PROGRESS:
            return QVariant {Qt::AlignCenter};
        case TR_ETA:
        case TR_SIZE:
        case TR_SEEDS:
        case TR_LEECHERS:
        case TR_AMOUNT_LEFT:
        case TR_ADDED_ON:
        case TR_HASH:
            return QVariant {Qt::AlignRight | Qt::AlignVCenter};
        }
        break;
    case Qt::FontRole:
        switch (column) {
        case TR_NAME:
            // Increase font size for name column
            auto font = dynamic_cast<QTableView *>(parent())->font();
            font.setPointSize(12);
            return font;
        }
        break;
    case Qt::DecorationRole:
        switch (column) {
        case TR_NAME:
            return (*m_statusHash)[record(row).value("status").toString()]
                    .getIcon();
        }
        break;
    case Qt::ToolTipRole:
        switch (column) {
        case TR_NAME:
            return getTooltipForNameColumn();
        case TR_SEEDS:
        case TR_LEECHERS:
            return getPeersTooltip();
        }
        break;
    }

    return QSqlTableModel::data(modelIndex, role);
}

int TorrentSqlTableModel::getTorrentRowByInfoHash(const QString &infoHash) const {
    if (!m_torrentMap.contains(infoHash)) {
        qDebug() << "Torrent with this info hash doesn't exist:"
                 << infoHash;
        return -1;
    }

    return m_torrentMap[infoHash];
}

quint64 TorrentSqlTableModel::getTorrentIdByInfoHash(const QString &infoHash) const
{
    // TODO investigate return value 0 and alternatives like exception and similar, or nullable type, std::optional is right solution, leaving it for now, may be refactor later silverqx
    if (!m_torrentIdMap.contains(infoHash)) {
        qDebug() << "Torrent with this info hash doesn't exist:"
                 << infoHash;
        return 0;
    }

    return m_torrentIdMap[infoHash];
}

bool TorrentSqlTableModel::select()
{
    bool retVal = QSqlTableModel::select();
    createInfoHashToRowTorrentMap();
    return retVal;
}

QString TorrentSqlTableModel::displayValue(const QModelIndex &modelIndex,
                                           const int column) const
{
    const auto unitString = [](const qint64 value, const bool isSpeedUnit = false) -> QString
    {
        return ((value == 0) && ::HIDE_ZERO_VALUES)
            ? QString {} : Utils::Misc::friendlyUnit(value, isSpeedUnit);
    };

    const auto amountString = [](const auto value, const auto total)
    {
        return ((value == 0) && (total == 0) && ::HIDE_ZERO_VALUES)
                ? QString {}
                : QString::number(value) + " (" + QString::number(total) + ')';
    };

    // Get value from underlying model
    const auto row = modelIndex.row();
    const QVariant rawData = record(row).value(column);
    if (!rawData.isValid() || rawData.isNull())
        return {};

    switch (column) {
    case TR_ID:
    case TR_NAME:
    case TR_HASH:
        return rawData.toString();
    case TR_PROGRESS:
        return QString::number(rawData.toReal() / 10) + '%';
    case TR_ETA:
        // If qBittorrent is not running, show ∞ for every torrent
        if (!m_torrentTableView->isQBittorrentUp())
            return QString::fromUtf8(::C_INFINITY);
        return Utils::Misc::userFriendlyDuration(rawData.toLongLong(), ::MAX_ETA);
    case TR_ADDED_ON:
        return QLocale::system().toString(rawData.toDateTime(), QLocale::ShortFormat);
    case TR_SIZE:
    case TR_AMOUNT_LEFT:
        return unitString(rawData.toLongLong());
    case TR_SEEDS:
    case TR_LEECHERS:
        return amountString(rawData.toInt(), record(row).value(mapToTotal.value(column)).toInt());
    }

    return {};
}

void TorrentSqlTableModel::createInfoHashToRowTorrentMap()
{
    for (int i = 0; i < rowCount() ; ++i) {
        const QString torrentHash = data(index(i, TR_HASH), UnderlyingDataRole)
                                    .toString();
        m_torrentMap[torrentHash] = i;
        m_torrentIdMap[torrentHash] = data(index(i, TR_ID), UnderlyingDataRole)
                                      .toULongLong();
    }
}
