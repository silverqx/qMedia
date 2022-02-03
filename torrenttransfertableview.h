#ifndef TORRENTTRANSFERTABLEVIEW_H
#define TORRENTTRANSFERTABLEVIEW_H

#include <QTableView>

class QSortFilterProxyModel;
class QSqlRecord;
class QSqlTableModel;

class StatusHash;
class TorrentTableDelegate;

class TorrentTransferTableView final : public QTableView
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentTransferTableView)

public:
    explicit TorrentTransferTableView(const HWND qBittorrentHwnd, QWidget *parent = nullptr);

    int getModelRowCount() const;
    inline bool isQBittorrentUp() const noexcept { return (m_qBittorrentHwnd != nullptr); };

public slots:
    void filterTextChanged(const QString &name) const;
    void reloadTorrentModel() const;
    void updateChangedTorrents(const QVector<QString> &torrentInfoHashes);
    void resizeColumns() const;
    inline void updateQBittorrentHwnd(const HWND hwnd) noexcept { m_qBittorrentHwnd = hwnd; };
    /*! Show/hide seeds/leechs column by isQBittorrentUp() */
    void togglePeerColumns();

protected:
    void showEvent(QShowEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // TODO duplicate enum, maps 1:1 to TorrentSqlTableModel::Column enum, may be enum using will be good too silverqx
    enum TorrentColumn
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

    QSharedPointer<const QVector<QSqlRecord>> selectTorrentFilesById(quint64 id) const;
    void displayListMenu(const QContextMenuEvent *const event);
    QModelIndex getSelectedTorrentIndex() const;
    QSqlRecord getSelectedTorrentRecord() const;
    void removeRecordFromTorrentFilesCache(quint64 torrentId) const;
    /*! Context menu action factory. */
    QAction *createActionForMenu(const QIcon &icon, const QString &text,
                                 const QKeySequence shortcut, const bool enabled,
                                 void (TorrentTransferTableView::*const slot)(),
                                 QWidget *parent = nullptr) const;
    /*! Context menu action factory ( overload ). */
    QAction *createActionForMenu(const QIcon &icon, const QString &text,
                                 const QKeySequence shortcut,
                                 void (TorrentTransferTableView::*const slot)(),
                                 QWidget *parent = nullptr) const;
    void resumeTorrent(const QSqlRecord &torrent) const;
    void pauseTorrent(const QSqlRecord &torrent) const;

    QSqlTableModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    TorrentTableDelegate *m_tableDelegate;
    /*! Contains torrent files by torrent id. */
    mutable QHash<quint64, QSharedPointer<const QVector<QSqlRecord>>> m_torrentFilesCache;
    bool m_showEventInitialized = false;
    HWND m_qBittorrentHwnd = nullptr;
    std::shared_ptr<StatusHash> m_statusHash;

private slots:
    void previewSelectedTorrent();
    void previewFile(const QString &filePath) const;
    void deleteSelectedTorrent();
    void showCsfdDetail();
    void showImdbDetail();
    void pauseSelectedTorrent();
    void resumeSelectedTorrent();
    void forceResumeSelectedTorrent();
    void forceRecheckSelectedTorrent();
    void openFolderForSelectedTorrent();
    void pauseResumeSelectedTorrent() const;
};

#endif // TORRENTTRANSFERTABLEVIEW_H
