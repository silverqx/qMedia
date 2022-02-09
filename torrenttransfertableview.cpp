#include "torrenttransfertableview.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QDir>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QScrollBar>
#include <QShortcut>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include "commonglobal.h"
#include "csfddetailservice.h"
#include "macros/clangwarnings.h"
#include "moviedetaildialog.h"
#include "previewselectdialog.h"
#include "torrentsqltablemodel.h"
#include "torrentstatus.h"
#include "torrenttabledelegate.h"
#include "torrenttablesortmodel.h"
#include "utils/fs.h"
#include "utils/gui.h"
#include "utils/string.h"

TorrentTransferTableView::TorrentTransferTableView(
        HWND qBittorrentHwnd, QWidget *const parent
)
    : QTableView(parent)
    , m_qBittorrentHwnd(qBittorrentHwnd)
    , m_statusHash(StatusHash::instance())
    , m_regexTorrentsFilter("regex_torrents_filter", false)
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

    // Horizontal / Vertical Headers
    auto *horizontalHeaderTmp = horizontalHeader();
    horizontalHeaderTmp->setHighlightSections(false);
    horizontalHeaderTmp->setStretchLastSection(false);
    auto *verticalHeaderTmp = verticalHeader();
    verticalHeaderTmp->setVisible(false);
    verticalHeaderTmp->setMinimumSectionSize(32);
    verticalHeaderTmp->setDefaultSectionSize(36);
    verticalHeaderTmp->setHighlightSections(false);
    verticalHeaderTmp->setStretchLastSection(false);

    // Create and apply delegate
    m_tableDelegate = new TorrentTableDelegate(this); // NOLINT(cppcoreguidelines-owning-memory)
    setItemDelegate(m_tableDelegate);

    // Create models
    m_model = new TorrentSqlTableModel(this); // NOLINT(cppcoreguidelines-owning-memory)
    m_model->setTable(QStringLiteral("torrents"));
    m_model->setEditStrategy(QSqlTableModel::OnRowChange);
    m_model->select();
    m_model->setHeaderData(TR_ID, Qt::Horizontal, QStringLiteral("Id"));
    m_model->setHeaderData(TR_NAME, Qt::Horizontal, QStringLiteral("Name"));
    m_model->setHeaderData(TR_PROGRESS, Qt::Horizontal, QStringLiteral("Done"));
    m_model->setHeaderData(TR_ETA, Qt::Horizontal, QStringLiteral("ETA"));
    m_model->setHeaderData(TR_SIZE, Qt::Horizontal, QStringLiteral("Size"));
    m_model->setHeaderData(TR_SEEDS, Qt::Horizontal, QStringLiteral("Seeds"));
    m_model->setHeaderData(TR_TOTAL_SEEDS, Qt::Horizontal, QStringLiteral("Total seeds"));
    m_model->setHeaderData(TR_LEECHERS, Qt::Horizontal, QStringLiteral("Leechs"));
    m_model->setHeaderData(TR_TOTAL_LEECHERS, Qt::Horizontal,
                           QStringLiteral("Total leechers"));
    m_model->setHeaderData(TR_AMOUNT_LEFT, Qt::Horizontal, QStringLiteral("Remaining"));
    m_model->setHeaderData(TR_ADDED_ON, Qt::Horizontal, QStringLiteral("Added on"));

    m_proxyModel = new TorrentTableSortModel(this); // NOLINT(cppcoreguidelines-owning-memory)
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(TR_NAME);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    setModel(m_proxyModel);
    hideColumn(TR_ID);
    hideColumn(TR_TOTAL_SEEDS);
    hideColumn(TR_TOTAL_LEECHERS);
    hideColumn(TR_HASH);
    hideColumn(TR_CSFD_MOVIE_DETAIL);
    hideColumn(TR_STATUS);
    hideColumn(TR_MOVIE_DETAIL_INDEX);
    hideColumn(TR_SAVE_PATH);
    sortByColumn(TR_ADDED_ON, Qt::DescendingOrder);

    // Init čsfd service
    m_csfdDetailService = std::make_shared<CsfdDetailService>(
                              qobject_cast<TorrentSqlTableModel *>(m_model));

    // Init torrent context menu
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // Connect events
    connect(this, &QAbstractItemView::doubleClicked,
            this, &TorrentTransferTableView::previewSelectedTorrent);

    // Hotkeys
    // torrentTransferTableView
QMEDIA_DISABLE_CLANG_WARNING_ENUM_CONVERSION
    const auto *const doubleClickHotkeyReturn =
            new QShortcut(Qt::Key_Return, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyReturn, &QShortcut::activated,
            this, &TorrentTransferTableView::previewSelectedTorrent);
    const auto *const doubleClickHotkeyReturnAlt =
            new QShortcut(Qt::SHIFT + Qt::Key_Return, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyReturnAlt, &QShortcut::activated,
            this, &TorrentTransferTableView::showCsfdDetail);
    const auto *const doubleClickHotkeyEnter =
            new QShortcut(Qt::Key_Enter, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyEnter, &QShortcut::activated,
            this, &TorrentTransferTableView::previewSelectedTorrent);
    const auto *const doubleClickHotkeyF3 =
            new QShortcut(Qt::Key_F3, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF3, &QShortcut::activated,
            this, &TorrentTransferTableView::previewSelectedTorrent);
    const auto *const doubleClickHotkeyDelete =
            new QShortcut(Qt::Key_Delete, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyDelete, &QShortcut::activated,
            this, &TorrentTransferTableView::deleteSelectedTorrent);
    const auto *const doubleClickHotkeyF4 =
            new QShortcut(Qt::Key_F4, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF4, &QShortcut::activated,
            this, &TorrentTransferTableView::showCsfdDetail);
    const auto *const doubleClickHotkeyF6 =
            new QShortcut(Qt::Key_F6, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF6, &QShortcut::activated,
            this, &TorrentTransferTableView::showImdbDetail);
    const auto *const doubleClickForceResume =
            new QShortcut(Qt::CTRL + Qt::Key_Space, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickForceResume, &QShortcut::activated,
            this, &TorrentTransferTableView::forceResumeSelectedTorrent);
    const auto *const doubleClickOpenFolder =
            new QShortcut(Qt::ALT + Qt::Key_O, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickOpenFolder, &QShortcut::activated,
            this, &TorrentTransferTableView::openFolderForSelectedTorrent);
    const auto *const doubleClickForceRecheck =
            new QShortcut(Qt::ALT + Qt::Key_R, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickForceRecheck, &QShortcut::activated,
            this, &TorrentTransferTableView::forceRecheckSelectedTorrent);
    const auto *const doubleClickPauseResume =
            new QShortcut(Qt::Key_Space, this, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickPauseResume, &QShortcut::activated,
            this, &TorrentTransferTableView::pauseResumeSelectedTorrent);
QMEDIA_RESTORE_CLANG_WARNINGS

    // Resize columns to the default state after right click
    horizontalHeaderTmp->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(horizontalHeaderTmp, &QHeaderView::customContextMenuRequested,
            this, &TorrentTransferTableView::resizeColumns);

    // Select torrent with latest / highest added on datetime
    selectionModel()->select(m_model->index(0, TR_NAME),
                             QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
#ifdef LOG_GEOMETRY
    qDebug() << "TorrentTransferTableView() ctor";
#endif
}

void TorrentTransferTableView::showEvent(QShowEvent *event)
{
    // Event originated from system
    if (event->spontaneous()) {
        QTableView::showEvent(event);
        return;
    }

#ifdef LOG_GEOMETRY
    qDebug() << "showEvent(), m_showEventInitialized ="
             << (m_showEventInitialized ? "true" : "false");
#endif

    if (m_showEventInitialized)
        return;

    // Move this as top as possible to prevent race condition
    m_showEventInitialized = true;

    resizeColumns();
}

void TorrentTransferTableView::contextMenuEvent(QContextMenuEvent *const event)
{
    if (event->reason() == QContextMenuEvent::Keyboard)
        qDebug() << "Torrent table context menu reason : keyboard";

    displayListMenu(event);
}

int TorrentTransferTableView::getModelRowCount() const
{
    return m_model->rowCount();
}

void TorrentTransferTableView::previewFile(const QString &filePath) const
{
    Utils::Gui::openPath(filePath);
}

QSharedPointer<const QVector<QSqlRecord>>
TorrentTransferTableView::selectTorrentFilesById(const quint64 id) const
{
    // Return from cache
    if (m_torrentFilesCache.contains(id))
        return m_torrentFilesCache.value(id);

    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare(QStringLiteral("SELECT * FROM torrent_previewable_files "
                                 "WHERE torrent_id = ?"));
    query.addBindValue(id);

    const auto ok = query.exec();
    if (!ok) {
        qDebug("Select of torrent(ID%llu) files failed : %s",
               id, qUtf8Printable(query.lastError().text()));
        return {};
    }

    const auto torrentFiles = QSharedPointer<QVector<QSqlRecord>>::create();
    while (query.next())
        torrentFiles->append(query.record());

    if (torrentFiles->isEmpty()) {
        qDebug().noquote() << QStringLiteral("No torrent files in DB for torrent(ID%1), "
                                             "this should never happen :/")
                              .arg(id);
        return torrentFiles;
    }

    m_torrentFilesCache.insert(id, torrentFiles);

    return torrentFiles;
}

QModelIndex TorrentTransferTableView::getSelectedTorrentIndex() const
{
    const auto selectedIndexes = selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return {};

    const auto selectedIndex = selectedIndexes.first();
    if (!selectedIndex.isValid())
        return {};

    return selectedIndex;
}

QSqlRecord TorrentTransferTableView::getSelectedTorrentRecord() const
{
    const auto selectedIndex = getSelectedTorrentIndex();
    if (!selectedIndex.isValid())
        return {};

    return m_model->record(
        m_proxyModel->mapToSource(selectedIndex).row()
    );
}

void TorrentTransferTableView::removeRecordFromTorrentFilesCache(const quint64 torrentId) const
{
    if (!m_torrentFilesCache.contains(torrentId)) {
        qDebug() << "Torrent files cache doesn't contain torrent :"
                 << torrentId;
        return;
    }

    m_torrentFilesCache.remove(torrentId);
}

void TorrentTransferTableView::filterTextChanged(const QString &name) const
{
    const auto pattern = m_regexTorrentsFilter
                         ? name
                         : Utils::String::wildcardToRegexPattern(name);

    m_proxyModel->setFilterRegularExpression(
                QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption));
}

void TorrentTransferTableView::previewSelectedTorrent()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug().noquote() << "Torrent doubleclicked :"
                       << torrent.value("name").toString();

    const auto torrentFiles = selectTorrentFilesById(torrent.value("id").toULongLong());
    if (torrentFiles->isEmpty()) {
        QMessageBox::critical(
                    this, QStringLiteral("Preview impossible"),
                    QStringLiteral("Torrent <strong>%1</strong> does not contain any "
                                   "previewable files.")
                    .arg(torrent.value("name").toString()));
        return;
    }

    // If torrent contains only one file, do not show the preview dialog
    if (torrentFiles->size() == 1) {
        // TODO duplicit code in PreviewSelectDialog::getTorrentFileFilePathAbs() and also in self::openFolderForSelectedTorrent() silverqx
        const QDir saveDir(torrent.value(TR_SAVE_PATH).toString());
        previewFile(Utils::Fs::expandPathAbs(
                        saveDir.absoluteFilePath(
                            torrentFiles->first().value("filepath").toString())));
        return;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto *const dialog = new PreviewSelectDialog(this, torrent, torrentFiles);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // TODO set width to 90% of mainwindow silverqx
    connect(dialog, &PreviewSelectDialog::readyToPreviewFile,
            this, &TorrentTransferTableView::previewFile);
    dialog->show();
}

void TorrentTransferTableView::reloadTorrentModel() const
{
    // CUR remember selected torrent by InfoHash, not by row silverqx
    // Remember currently selected row or first row, if was nothing selected
    int selectRow = 0;
    const auto selectedRows = selectionModel()->selectedRows();
    if (!selectedRows.isEmpty())
        selectRow = selectedRows.first().row();

    // Reload model
    m_model->select();
    qInfo() << "Torrent model reloaded";

    // This situation can occur, when torrents was deleted and one of the last rows was selected
    const auto rowCount = m_model->rowCount();
    if (selectRow > rowCount)
        selectRow = rowCount;

    // Reselect remembered row
    // TODO currentSelection rectangle is not showed silverqx
    selectionModel()->setCurrentIndex(
                m_model->index(selectRow, TR_NAME),
                QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void TorrentTransferTableView::displayListMenu(const QContextMenuEvent *const event)
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Not displaying the context menu because any torrent is selected";
        return;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto *const listMenu = new QMenu(this);
    listMenu->setAttribute(Qt::WA_DeleteOnClose);
    listMenu->setStyleSheet(R"T(QMenu::separator {
    height: 1px;
    background-color: #393939;
    margin: 4px 14px 1px 33px;
}
)T");

    // Create actions
QMEDIA_DISABLE_CLANG_WARNING_ENUM_CONVERSION
    auto *const actionPause =
            createActionForMenu(QIcon(":/icons/media-playback-pause.svg"),
                                QStringLiteral("&Pause"),
                                Qt::Key_Space,
                                &TorrentTransferTableView::pauseSelectedTorrent,
                                listMenu, isqBittorrentUp());
    auto *const actionResume =
            createActionForMenu(QIcon(":/icons/media-playback-start.svg"),
                                QStringLiteral("&Resume"),
                                Qt::Key_Space,
                                &TorrentTransferTableView::resumeSelectedTorrent,
                                listMenu, isqBittorrentUp());
    auto *const actionForceResume =
            createActionForMenu(QIcon(":/icons/media-seek-forward.svg"),
                                QStringLiteral("&Force Resume"),
                                Qt::CTRL + Qt::Key_Space,
                                &TorrentTransferTableView::forceResumeSelectedTorrent,
                                listMenu, isqBittorrentUp());
    auto *const actionShowCsfdDetail =
            createActionForMenu(QIcon(":/icons/csfd_w.svg"),
                                QStringLiteral("Show &csfd detail..."),
                                Qt::Key_F4,
                                &TorrentTransferTableView::showCsfdDetail, listMenu);
    auto *const actionShowImdbDetail =
            createActionForMenu(QIcon(":/icons/imdb_w.svg"),
                                QStringLiteral("Show &imdb detail..."),
                                Qt::Key_F6,
                                &TorrentTransferTableView::showImdbDetail,
                                listMenu, false);
    auto *const actionOpenFolder =
            createActionForMenu(QIcon(":/icons/inode-directory_w.svg"),
                                QStringLiteral("&Open folder"),
                                Qt::ALT + Qt::Key_O,
                                &TorrentTransferTableView::openFolderForSelectedTorrent, listMenu);
    auto *const actionForceRecheck =
            createActionForMenu(QIcon(":/icons/document-edit-verify_w.svg"),
                                QStringLiteral("Force Rechec&k"),
                                Qt::ALT + Qt::Key_R,
                                &TorrentTransferTableView::forceRecheckSelectedTorrent,
                                listMenu, isqBittorrentUp());
    auto *const actionPreviewTorrent =
            createActionForMenu(QIcon(":/icons/ondemand_video_w.svg"),
                                QStringLiteral("Previe&w..."),
                                Qt::Key_F3,
                                &TorrentTransferTableView::previewSelectedTorrent, listMenu);
    auto *const actionDeleteTorrent =
            createActionForMenu(QIcon(":/icons/delete_w.svg"),
                                QStringLiteral("&Delete torrent"),
                                QKeySequence::Delete,
                                &TorrentTransferTableView::deleteSelectedTorrent,
                                listMenu, isqBittorrentUp());
QMEDIA_RESTORE_CLANG_WARNINGS

    // Add actions to the context menu
    // Pause, Resume and Force resume
    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (statusProperties.isForced()) {
        listMenu->addAction(actionPause);
        listMenu->addAction(actionResume);
    }
    else if (statusProperties.isDownloading()) {
        listMenu->addAction(actionPause);
        listMenu->addAction(actionForceResume);
    }
    if (statusProperties.isPaused()) {
        listMenu->addAction(actionResume);
        listMenu->addAction(actionForceResume);
    }
    listMenu->addSeparator();

    // Show čsfd.cz and imdb.com movie detail
    listMenu->addAction(actionShowCsfdDetail);
    listMenu->addAction(actionShowImdbDetail);

    // Open folder
    if (!statusProperties.isMoving()) {
        listMenu->addSeparator();
        listMenu->addAction(actionOpenFolder);
    }

    // Force recheck, Preview and Delete torrent
    if (!statusProperties.isChecking() || !statusProperties.isMoving()) {
        listMenu->addAction(actionForceRecheck);
        listMenu->addSeparator();
        listMenu->addAction(actionPreviewTorrent);
        listMenu->addAction(actionDeleteTorrent);
    }

    // When triggered from the keyboard, show the context menu on the custom position
    if (event->reason() == QContextMenuEvent::Keyboard) {
        // Show context menu next to a selected row
        // I like this method because it shows a popup every time on the same x position
        const auto selectedTorrentIndex = getSelectedTorrentIndex();
        const auto x = columnViewportPosition(TR_PROGRESS);
        const auto y = rowViewportPosition(selectedTorrentIndex.row());
        listMenu->popup(mapToGlobal(QPoint(x, y)));
        return;
    }

    listMenu->popup(QCursor::pos());
}

void TorrentTransferTableView::deleteSelectedTorrent()
{
    // qBittorrent is not running, so nothing to send
    if (!isqBittorrentUp()) {
        QMessageBox::information(this, QStringLiteral("Delete impossible"),
                                 QStringLiteral("qBittorrent is not running."));
        return;
    }

    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    if (const auto &statusProperties =
                (*m_statusHash)[torrent.value("status").toString()];
        statusProperties.isChecking() || statusProperties.isMoving()
    ) {
        QMessageBox::information(
                    this, QStringLiteral("Delete impossible"),
                    QStringLiteral("Torrent is in the checking or moving state."));
        return;
    }

    // Delete question modal
    {
        const auto result =
                QMessageBox::question(
                    parentWidget(), QStringLiteral("Delete torrent"),
                    QStringLiteral("Do you want to delete torrent:<br><strong>%1</strong>")
                    .arg(torrent.value("name").toString()),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        // Nothing to delete
        if (result != QMessageBox::Yes)
            return;
    }

    qDebug().noquote() << "Delete selected torrent :"
                       << torrent.value("name").toString();

    ::IpcSendByteArray(m_qBittorrentHwnd, ::MSG_QMD_DELETE_TORRENT,
                       torrent.value("hash").toByteArray());
}

void TorrentTransferTableView::showCsfdDetail()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Show čsfd movie detail failed because any torrent was selected";
        return;
    }

    qDebug().noquote() << "Show ČSFD detail :"
                       << torrent.value("name").toString();

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto *const dialog = new MovieDetailDialog(m_csfdDetailService, this);
    // TODO measure with and w/o Qt::WA_DeleteOnClose, idea is to reuse dialog and not to create and destroy everytime silverqx
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->prepareData(torrent);
    connect(dialog, &MovieDetailDialog::readyToPreviewFile,
            this, &TorrentTransferTableView::previewSelectedTorrent);
    dialog->show();
}

void TorrentTransferTableView::showImdbDetail()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Show imdb movie detail failed because any torrent was selected";
        return;
    }

    qDebug().noquote() << "Show IMDB detail :"
                       << torrent.value("name").toString();

    QMessageBox::information(this, QStringLiteral("imdb movie detail"),
                             QStringLiteral("Currently not implemented."));
}

void TorrentTransferTableView::pauseTorrent(const QSqlRecord &torrent) const
{
    qDebug().noquote() << "Pause selected torrent :"
                       << torrent.value("name").toString();

    ::IpcSendByteArray(m_qBittorrentHwnd, ::MSG_QMD_PAUSE_TORRENT,
                       torrent.value("hash").toByteArray());
}

void TorrentTransferTableView::pauseSelectedTorrent()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Pause failed because any torrent was selected";
        return;
    }

    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (!isqBittorrentUp() || !statusProperties.isDownloading())
        return;

    pauseTorrent(torrent);
}

void TorrentTransferTableView::resumeTorrent(const QSqlRecord &torrent) const
{
    qDebug().noquote() << "Resume selected torrent :"
                       << torrent.value("name").toString();

    ::IpcSendByteArray(m_qBittorrentHwnd, ::MSG_QMD_RESUME_TORRENT,
                       torrent.value("hash").toByteArray());
}

void TorrentTransferTableView::resumeSelectedTorrent()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Resume failed because any torrent was selected";
        return;
    }

    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (!isqBittorrentUp() ||
        (!statusProperties.isPaused() && !statusProperties.isForced())
    )
        return;

    resumeTorrent(torrent);
}

void TorrentTransferTableView::forceResumeSelectedTorrent()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Force resume failed because any torrent was selected";
        return;
    }

    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (!isqBittorrentUp() ||
        (!statusProperties.isDownloading() && !statusProperties.isPaused())
    )
        return;

    qDebug().noquote() << "Force resume selected torrent :"
                       << torrent.value("name").toString();

    ::IpcSendByteArray(m_qBittorrentHwnd, ::MSG_QMD_FORCE_RESUME_TORRENT,
                       torrent.value("hash").toByteArray());
}

void TorrentTransferTableView::forceRecheckSelectedTorrent()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Force recheck failed because any torrent was selected";
        return;
    }

    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (!isqBittorrentUp() ||
        statusProperties.isChecking() || statusProperties.isMoving()
    )
        return;

    qDebug().noquote() << "Force recheck selected torrent :"
                       << torrent.value("name").toString();

    ::IpcSendByteArray(m_qBittorrentHwnd, ::MSG_QMD_FORCE_RECHECK_TORRENT,
                       torrent.value("hash").toByteArray());
}

void TorrentTransferTableView::openFolderForSelectedTorrent()
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Open folder failed because any torrent was selected";
        return;
    }

    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (statusProperties.isMoving())
        return;

    const auto torrentName = torrent.value("name").toString();

    // Obtain torrent storage location
    const auto torrentSaveDir = torrent.value(TR_SAVE_PATH).toString();
    if (torrentSaveDir.isEmpty()) {
        qDebug().noquote()
                << "Open folder failed for the selected torrent (is empty) :"
                << torrentName;
        return;
    }

    // Check if a folder exists
    const QDir saveDir(torrentSaveDir);
    const auto folderName = saveDir.absolutePath();
    if (!saveDir.exists()) {
        qDebug("Open folder '%s' failed (doesn't exist) for the selected torrent : %s",
               qUtf8Printable(folderName),
               qUtf8Printable(torrentName));
        return;
    }

    qDebug("Open folder '%s' for the selected torrent : %s",
           qUtf8Printable(folderName),
           qUtf8Printable(torrentName));

    // Prepare process to execute
    QProcess totalCommander;
    totalCommander.setProgram(
                Utils::Fs::toNativePath(
                    QStringLiteral("C:/Program Files (x86)/TC UP/TOTALCMD.EXE")));

    // Prepare command line arguments
    QStringList arguments;
    // "C:\Program Files (x86)\TC UP\TOTALCMD.EXE" /O /T /R="%1"
    arguments << QStringLiteral("/O")
              << QStringLiteral("/T")
              << QStringLiteral("/R=""%1""")
                 .arg(Utils::Fs::toNativePath(folderName));
    totalCommander.setArguments(arguments);

    // Fire it up
    totalCommander.startDetached();
}

void TorrentTransferTableView::pauseResumeSelectedTorrent() const
{
    const auto torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Pause/Resume failed because any torrent was selected";
        return;
    }

    if (!isqBittorrentUp())
        return;

    const auto torrentName = torrent.value("name").toString();

    qDebug().noquote() << "Pause/Resume selected torrent :"
                       << torrentName;

    const auto &statusProperties = (*m_statusHash)[torrent.value("status").toString()];
    if (statusProperties.isPaused())
        resumeTorrent(torrent);
    else if (statusProperties.isDownloading() || statusProperties.isForced())
        pauseTorrent(torrent);
    else
        qDebug("Can't pause/resume selected torrent because status is '%s', torrent : %s",
               qUtf8Printable(statusProperties.title),
               qUtf8Printable(torrentName));
}

void TorrentTransferTableView::updateChangedTorrents(const QVector<QString> &torrentInfoHashes)
{
    auto *const model = qobject_cast<TorrentSqlTableModel *const>(m_model);

    for (const auto &infoHash : torrentInfoHashes) {
        // Update row by row id
        model->selectRow(model->getTorrentRowByInfoHash(infoHash));

        // If torrent is saved in torrent files cache, then remove it
        const auto torrentId = model->getTorrentIdByInfoHash(infoHash);
        if (m_torrentFilesCache.contains(torrentId))
            removeRecordFromTorrentFilesCache(torrentId);
    }
}

void TorrentTransferTableView::resizeColumns() const
{
#ifdef LOG_GEOMETRY
    qDebug() << "resizeColumns(), m_showEventInitialized ="
             << (m_showEventInitialized ? "true" : "false");
#endif
    /* Pixel perfectly sized torrentTransferTableView header.
       Set Name column width to all remaining area.
       Have to be called after show() because torrentTransferTableView width is needed. */
    auto *const tableViewHeader = horizontalHeader();
    tableViewHeader->resizeSections(QHeaderView::ResizeToContents);

    // Increase size section size about 40%
    const auto progressSize = tableViewHeader->sectionSize(TR_PROGRESS);
    tableViewHeader->resizeSection(TR_PROGRESS,
                                   static_cast<int>(progressSize + (progressSize * 0.4)));
    // Increase ETA section size about 30%
    const auto etaSize = tableViewHeader->sectionSize(TR_ETA);
    tableViewHeader->resizeSection(TR_ETA,
                                   static_cast<int>(etaSize + (etaSize * 0.3)));
    // Increase progress section size about 10%
    const auto sizeSize = tableViewHeader->sectionSize(TR_SIZE);
    tableViewHeader->resizeSection(TR_SIZE,
                                   static_cast<int>(sizeSize + (sizeSize * 0.1)));
    // Decrease seeds/leechs sections size about 10%
    const auto seedsSize = tableViewHeader->sectionSize(TR_SEEDS);
    tableViewHeader->resizeSection(TR_SEEDS,
                                   static_cast<int>(seedsSize - (sizeSize * 0.1)));
    const auto leechsSize = tableViewHeader->sectionSize(TR_LEECHERS);
    tableViewHeader->resizeSection(TR_LEECHERS,
                                   static_cast<int>(leechsSize - (sizeSize * 0.1)));
    // Remaining section do not need resize
    // Increase added on section size about 10%
    const auto addedOnSize = tableViewHeader->sectionSize(TR_ADDED_ON);
    tableViewHeader->resizeSection(TR_ADDED_ON,
                                   static_cast<int>(addedOnSize + (addedOnSize * 0.1)));

    // TODO also set minimum size for each section, based on above computed sizes silverqx

    // Compute name section size
    auto nameColWidth = width();
    const auto *const vScrollBar = verticalScrollBar();
    if (vScrollBar->isVisible())
        nameColWidth -= vScrollBar->width();
    nameColWidth -= tableViewHeader->sectionSize(TR_PROGRESS) +
                    tableViewHeader->sectionSize(TR_ETA) +
                    tableViewHeader->sectionSize(TR_SIZE) +
                    tableViewHeader->sectionSize(TR_SEEDS) +
                    tableViewHeader->sectionSize(TR_LEECHERS) +
                    tableViewHeader->sectionSize(TR_AMOUNT_LEFT) +
                    tableViewHeader->sectionSize(TR_ADDED_ON) +
                    2; // Borders

    // TODO at the end set mainwindow width and height based on torrentTransferTableView ( inteligently 😎 ) silverqx

    tableViewHeader->resizeSection(TR_NAME, nameColWidth);
    horizontalHeader()->setStretchLastSection(true);

    // Finally, I like this
    tableViewHeader->setSectionResizeMode(QHeaderView::Interactive);
    tableViewHeader->setCascadingSectionResizes(true);
}

void TorrentTransferTableView::togglePeerColumns()
{
    setColumnHidden(TR_SEEDS, !isqBittorrentUp());
    setColumnHidden(TR_LEECHERS, !isqBittorrentUp());

    if (!m_showEventInitialized) {
#ifdef LOG_GEOMETRY
        qDebug() << "togglePeerColumns(), m_showEventInitialized = false";
#endif
        return;
    }

#ifdef LOG_GEOMETRY
    qDebug() << "togglePeerColumns(), m_showEventInitialized = true";
#endif
    resizeColumns();
}
