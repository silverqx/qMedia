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
    inline bool isQBittorrentUp() const { return (m_qBittorrentHwnd != nullptr); };

public slots:
    void filterTextChanged(const QString &name);
    void reloadTorrentModel();
    void updateChangedTorrents(const QVector<QString> &torrentInfoHashes);
    void updateQBittorrentHwnd(const HWND hwnd);

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

    QVector<QSqlRecord> *selectTorrentFilesById(quint64 id);
    void displayListMenu(const QContextMenuEvent *const event);
    QModelIndex getSelectedTorrentIndex() const;
    QSqlRecord getSelectedTorrentRecord() const;
    void removeRecordFromTorrentFilesCache(quint64 torrentId);
    void resizeColumns();
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
    void resumeTorrent(const QSqlRecord &torrent);
    void pauseTorrent(const QSqlRecord &torrent);

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
    // TODO use const slots where appropriate silverqx
    void previewSelectedTorrent();
    void previewFile(const QString &filePath);
    void deleteSelectedTorrent();
    void showCsfdDetail();
    void showImdbDetail();
    void pauseSelectedTorrent();
    void resumeSelectedTorrent();
    void forceResumeSelectedTorrent();
    void forceRecheckSelectedTorrent();
    void openFolderForSelectedTorrent();
    void pauseResumeSelectedTorrent();
};

#endif // TORRENTTRANSFERTABLEVIEW_H
