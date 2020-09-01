#include "torrenttransfertableview.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <qt_windows.h>

#include "common.h"
#include "csfddetailservice.h"
#include "moviedetaildialog.h"
#include "previewselectdialog.h"
#include "torrentsqltablemodel.h"
#include "torrenttabledelegate.h"
#include "torrenttablesortmodel.h"
#include "utils/gui.h"

TorrentTransferTableView::TorrentTransferTableView(const HWND qBittorrentHwnd, QWidget *parent)
    : QTableView(parent)
    , m_qBittorrentHwnd(qBittorrentHwnd)
{
    // QObject
    setObjectName(QStringLiteral("torrentTransferTableView"));
    // QWidget
    // Set font size bigger by 1pt
    QFont currentFont = font();
    currentFont.setFamily("Arial");
    currentFont.setPointSize(10);
    setFont(currentFont);
    // QAbstractItemView
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setTabKeyNavigation(false);
    setDropIndicatorShown(true);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setTextElideMode(Qt::ElideRight);
    // QTableView
    setSortingEnabled(true);
    setWordWrap(true);
    setCornerButtonEnabled(false);
    // Horizontal / Vertical Header
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(false);
    verticalHeader()->setVisible(false);
    verticalHeader()->setMinimumSectionSize(32);
    verticalHeader()->setDefaultSectionSize(36);
    verticalHeader()->setHighlightSections(false);
    verticalHeader()->setStretchLastSection(false);

    // Create and apply delegate
    m_tableDelegate = new TorrentTableDelegate(this);
    setItemDelegate(m_tableDelegate);

    // Create models
    m_model = new TorrentSqlTableModel(this);
    m_model->setTable(QStringLiteral("torrents"));
    m_model->setEditStrategy(QSqlTableModel::OnRowChange);
    m_model->select();
    m_model->setHeaderData(TR_ID, Qt::Horizontal, QStringLiteral("Id"));
    m_model->setHeaderData(TR_NAME, Qt::Horizontal, QStringLiteral("Name"));
    m_model->setHeaderData(TR_PROGRESS, Qt::Horizontal, QStringLiteral("Done"));
    m_model->setHeaderData(TR_ETA, Qt::Horizontal, QStringLiteral("ETA"));
    m_model->setHeaderData(TR_SIZE, Qt::Horizontal, QStringLiteral("Size"));
    m_model->setHeaderData(TR_AMOUNT_LEFT, Qt::Horizontal, QStringLiteral("Remaining"));
    m_model->setHeaderData(TR_ADDED_ON, Qt::Horizontal, QStringLiteral("Added on"));

    m_proxyModel = new TorrentTableSortModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(TR_NAME);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    CsfdDetailService::initInstance(dynamic_cast<TorrentSqlTableModel *>(m_model));

    setModel(m_proxyModel);
    hideColumn(TR_ID);
    hideColumn(TR_HASH);
    hideColumn(TR_CSFD_MOVIE_DETAIL);
    hideColumn(TR_STATUS);
    hideColumn(TR_MOVIE_DETAIL_INDEX);
    sortByColumn(TR_ADDED_ON, Qt::DescendingOrder);

    // Init torrent context menu
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // Connect events
    connect(this, &QAbstractItemView::doubleClicked, this, &TorrentTransferTableView::previewSelectedTorrent);

    // Hotkeys
    // torrentTransferTableView
    const auto *doubleClickHotkeyReturn = new QShortcut(Qt::Key_Return, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyReturn, &QShortcut::activated, this, &TorrentTransferTableView::previewSelectedTorrent);
    const auto *doubleClickHotkeyReturnAlt = new QShortcut(Qt::SHIFT + Qt::Key_Return, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyReturnAlt, &QShortcut::activated, this, &TorrentTransferTableView::showCsfdDetail);
    const auto *doubleClickHotkeyEnter = new QShortcut(Qt::Key_Enter, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyEnter, &QShortcut::activated, this, &TorrentTransferTableView::previewSelectedTorrent);
    const auto *doubleClickHotkeyF3 = new QShortcut(Qt::Key_F3, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF3, &QShortcut::activated, this, &TorrentTransferTableView::previewSelectedTorrent);
    const auto *doubleClickHotkeyDelete = new QShortcut(Qt::Key_Delete, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyDelete, &QShortcut::activated, this, &TorrentTransferTableView::deleteSelectedTorrent);
    const auto *doubleClickHotkeyF4 = new QShortcut(Qt::Key_F4, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF4, &QShortcut::activated, this, &TorrentTransferTableView::showCsfdDetail);
    const auto *doubleClickHotkeyF6 = new QShortcut(Qt::Key_F6, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF6, &QShortcut::activated, this, &TorrentTransferTableView::showImdbDetail);

    // Resize columns to default state after right click
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, [this]()
    {
        m_showEventInitialized = false;
        resizeColumns();
    });

    // Select torrent with latest / highest added on datetime
    selectionModel()->select(m_model->index(0, TR_NAME),
                             QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

TorrentTransferTableView::~TorrentTransferTableView()
{
    foreach (auto torrentFiles, m_torrentFilesCache)
        delete torrentFiles;
}

void TorrentTransferTableView::showEvent(QShowEvent *event)
{
    // Event originated from system
    if (event->spontaneous())
        return QTableView::showEvent(event);

    return resizeColumns();
}

void TorrentTransferTableView::resizeColumns()
{
    if (m_showEventInitialized)
        return;

    // Move this as top as possible to prevent race condition
    m_showEventInitialized = true;

    // Pixel perfectly sized torrentTransferTableView header
    // Set Name column width to all remaining area
    // Have to be called after show(), because torrentTransferTableView width is needed
    QHeaderView *const tableViewHeader = horizontalHeader();
    tableViewHeader->resizeSections(QHeaderView::ResizeToContents);

    // Increase progress section size about 10%
    const int sizeSize = tableViewHeader->sectionSize(TR_SIZE);
    tableViewHeader->resizeSection(TR_SIZE, sizeSize + (sizeSize * 0.1));
    // Increase size section size about 40%
    const int progressSize = tableViewHeader->sectionSize(TR_PROGRESS);
    tableViewHeader->resizeSection(TR_PROGRESS, progressSize + (progressSize * 0.4));
    // Increase ETA section size about 30%
    const int etaSize = tableViewHeader->sectionSize(TR_ETA);
    tableViewHeader->resizeSection(TR_ETA, etaSize + (etaSize * 0.3));
    // Remaining section do not need resize
    // Increase added on section size about 10%
    const int addedOnSize = tableViewHeader->sectionSize(TR_ADDED_ON);
    tableViewHeader->resizeSection(TR_ADDED_ON, addedOnSize + (addedOnSize * 0.1));

    // TODO also set minimum size for each section, based on above computed sizes silverqx

    // Compute name section size
    int nameColWidth = width();
    const QScrollBar *const vScrollBar = verticalScrollBar();
    if (vScrollBar->isVisible())
        nameColWidth -= vScrollBar->width();
    nameColWidth -= tableViewHeader->sectionSize(TR_PROGRESS);
    nameColWidth -= tableViewHeader->sectionSize(TR_ETA);
    nameColWidth -= tableViewHeader->sectionSize(TR_SIZE);
    nameColWidth -= tableViewHeader->sectionSize(TR_AMOUNT_LEFT);
    nameColWidth -= tableViewHeader->sectionSize(TR_ADDED_ON);
    nameColWidth -= 2; // Borders

    // TODO at the end set mainwindow width and height based on torrentTransferTableView ( inteligently 😎 ) silverqx

    tableViewHeader->resizeSection(TR_NAME, nameColWidth);
    horizontalHeader()->setStretchLastSection(true);

    // Finally, I like this
    tableViewHeader->setSectionResizeMode(QHeaderView::Interactive);
    tableViewHeader->setCascadingSectionResizes(true);
}

void TorrentTransferTableView::contextMenuEvent(QContextMenuEvent *event)
{
    if (event->reason() == QContextMenuEvent::Keyboard)
        qDebug() << "Torrent table context menu reason : keyboard";

    displayListMenu(event);
}

int TorrentTransferTableView::getModelRowCount() const
{
    return m_model->rowCount();
}

void TorrentTransferTableView::previewFile(const QString &filePath)
{
    Utils::Gui::openPath(filePath);
}

QVector<QSqlRecord> *TorrentTransferTableView::selectTorrentFilesById(quint64 id)
{
    // Return from cache
    if (m_torrentFilesCache.contains(id))
        return m_torrentFilesCache.value(id);

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare("SELECT * FROM torrents_previewable_files WHERE torrent_id = ?");
    query.addBindValue(id);

    const bool ok = query.exec();
    if (!ok) {
        qDebug() << QString("Select of torrent(ID%1) files failed :").arg(id)
                 << query.lastError().text();
        return {};
    }

    QVector<QSqlRecord> *torrentFiles = new QVector<QSqlRecord>;
    while (query.next())
        torrentFiles->append(query.record());

    if (torrentFiles->isEmpty()) {
        qDebug() << QString("No torrent files in DB for torrent(ID%1), this should never happen :/")
                    .arg(id);
        return torrentFiles;
    }

    m_torrentFilesCache.insert(id, torrentFiles);

    return torrentFiles;
}

QModelIndex TorrentTransferTableView::getSelectedTorrentIndex() const
{
    QModelIndexList selectedIndexes = selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return {};
    QModelIndex selectedIndex = selectedIndexes.first();
    if (!selectedIndex.isValid())
        return {};

    return selectedIndex;
}

QSqlRecord TorrentTransferTableView::getSelectedTorrentRecord() const
{
    QModelIndex selectedIndex = getSelectedTorrentIndex();
    if (!selectedIndex.isValid())
        return {};

    return m_model->record(
        m_proxyModel->mapToSource(selectedIndex).row()
    );
}

void TorrentTransferTableView::removeRecordFromTorrentFilesCache(const quint64 torrentId)
{
    if (!m_torrentFilesCache.contains(torrentId)) {
        qDebug() << "Torrent files cache doesn't contain torrent :" << torrentId;
        return;
    }

    const auto torrentFiles = m_torrentFilesCache[torrentId];
    m_torrentFilesCache.remove(torrentId);
    delete torrentFiles;
}

void TorrentTransferTableView::filterTextChanged(const QString &name)
{
    m_proxyModel->setFilterRegExp(
        // TODO port to QRegularExpression silverqx
        QRegExp(name, Qt::CaseInsensitive, QRegExp::WildcardUnix));
}

void TorrentTransferTableView::previewSelectedTorrent()
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Torrent doubleclicked :" << torrent.value("name").toString();

    const QVector<QSqlRecord> *const torrentFiles = selectTorrentFilesById(torrent.value("id").toULongLong());
    if (torrentFiles->isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Preview impossible"),
                              QStringLiteral("Torrent <strong>%1</strong> does not contain any previewable files.")
                              .arg(torrent.value("name").toString()));
        return;
    }

    // If torrent contains only one file, do not show preview dialog
    if (torrentFiles->size() == 1) {
        previewFile(torrentFiles->first().value("filepath").toString());
        return;
    }

    auto *const dialog = new PreviewSelectDialog(this, torrent, torrentFiles);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &PreviewSelectDialog::readyToPreviewFile, this, &TorrentTransferTableView::previewFile);
    dialog->show();
    // TODO set current selection to selected model, because after preview dialog is closed, current selection is at 0,0 silverqx
}

void TorrentTransferTableView::reloadTorrentModel()
{
    // TODO remember selected torrent by InfoHash, not by row silverqx
    // Remember currently selected row or first row, if was nothing selected
    int selectRow = 0;
    auto selectedRows = selectionModel()->selectedRows();
    if (!selectedRows.isEmpty())
        selectRow = selectedRows.first().row();

    // Reload model
    m_model->select();
    qInfo() << "Torrent model reloaded";

    // This situation can occur, when torrents was deleted and was selected one of the last rows
    const int rowCount = m_model->rowCount();
    if (selectRow > rowCount)
        selectRow = rowCount;

    // Reselect remembered row
    // TODO currentSelection rectangle is not showed silverqx
    selectionModel()->setCurrentIndex(m_model->index(selectRow, TR_NAME),
                                      QItemSelectionModel::SelectCurrent |
                                      QItemSelectionModel::Rows);
}

void TorrentTransferTableView::displayListMenu(const QContextMenuEvent *const event)
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Do not displaying context menu, because any torrent is selected.";
        return;
    }

    auto *listMenu = new QMenu(this);
    listMenu->setAttribute(Qt::WA_DeleteOnClose);

    // Create actions
    auto *actionShowCsfdDetail = new QAction(QIcon(":/icons/csfd_w.svg"), QStringLiteral("Show &csfd detail..."), listMenu);
    // TODO would be ideal to show this shortcut's text little grey silverqx
    actionShowCsfdDetail->setShortcut(Qt::Key_F4);
    connect(actionShowCsfdDetail, &QAction::triggered, this, &TorrentTransferTableView::showCsfdDetail);
    auto *actionShowImdbDetail = new QAction(QIcon(":/icons/imdb_w.svg"), QStringLiteral("Show &imdb detail..."), listMenu);
    actionShowImdbDetail->setShortcut(Qt::Key_F6);
    actionShowImdbDetail->setEnabled(false);
    connect(actionShowImdbDetail, &QAction::triggered, this, &TorrentTransferTableView::showImdbDetail);
    auto *actionPreviewTorrent = new QAction(QIcon(":/icons/ondemand_video_w.svg"), QStringLiteral("&Preview file..."), listMenu);
    actionPreviewTorrent->setShortcut(Qt::Key_F3);
    connect(actionPreviewTorrent, &QAction::triggered, this, &TorrentTransferTableView::previewSelectedTorrent);
    auto *actionDeleteTorrent = new QAction(QIcon(":/icons/delete_w.svg"), QStringLiteral("&Delete torrent"), listMenu);
    actionDeleteTorrent->setShortcuts(QKeySequence::Delete);
    connect(actionDeleteTorrent, &QAction::triggered, this, &TorrentTransferTableView::deleteSelectedTorrent);

    // Add actions to menu
    listMenu->addAction(actionShowCsfdDetail);
    listMenu->addAction(actionShowImdbDetail);
    listMenu->addSeparator();
    listMenu->addAction(actionPreviewTorrent);
    listMenu->addAction(actionDeleteTorrent);

    if (event->reason() == QContextMenuEvent::Keyboard) {
        // Show context menu next to selected row
        // I like this method, because it show popup every time on the same x position
        auto selectedTorrentIndex = getSelectedTorrentIndex();
        auto x = columnViewportPosition(TR_PROGRESS);
        auto y = rowViewportPosition(selectedTorrentIndex.row());
        listMenu->popup(mapToGlobal(QPoint(x, y)));
        return;
    }

    listMenu->popup(QCursor::pos());
}

void TorrentTransferTableView::deleteSelectedTorrent()
{
    // qBittorrent is not running, so nothing to send
    if (m_qBittorrentHwnd == nullptr) {
        QMessageBox::information(this, QStringLiteral("Delete impossible"),
                                 QStringLiteral("qBittorrent is not running."));
        return;
    }

    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    const auto result =
            QMessageBox::question(parentWidget(), QStringLiteral("Delete torrent"),
                                  QStringLiteral("Do you want to delete torrent:<br>"
                                                 "<strong>%1</strong>")
                                  .arg(torrent.value("name").toString()),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No);
    // Nothing to delete
    if (result != QMessageBox::Yes)
        return;

    qDebug() << "Delete selected torrent :" << torrent.value("name").toString();

    QByteArray infoHash = torrent.value("hash").toByteArray();
    COPYDATASTRUCT torrentInfoHash;
    torrentInfoHash.lpData = infoHash.data();
    torrentInfoHash.cbData = infoHash.size();
    torrentInfoHash.dwData = NULL;
    ::SendMessage(m_qBittorrentHwnd, WM_COPYDATA, (WPARAM) MSG_QMD_DELETE_TORRENT, (LPARAM) (LPVOID) &torrentInfoHash);
}

void TorrentTransferTableView::showCsfdDetail()
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Show CSFD detail :" << torrent.value("name").toString();

    auto *const dialog = new MovieDetailDialog(this);
    // TODO measure with and w/o Qt::WA_DeleteOnClose, idea is to reuse dialog and not to create and destroy everytime silverqx
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->prepareData(torrent);
    connect(dialog, &MovieDetailDialog::readyToPreviewFile, this, &TorrentTransferTableView::previewSelectedTorrent);
    dialog->show();
}

void TorrentTransferTableView::showImdbDetail()
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Show IMDB detail :" << torrent.value("name").toString();

    QMessageBox::information(this, QStringLiteral("imdb movie detail"),
                             QStringLiteral("Currently not implemented."));
}

void TorrentTransferTableView::updateChangedTorrents(const QVector<QString> &torrentInfoHashes)
{
    const auto model = dynamic_cast<TorrentSqlTableModel *const>(m_model);
    quint64 torrentId;

    foreach (const auto infoHash, torrentInfoHashes) {
        // Update row by row id
        model->selectRow(model->getTorrentRowByInfoHash(infoHash));

        // If torrent is saved in torrent files cache, then remove it
        torrentId = model->getTorrentIdByInfoHash(infoHash);
        if (m_torrentFilesCache.contains(torrentId))
            removeRecordFromTorrentFilesCache(torrentId);
    }
}
