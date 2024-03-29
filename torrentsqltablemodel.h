#pragma once
#ifndef TORRENTSQLTABLEMODEL_H
#define TORRENTSQLTABLEMODEL_H

#include <QSqlTableModel>

class StatusHash;
class TorrentTransferTableView;

class TorrentSqlTableModel final : public QSqlTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentSqlTableModel)

public:
    enum Column
    {
        TR_ID,
        TR_NAME,
        TR_PROGRESS,
        TR_ETA,
        TR_SIZE,
        TR_SEEDS,
        TR_TOTAL_SEEDS,
        TR_LEECHERS,
        TR_TOTAL_LEECHERS,
        TR_AMOUNT_LEFT,
        TR_ADDED_ON,
        TR_HASH,
        TR_CSFD_MOVIE_DETAIL,
        TR_STATUS,
        TR_MOVIE_DETAIL_INDEX,
        TR_SAVE_PATH,

        NB_COLUMNS
    };

    enum DataRole
    {
        UnderlyingDataRole = Qt::UserRole,
    };

    explicit TorrentSqlTableModel(
            TorrentTransferTableView *parent = nullptr,
            QSqlDatabase db = QSqlDatabase());

    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const final;
    /*! Returns the data for the given role and section in the header, used to modify
        an initial sort order. */
    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const final;

    int getTorrentRowByInfoHash(const QString &infoHash) const;
    quint64 getTorrentIdByInfoHash(const QString &infoHash) const;
    // TODO implement rowCount() and columnCount() and may be some others, look qbt model class as reference silverqx

    /*! Map seeds/leechers column to total seeds/leechers counterparts. */
    static const QHash<int, int> &mapPeersToTotal();

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
    bool select() final;

private:
    QString displayValue(const QModelIndex &modelIndex, int column) const;
    void createInfoHashToRowTorrentMap();

    /*! Map a torrent info hash to the table row. */
    QHash<QString, int> m_torrentMap;
    /*! Map a torrent info hash to the torrent id. */
    QHash<QString, quint64> m_torrentIdMap;
    const TorrentTransferTableView *const m_torrentTableView;
    std::shared_ptr<StatusHash> m_statusHash;
};

#endif // TORRENTSQLTABLEMODEL_H
