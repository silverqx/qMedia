#ifndef TORRENTTRANSFERTABLEVIEW_H
#define TORRENTTRANSFERTABLEVIEW_H

#include <QTableView>

class QSortFilterProxyModel;
class QSqlRecord;
class QSqlTableModel;

class TorrentTableDelegate;

class TorrentTransferTableView final : public QTableView
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentTransferTableView)

public:
    explicit TorrentTransferTableView(const HWND qBittorrentHwnd, QWidget *parent = nullptr);
    ~TorrentTransferTableView();

    int getModelRowCount() const;

public slots:
    void filterTextChanged(const QString &name);
    void reloadTorrentModel();
    void updateChangedTorrents(const QVector<QString> &torrentInfoHashes);

protected:
    void showEvent(QShowEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    enum TorrentColumn
    {
        TR_ID,
        TR_NAME,
        TR_PROGRESS,
        TR_ETA,
        TR_SIZE,
        TR_AMOUNT_LEFT,
        TR_ADDED_ON,
        TR_HASH,

        NB_COLUMNS
    };

    QVector<QSqlRecord> *selectTorrentFilesById(quint64 id);
    void displayListMenu(const QContextMenuEvent *const event);
    QModelIndex getSelectedTorrentIndex() const;
    QSqlRecord getSelectedTorrentRecord() const;
    void removeRecordFromTorrentFilesCache(quint64 torrentId);

    QSqlTableModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    TorrentTableDelegate *m_tableDelegate;
    /*!
       \brief Contains torrent files by torrent id.
     */
    QHash<quint64, QVector<QSqlRecord> *> m_torrentFilesCache;
    bool m_showEventInitialized = false;
    HWND m_qBittorrentHwnd = nullptr;

private slots:
    void previewSelectedTorrent();
    void previewFile(const QString &filePath);
    void deleteSelectedTorrent();
    void showCsfdDetail();
    void showImdbDetail();
};

#endif // TORRENTTRANSFERTABLEVIEW_H
