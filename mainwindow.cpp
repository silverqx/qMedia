#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QShowEvent>
#include <QToolButton>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QWindow>

#include <qt_windows.h>
#include <Psapi.h>

#include <regex>

#include "common.h"
#include "previewselectdialog.h"
#include "torrentsqltablemodel.h"
#include "torrenttabledelegate.h"
#include "torrenttablesortmodel.h"
#include "utils/fs.h"
#include "utils/gui.h"

namespace {
    // Needed in EnumWindowsProc()
    MainWindow *l_mainWindow = nullptr;

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM)
    {
        // For reference, very important:
        // https://docs.microsoft.com/en-us/archive/msdn-magazine/2015/july/c-using-stl-strings-at-win32-api-boundaries
        const int windowTextLength = ::GetWindowTextLength(hwnd) + 1;
        auto windowText = std::make_unique<wchar_t[]>(windowTextLength);

        ::GetWindowText(hwnd, windowText.get(), windowTextLength);
#ifdef QT_DEBUG
        std::wstring text(windowText.get());
#endif

        // Example: [D: 0 B/s, U: 1,3 MiB/s] qBittorrent v4.2.5
        const std::wregex re(L"^(\\[\\D: .*\\, U\\: .*\\] )?qBittorrent (v\\d+\\.\\d+\\.\\d+([a-zA-Z]+\\d{0,2})?)$");
        if (!std::regex_match(windowText.get(), re))
            return true;

        DWORD pid;
        ::GetWindowThreadProcessId(hwnd, &pid);
        const HANDLE processHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
        if (processHandle == NULL) {
            qDebug() << "OpenProcess() in EnumWindows() failed :" << ::GetLastError();
            return true;
        }

        wchar_t moduleFilePath[MAX_PATH];
        ::GetModuleFileNameEx(processHandle, NULL, moduleFilePath, ARRAYSIZE(moduleFilePath));
        // More instances of qBittorrent can run, so find proper one
#ifdef QT_DEBUG
        // String has to start with moduleFileName
        if (::wcsstr(moduleFilePath, L"E:\\c\\qbittorrent_64-dev\\qBittorrent\\qBittorrent-builds")
            != &moduleFilePath[0])
            return true;
#else
        if (::wcsstr(moduleFilePath, L"C:\\Program Files\\qBittorrent") != &moduleFilePath[0])
            return true;
#endif
        const QString moduleFileName = Utils::Fs::fileName(QString::fromWCharArray(moduleFilePath));
        // TODO create finally helper https://www.modernescpp.com/index.php/c-core-guidelines-when-you-can-t-throw-an-exception silverqx
        // Or https://www.codeproject.com/Tips/476970/finally-clause-in-Cplusplus
        ::CloseHandle(processHandle);
        if (moduleFileName != "qbittorrent.exe")
            return true;

        qDebug() << "HWND for qBittorrent window found :" << hwnd;
        l_mainWindow->setQBittorrentHwnd(hwnd);

        return false;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    l_mainWindow = this;

    ui->setupUi(this);

    // MainWindow
    setWindowTitle("qMedia v0.0.1");
    const QIcon appIcon(QStringLiteral(":/icons/qmedia.svg"));
    setWindowIcon(appIcon);

    connectToDb();
    initTorrentTableView();
    initFilterLineEdit();

    // StatusBar
    createStatusBar();

    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::filterTextChanged);
    connect(ui->tableView, &QAbstractItemView::doubleClicked, this, &MainWindow::previewSelectedTorrent);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::reloadTorrentModel);
    connect(this, &MainWindow::torrentsAddedOrRemoved, this, &MainWindow::reloadTorrentModel);
    connect(this, &MainWindow::torrentsChanged, this, &MainWindow::updateChangedTorrents);

    // Hotkeys
    // tableview
    const auto *doubleClickHotkeyReturn = new QShortcut(Qt::Key_Return, ui->tableView, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyReturn, &QShortcut::activated, this, &MainWindow::previewSelectedTorrent);
    const auto *doubleClickHotkeyEnter = new QShortcut(Qt::Key_Enter, ui->tableView, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyEnter, &QShortcut::activated, this, &MainWindow::previewSelectedTorrent);
    const auto *doubleClickHotkeyF3 = new QShortcut(Qt::Key_F3, ui->tableView, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyF3, &QShortcut::activated, this, &MainWindow::previewSelectedTorrent);
    const auto *doubleClickHotkeyDelete = new QShortcut(Qt::Key_Delete, ui->tableView, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyDelete, &QShortcut::activated, this, &MainWindow::deleteSelectedTorrent);
    // lineedit
    const auto *doubleClickHotkeyF2 = new QShortcut(Qt::Key_F2, this, nullptr, nullptr, Qt::WindowShortcut);
    connect(doubleClickHotkeyF2, &QShortcut::activated, this, &MainWindow::focusSearchFilter);
    const auto *switchSearchFilterShortcut = new QShortcut(QKeySequence::Find, this);
    connect(switchSearchFilterShortcut, &QShortcut::activated, this, &MainWindow::focusSearchFilter);
    const auto *doubleClickHotkeyDown = new QShortcut(Qt::Key_Down, ui->lineEdit, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyDown, &QShortcut::activated, this, &MainWindow::focusTableView);
    const auto *doubleClickHotkeyEsc = new QShortcut(Qt::Key_Escape, ui->lineEdit, nullptr, nullptr, Qt::WidgetShortcut);
    connect(doubleClickHotkeyEsc, &QShortcut::activated, ui->lineEdit, &QLineEdit::clear);
    // Reload model from DB
    const auto *doubleClickHotkeyF5 = new QShortcut(Qt::Key_F5, this, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(doubleClickHotkeyF5, &QShortcut::activated, this, &MainWindow::reloadTorrentModel);
    const auto *doubleClickCtrlR = new QShortcut(Qt::CTRL + Qt::Key_R, this, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(doubleClickCtrlR, &QShortcut::activated, this, &MainWindow::reloadTorrentModel);

    // Initial focus
    ui->tableView->setFocus();

    // Find qBittorent's main window HWND
    ::EnumWindows(EnumWindowsProc, NULL);
    // Send hwnd of MainWindow to qBittorrent, aka. inform that qMedia is running
    if (m_qbittorrentHwnd != nullptr)
        ::PostMessage(m_qbittorrentHwnd, MSG_QMEDIA_UP, (WPARAM) winId(), NULL);
}

MainWindow::~MainWindow()
{
    if (m_qbittorrentHwnd != nullptr)
        ::PostMessage(m_qbittorrentHwnd, MSG_QMEDIA_DOWN, NULL, NULL);

    foreach (auto torrentFiles, m_torrentFilesCache)
        delete torrentFiles;

    delete ui;
}

void MainWindow::setQBittorrentHwnd(const HWND hwnd)
{
    m_qbittorrentHwnd = hwnd;
}

void MainWindow::previewFile(const QString &filePath)
{
    Utils::Gui::openPath(filePath);
}

void MainWindow::refreshStatusBar()
{
    m_torrentsCountLabel->setText(QStringLiteral("Torrents: <strong>%1</strong>").arg(selectTorrentsCount()));
    m_torrentFilesCountLabel->setText(QStringLiteral("Video Files: <strong>%1</strong>").arg(selectTorrentFilesCount()));
}

void MainWindow::connectToDb() const
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"));
    db.setHostName(qEnvironmentVariable("QMEDIA_DB_HOST", QStringLiteral("127.0.0.1")));
#ifdef QT_DEBUG
    db.setDatabaseName(qEnvironmentVariable("QMEDIA_DB_DATABASE_DEBUG", ""));
#else
    db.setDatabaseName(qEnvironmentVariable("QMEDIA_DB_DATABASE", ""));
#endif
    db.setUserName(qEnvironmentVariable("QMEDIA_DB_USERNAME", ""));
    db.setPassword(qEnvironmentVariable("QMEDIA_DB_PASSWORD", ""));

    bool ok = db.open();
    if (!ok)
        // TODO qt insert spaces automatically, so remove redundant spaces silverqx
        qDebug() << "Connect to database failed :" << db.lastError().text();
}

void MainWindow::initTorrentTableView()
{
    // Create and apply delegate
    m_tableDelegate = new TorrentTableDelegate(this);
    ui->tableView->setItemDelegate(m_tableDelegate);

    m_model = new TorrentSqlTableModel(this);
    m_model->setTable(QStringLiteral("torrents"));
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->select();
    m_model->setHeaderData(TR_ID, Qt::Horizontal, QStringLiteral("Id"));
    m_model->setHeaderData(TR_NAME, Qt::Horizontal, QStringLiteral("Name"));
    m_model->setHeaderData(TR_SIZE, Qt::Horizontal, QStringLiteral("Size"));
    m_model->setHeaderData(TR_PROGRESS, Qt::Horizontal, QStringLiteral("Done"));
    m_model->setHeaderData(TR_ADDED_ON, Qt::Horizontal, QStringLiteral("Added on"));

    m_proxyModel = new TorrentTableSortModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(TR_NAME);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    ui->tableView->setModel(m_proxyModel);
    ui->tableView->hideColumn(TR_ID);
    ui->tableView->hideColumn(TR_HASH);
    ui->tableView->sortByColumn(TR_ADDED_ON, Qt::DescendingOrder);

    // Init torrent context menu
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QWidget::customContextMenuRequested, this, &MainWindow::displayListMenu);
}

void MainWindow::initFilterLineEdit()
{
    m_searchButton = new QToolButton(ui->lineEdit);
    const QIcon searchIcon(QStringLiteral(":/icons/search.svg"));
    m_searchButton->setIcon(searchIcon);
    m_searchButton->setCursor(Qt::ArrowCursor);
    m_searchButton->setStyleSheet(QStringLiteral("QToolButton {border: none; padding: 2px 0 2px 4px;}"));

    // Padding between text and widget borders
    ui->lineEdit->setStyleSheet(QString::fromLatin1("QLineEdit {padding-left: %1px;}")
                                .arg(m_searchButton->sizeHint().width()));

    const int frameWidth = ui->lineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    ui->lineEdit->setMaximumHeight(std::max(ui->lineEdit->sizeHint().height(),
                                            m_searchButton->sizeHint().height()) + (frameWidth * 2));
    ui->lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate)
        reloadTorrentModel();

    return QMainWindow::event(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    // Event originated from system
    if (event->spontaneous())
        return QMainWindow::showEvent(event);

    if (m_showEventInitialized)
        return;

    // Pixel perfectly sized tableView header
    // Set Name column width to all remaining area
    // Have to be called after show(), because tableView width is needed
    QHeaderView *const tableViewHeader = ui->tableView->horizontalHeader();
    tableViewHeader->resizeSections(QHeaderView::ResizeToContents);

    // Increase progress section size about 10%
    const int sizeSize = tableViewHeader->sectionSize(TR_SIZE);
    tableViewHeader->resizeSection(TR_SIZE, sizeSize + (sizeSize * 0.1));
    // Increase size section size about 40%
    const int progressSize = tableViewHeader->sectionSize(TR_PROGRESS);
    tableViewHeader->resizeSection(TR_PROGRESS, progressSize + (progressSize * 0.4));
    // Increase added on section size about 10%
    const int addedOnSize = tableViewHeader->sectionSize(TR_ADDED_ON);
    tableViewHeader->resizeSection(TR_ADDED_ON, addedOnSize + (addedOnSize * 0.1));

    // Compute name section size
    int nameColWidth = ui->tableView->width();
    const QScrollBar *const vScrollBar = ui->tableView->verticalScrollBar();
    if (vScrollBar->isVisible())
        nameColWidth -= vScrollBar->width();
    nameColWidth -= tableViewHeader->sectionSize(TR_SIZE);
    nameColWidth -= tableViewHeader->sectionSize(TR_PROGRESS);
    nameColWidth -= tableViewHeader->sectionSize(TR_ADDED_ON);
    nameColWidth -= 2; // Borders

    tableViewHeader->resizeSection(TR_NAME, nameColWidth);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    // Finally, I like this
    tableViewHeader->setSectionResizeMode(QHeaderView::Interactive);
    tableViewHeader->setCascadingSectionResizes(true);

    m_showEventInitialized = true;
}

QVector<QSqlRecord> *MainWindow::selectTorrentFilesById(quint64 id)
{
    // Return from cache
    if (m_torrentFilesCache.contains(id))
        return m_torrentFilesCache.value(id);

    QSqlQuery query;
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

void MainWindow::createStatusBar()
{
    QWidget *container = new QWidget(this);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(11, 0, 16, 5);
    container->setLayout(layout);

    // Create widgets displayed statusbar
    m_torrentsCountLabel = new QLabel(QStringLiteral("Torrents: <strong>%1</strong>")
                                      .arg(m_model->rowCount()), this);
    m_torrentFilesCountLabel = new QLabel(QStringLiteral("Video Files: <strong>%1</strong>")
                                          .arg(selectTorrentFilesCount()), this);
    connect(this, &MainWindow::torrentsAddedOrRemoved, this, &MainWindow::refreshStatusBar);

    // Create needed splitters
    auto splitter1 = new QFrame(statusBar());
    splitter1->setFrameStyle(QFrame::VLine);
    splitter1->setFrameShadow(QFrame::Raised);

    layout->addWidget(m_torrentsCountLabel);
    layout->addWidget(splitter1);
    layout->addWidget(m_torrentFilesCountLabel);

    statusBar()->addPermanentWidget(container);
}

uint MainWindow::selectTorrentsCount() const
{
    QSqlQuery query("SELECT COUNT(*) as count FROM torrents");

    const bool ok = query.exec();
    if (!ok) {
        qDebug() << QStringLiteral("Select of torrents count failed :")
                 << query.lastError().text();
        return 0;
    }

    query.first();
    return query.value(0).toUInt();
}

uint MainWindow::selectTorrentFilesCount() const
{
    QSqlQuery query("SELECT COUNT(*) as count FROM torrents_previewable_files");

    const bool ok = query.exec();
    if (!ok) {
        qDebug() << QStringLiteral("Select of torrent files count failed :")
                 << query.lastError().text();
        return 0;
    }

    query.first();
    return query.value(0).toUInt();
}

QModelIndex MainWindow::getSelectedTorrentIndex() const
{
    QModelIndexList selectedIndexes = ui->tableView->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return {};
    QModelIndex selectedIndex = selectedIndexes.first();
    if (!selectedIndex.isValid())
        return {};

    return selectedIndex;
}

QSqlRecord MainWindow::getSelectedTorrentRecord() const
{
    QModelIndex selectedIndex = getSelectedTorrentIndex();
    if (!selectedIndex.isValid())
        return {};

    return m_model->record(
        m_proxyModel->mapToSource(selectedIndex).row()
    );
}

void MainWindow::filterTextChanged(const QString &name)
{
    m_proxyModel->setFilterRegExp(
        QRegExp(name, Qt::CaseInsensitive, QRegExp::WildcardUnix));
}

void MainWindow::previewSelectedTorrent()
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Torrent doubleclicked :" << torrent.value("name").toString();

    const QVector<QSqlRecord> *const torrentFiles = selectTorrentFilesById(torrent.value("id").toULongLong());
    if (torrentFiles->isEmpty()) {
        QMessageBox::critical(this, tr("Preview impossible"),
                              tr("Torrent <strong>%1</strong> does not contain any previewable files.")
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
    connect(dialog, &PreviewSelectDialog::readyToPreviewFile, this, &MainWindow::previewFile);
    dialog->show();
    // TODO set current selection to selected model, because after preview dialog is closed, current selection is at 0,0 silverqx
}

void MainWindow::reloadTorrentModel()
{
    // TODO remember selected torrent by InfoHash, not by row silverqx
    // Remember currently selected row or first row, if was nothing selected
    int selectRow = 0;
    auto selectedRows = ui->tableView->selectionModel()->selectedRows();
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
    // TODO doesn't work well, selection works, but when I press up or down, then navigation starts from modelindex(0,0), don't know why :/, also tried | QItemSelectionModel::Current, but didn't help silverqx
    ui->tableView->selectionModel()->select(m_model->index(selectRow, 0),
                                            QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void MainWindow::displayListMenu(const QPoint &position)
{
    Q_UNUSED(position);

    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty()) {
        qDebug() << "Do not displaying context menu, because any torrent is selected.";
        return;
    }

    auto *listMenu = new QMenu(ui->tableView);
    listMenu->setAttribute(Qt::WA_DeleteOnClose);

    // Create actions
    auto *actionShowCsfdDetail = new QAction(QIcon(":/icons/csfd.svg"), QStringLiteral("Show &csfd detail..."), listMenu);
    connect(actionShowCsfdDetail, &QAction::triggered, this, &MainWindow::showCsfdDetail);
    auto *actionShowImdbDetail = new QAction(QIcon(":/icons/imdb.svg"), QStringLiteral("Show &imdb detail..."), listMenu);
    connect(actionShowImdbDetail, &QAction::triggered, this, &MainWindow::showImdbDetail);
    auto *actionPreviewTorrent = new QAction(QIcon(":/icons/ondemand_video.svg"), QStringLiteral("&Preview file..."), listMenu);
    actionPreviewTorrent->setShortcut(Qt::Key_F3);
    connect(actionPreviewTorrent, &QAction::triggered, this, &MainWindow::previewSelectedTorrent);
    auto *actionDeleteTorrent = new QAction(QIcon(":/icons/delete.svg"), QStringLiteral("&Delete torrent"), listMenu);
    actionDeleteTorrent->setShortcuts(QKeySequence::Delete);
    connect(actionDeleteTorrent, &QAction::triggered, this, &MainWindow::deleteSelectedTorrent);

    // Add actions to menu
    listMenu->addAction(actionShowCsfdDetail);
    listMenu->addAction(actionShowImdbDetail);
    listMenu->addSeparator();
    listMenu->addAction(actionPreviewTorrent);
    listMenu->addAction(actionDeleteTorrent);

    listMenu->popup(QCursor::pos());
    // Show context menu next to selected row
    // TODO only use this, if popup was triggered by keyboard, I need to subclass QTableView and override conextMenuEvent(), than I will have access to reason() silverqx
//    auto selectedTorrentIndex = getSelectedTorrentIndex();
//    auto x = ui->tableView->columnViewportPosition(TR_SIZE);
//    auto y = ui->tableView->rowViewportPosition(selectedTorrentIndex.row());
//    // +70 because it is centered by default
//    listMenu->popup(mapToGlobal(QPoint(x, y + 70)));
    // This is other method, position is passed as parameter
//    auto p = QPoint(position.x(), position.y() + 70);
//    listMenu->popup(mapToGlobal(p));
}

void MainWindow::deleteSelectedTorrent()
{
    // qBittorrent is not running, so nothing to send
    if (m_qbittorrentHwnd == nullptr) {
        QMessageBox::information(this, QStringLiteral("Delete impossible"),
                                 QStringLiteral("qBittorrent is not running."));
        return;
    }

    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Delete selected torrent :" << torrent.value("name").toString();

    QByteArray infoHash = torrent.value("hash").toByteArray();
    COPYDATASTRUCT torrentInfoHash;
    torrentInfoHash.lpData = infoHash.data();
    torrentInfoHash.cbData = infoHash.size();
    torrentInfoHash.dwData = NULL;
    ::SendMessage(m_qbittorrentHwnd, WM_COPYDATA, (WPARAM) MSG_QMD_DELETE_TORRENT, (LPARAM) (LPVOID) &torrentInfoHash);
}

void MainWindow::showCsfdDetail()
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Show CSFD detail :" << torrent.value("name").toString();
}

void MainWindow::showImdbDetail()
{
    QSqlRecord torrent = getSelectedTorrentRecord();
    if (torrent.isEmpty())
        return;

    qDebug() << "Show IMDB detail :" << torrent.value("name").toString();
}

void MainWindow::updateChangedTorrents(const QVector<QString> &torrentInfoHashes)
{
    foreach (const auto infoHash, torrentInfoHashes)
        m_model->selectRow(dynamic_cast<TorrentSqlTableModel *const>(m_model)
                           ->getTorrentRowByInfoHash(infoHash));
}

void MainWindow::focusSearchFilter()
{
    ui->lineEdit->setFocus();
    ui->lineEdit->selectAll();
}

void MainWindow::focusTableView()
{
    ui->tableView->setFocus();
}
