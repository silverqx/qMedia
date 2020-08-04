#include "torrentsqltablemodel.h"

#include <QDateTime>
#include <QDebug>
#include <QLocale>
#include <QSqlField>
#include <QSqlRecord>

#include "common.h"
#include "mainwindow.h"
#include "utils/misc.h"

TorrentSqlTableModel::TorrentSqlTableModel(QObject *parent, const QSqlDatabase db)
    : QSqlTableModel(parent, db)
{}

QVariant TorrentSqlTableModel::data(const QModelIndex &idx, const int role) const
{
    if (!idx.isValid())
        return {};

    const int column = idx.column();

    switch (role) {
    case Qt::DisplayRole:
        return displayValue(idx, column);
    case UnderlyingDataRole:
        return record(idx.row()).value(column);
    case Qt::TextAlignmentRole:
        switch (column) {
        case TR_ID:
        case TR_PROGRESS:
            return QVariant {Qt::AlignCenter};
        case TR_ETA:
        case TR_SIZE:
        case TR_AMOUNT_LEFT:
        case TR_ADDED_ON:
        case TR_HASH:
            return QVariant {Qt::AlignRight | Qt::AlignVCenter};
        }
        break;
    case Qt::ToolTipRole:
        switch (column) {
        case TR_NAME:
            return displayValue(idx, column);
        }
        break;
    }

    return QSqlTableModel::data(idx, role);
}

int TorrentSqlTableModel::getTorrentRowByInfoHash(const QString &infoHash) {
    if (!m_torrentMap.contains(infoHash)) {
        qDebug() << "Torrent with this info hash doesn't exist:" << infoHash;
        return -1;
    }

    return m_torrentMap[infoHash];
}

bool TorrentSqlTableModel::select()
{
    bool retVal = QSqlTableModel::select();
    createInfoHashToRowTorrentMap();
    return retVal;
}

QString TorrentSqlTableModel::displayValue(const QModelIndex &modelIndex, const int column) const
{
    bool hideValues = false;
    const auto unitString = [hideValues](const qint64 value, const bool isSpeedUnit = false) -> QString
    {
        return ((value == 0) && hideValues)
            ? QString {} : Utils::Misc::friendlyUnit(value, isSpeedUnit);
    };

    // Get value from underlying model
    const QVariant rawData = record(modelIndex.row()).value(column);
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
        if (dynamic_cast<const MainWindow *const>(parent())->getQBittorrentHwnd() == nullptr)
            return QString::fromUtf8(C_INFINITY);
        return Utils::Misc::userFriendlyDuration(rawData.toLongLong(), MAX_ETA);
    case TR_ADDED_ON:
        return QLocale::system().toString(rawData.toDateTime(), QLocale::ShortFormat);
    case TR_SIZE:
    case TR_AMOUNT_LEFT:
        return unitString(rawData.toLongLong());
    }

    return {};
}

void TorrentSqlTableModel::createInfoHashToRowTorrentMap()
{
    for (int i = 0; i < rowCount() ; ++i) {
        const QString torrentHash = data(index(i, TR_HASH), UnderlyingDataRole).toString();
        m_torrentMap[torrentHash] = i;
    }
}
