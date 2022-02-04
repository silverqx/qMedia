#pragma once
#ifndef TORRENTTRANSFERTABLEVIEW_H
#define TORRENTTRANSFERTABLEVIEW_H

#include <QAction>
#include <QPointer>
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
    explicit TorrentTransferTableView(HWND qBittorrentHwnd, QWidget *parent = nullptr);

    int getModelRowCount() const;
    inline bool isqBittorrentUp() const noexcept;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
    void filterTextChanged(const QString &name) const;
    void reloadTorrentModel() const;
    void updateChangedTorrents(const QVector<QString> &torrentInfoHashes);
    void resizeColumns() const;
    inline void updateqBittorrentHwnd(HWND hwnd) noexcept;
    /*! Show/hide seeds/leechs column by isqBittorrentUp() */
    void togglePeerColumns();

protected:
    void showEvent(QShowEvent *event) final;
    void contextMenuEvent(QContextMenuEvent *event) final;

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
    void displayListMenu(const QContextMenuEvent *event);
    QModelIndex getSelectedTorrentIndex() const;
    QSqlRecord getSelectedTorrentRecord() const;
    void removeRecordFromTorrentFilesCache(quint64 torrentId) const;

    /*! Context menu action factory. */
    template<typename Func>
    QAction *createActionForMenu(
            const QIcon &icon, const QString &text, QKeySequence &&shortcut,
            Func &&slot, QWidget *parent = nullptr, bool enabled = true) const;

    void resumeTorrent(const QSqlRecord &torrent) const;
    void pauseTorrent(const QSqlRecord &torrent) const;

    QPointer<QSqlTableModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxyModel;
    QPointer<TorrentTableDelegate> m_tableDelegate;
    /*! Contains torrent files by torrent id. */
    mutable QHash<quint64, QSharedPointer<const QVector<QSqlRecord>>> m_torrentFilesCache;
    bool m_showEventInitialized = false;
    HWND m_qBittorrentHwnd = nullptr;
    std::shared_ptr<StatusHash> m_statusHash;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
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

bool TorrentTransferTableView::isqBittorrentUp() const noexcept
{
    return m_qBittorrentHwnd != nullptr;
}

void TorrentTransferTableView::updateqBittorrentHwnd(HWND hwnd) noexcept
{
    m_qBittorrentHwnd = hwnd;
}

template<typename Func>
QAction *TorrentTransferTableView::createActionForMenu(
        const QIcon &icon, const QString &text, QKeySequence &&shortcut,
        Func &&slot, QWidget *parent, const bool enabled) const
{
    auto *const newAction = new QAction(icon, text, parent);
    // TODO would be ideal to show this shortcut's text little grey silverqx
    newAction->setShortcut(shortcut);
    newAction->setEnabled(enabled);

    connect(newAction, &QAction::triggered, this, std::forward<Func>(slot));

    return newAction;
}

#endif // TORRENTTRANSFERTABLEVIEW_H
