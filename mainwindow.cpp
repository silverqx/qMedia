#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QLabel>
#include <QScreen>
#include <QShortcut>
#include <QToolButton>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <qt_windows.h>

#include <Psapi.h>

#include <regex>

#include "commonglobal.h"

#include "torrenttransfertableview.h"
#include "version.h"
#include "utils/fs.h"

/*! Order of qBittorrentHwndChanged() or qBittorentUp/Down() slots:
    updateQBittorrentHwnd()
    reloadTorrentModel()
    setGeometry()
    togglePeerColumns()
    resizeColumns()

    There can be more combinations like initial show, when qBittorrent
    is up or down, etc, but order above is crucial.
 */

namespace
{
    // Needed in EnumWindowsProc()
    MainWindow *l_mainWindow = nullptr;

    /*! Main window width by isQBittorrentUp().*/
    const auto mainWindowWidth = [](const bool isQBittorrentUp)
    {
        static const QHash<bool, int> cached {
            {false, 1136},
            {true,  1300},
        };
        return cached.value(isQBittorrentUp);
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM)
    {
        // For reference, very important:
        // https://docs.microsoft.com/en-us/archive/msdn-magazine/2015/july/c-using-stl-strings-at-win32-api-boundaries
        const int windowTextLength = ::GetWindowTextLength(hwnd) + 1;
        auto windowText = std::make_unique<wchar_t[]>(windowTextLength);

        ::GetWindowText(hwnd, windowText.get(), windowTextLength);
#ifdef QMEDIA_DEBUG
        std::wstring text(windowText.get());
#endif

        // Example: [D: 0 B/s, U: 1,3 MiB/s] qBittorrent v4.2.5
        const std::wregex re(L"^(\\[D: .*, U: .*\\] )?qBittorrent "
                             "(v\\d+\\.\\d+\\.\\d+([a-zA-Z]+\\d{0,2})?)$");
        if (!std::regex_match(windowText.get(), re))
            return true;

        DWORD pid;
        ::GetWindowThreadProcessId(hwnd, &pid);
        const HANDLE processHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                                   false, pid);
        if (processHandle == NULL) {
            qDebug() << "OpenProcess() in EnumWindows() failed :"
                     << ::GetLastError();
            return true;
        }

        wchar_t moduleFilePath[MAX_PATH];
        ::GetModuleFileNameEx(processHandle, NULL, moduleFilePath, ARRAYSIZE(moduleFilePath));
        // More instances of qBittorrent can run, so find proper one
#ifdef QMEDIA_DEBUG
        // String has to start with moduleFileName
        if (::wcsstr(moduleFilePath, L"O:\\Code\\c\\qbittorrent_64-dev\\qBittorrent\\qBittorrent-builds")
            != &moduleFilePath[0])
            return true;
#else
        if (::wcsstr(moduleFilePath, L"C:\\Program Files\\qBittorrent") != &moduleFilePath[0])
            return true;
#endif
        const QString moduleFileName =
                Utils::Fs::fileName(QString::fromWCharArray(moduleFilePath));
        // TODO create finally helper https://www.modernescpp.com/index.php/c-core-guidelines-when-you-can-t-throw-an-exception silverqx
        // Or https://www.codeproject.com/Tips/476970/finally-clause-in-Cplusplus
        ::CloseHandle(processHandle);
        if (moduleFileName != "qbittorrent.exe")
            return true;

        qDebug() << "HWND for qBittorrent window found :"
                 << hwnd;

        if (l_mainWindow->getQBittorrentHwnd() != hwnd)
            emit l_mainWindow->qBittorrentHwndChanged(hwnd);

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
    setWindowTitle(QStringLiteral(" qMedia %1").arg(QMEDIA_VERSION_STR_2));
    const QIcon appIcon(QStringLiteral(":/icons/qmedia.svg"));
    setWindowIcon(appIcon);

    connectToDb();

    // Create and initialize widgets
    m_tableView = new TorrentTransferTableView(m_qBittorrentHwnd, ui->centralwidget);
    ui->verticalLayout->addWidget(m_tableView);
    initFilterTorrentsLineEdit();
    // StatusBar
    createStatusBar();

    // Connect events
    connect(this, &MainWindow::qBittorrentHwndChanged, this, &MainWindow::updateQBittorrentHwnd);
    connect(this, &MainWindow::qBittorrentHwndChanged,
            m_tableView, &TorrentTransferTableView::updateqBittorrentHwnd);
    // If qBittorrent was closed, reload model to display updated ETA ∞ and seeds/leechs
    // for every torrent.
    // Order is crucial here.
    // qBittorentDown
    connect(this, &MainWindow::qBittorrentDown,
            m_tableView, &TorrentTransferTableView::reloadTorrentModel);
    connect(this, &MainWindow::qBittorrentDown, this, &MainWindow::setGeometry);
    connect(this, &MainWindow::qBittorrentDown,
            m_tableView, &TorrentTransferTableView::togglePeerColumns);
    /* qBittorentUp - reloadTorrentModel on qBittorrentUp is not needed, it is handled
       in the applicationStateChanged event. */
    connect(this, &MainWindow::qBittorrentUp, this, &MainWindow::setGeometry);
    connect(this, &MainWindow::qBittorrentUp,
            m_tableView, &TorrentTransferTableView::togglePeerColumns);
    // End of crucial order
    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &MainWindow::applicationStateChanged);
    connect(ui->filterTorrentsLineEdit, &QLineEdit::textChanged,
            m_tableView, &TorrentTransferTableView::filterTextChanged);
    connect(ui->reloadTorrentsButton, &QPushButton::clicked,
            m_tableView, &TorrentTransferTableView::reloadTorrentModel);
    connect(this, &MainWindow::torrentsAddedOrRemoved,
            m_tableView, &TorrentTransferTableView::reloadTorrentModel);
    connect(this, &MainWindow::torrentsChanged,
            m_tableView, &TorrentTransferTableView::updateChangedTorrents);

    // Hotkeys
    // global
#ifdef __clang__
#  pragma clang diagnostic push
#  if __has_warning("-Wdeprecated-enum-enum-conversion")
#    pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#  endif
#endif
    const auto *const quitShortcut =
            new QShortcut(Qt::CTRL + Qt::Key_Q, this, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(quitShortcut, &QShortcut::activated,
            qApp, &QCoreApplication::quit, Qt::QueuedConnection);
    // filterTorrentsLineEdit
    const auto *const doubleClickHotkeyF2 =
            new QShortcut(Qt::Key_F2, this, nullptr, nullptr, Qt::WindowShortcut);
    connect(doubleClickHotkeyF2, &QShortcut::activated,
            this, &MainWindow::focusTorrentsFilterLineEdit);
    const auto *const switchSearchFilterShortcut =
            new QShortcut(QKeySequence::Find, this);
    connect(switchSearchFilterShortcut, &QShortcut::activated,
            this, &MainWindow::focusTorrentsFilterLineEdit);
    const auto *const doubleClickHotkeyDown =
            new QShortcut(Qt::Key_Down, ui->filterTorrentsLineEdit, nullptr, nullptr,
                          Qt::WidgetShortcut);
    connect(doubleClickHotkeyDown, &QShortcut::activated,
            m_tableView, qOverload<>(&TorrentTransferTableView::setFocus));
    const auto *const doubleClickHotkeyEsc =
            new QShortcut(Qt::Key_Escape, ui->filterTorrentsLineEdit, nullptr, nullptr,
                          Qt::WidgetShortcut);
    connect(doubleClickHotkeyEsc, &QShortcut::activated,
            ui->filterTorrentsLineEdit, &QLineEdit::clear);
    // Reload model from DB
    const auto *const doubleClickHotkeyF5 =
            new QShortcut(Qt::Key_F5, this, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(doubleClickHotkeyF5, &QShortcut::activated,
            m_tableView, &TorrentTransferTableView::reloadTorrentModel);
    const auto *const doubleClickCtrlR =
            new QShortcut(Qt::CTRL + Qt::Key_R, this, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(doubleClickCtrlR, &QShortcut::activated,
            m_tableView, &TorrentTransferTableView::reloadTorrentModel);
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

    // Tab order
    setTabOrder(m_tableView, ui->filterTorrentsLineEdit);
    setTabOrder(ui->filterTorrentsLineEdit, ui->reloadTorrentsButton);

    // Initial focus
    m_tableView->setFocus();

    // TODO node.exe on path checker in qtimer 1sec after start, may be also nodejs version silverqx
}

MainWindow::~MainWindow()
{
    if (isQBittorrentUp())
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMEDIA_DOWN, NULL, NULL);

    delete ui;
}

MainWindow *MainWindow::instance()
{
    return l_mainWindow;
}

void MainWindow::show()
{
    // I had to put this code outside from ctor, to prevent clazy-incorrect-emit
    // Find qBittorent's main window HWND
    ::EnumWindows(EnumWindowsProc, NULL);
    // Send hwnd of MainWindow to qBittorrent, aka. inform that qMedia is running
    if (isQBittorrentUp()) {
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMEDIA_UP, (WPARAM) winId(), NULL);
        emit qBittorrentUp(true);
    } else
        emit qBittorrentDown(true);

    QMainWindow::show();
}

void MainWindow::refreshStatusBar() const
{
    m_torrentsCountLabel->setText(QStringLiteral("Torrents: <strong>%1</strong>")
                                  .arg(selectTorrentsCount()));
    m_torrentFilesCountLabel->setText(QStringLiteral("Video Files: <strong>%1</strong>")
                                      .arg(selectTorrentFilesCount()));
}

void MainWindow::applicationStateChanged(Qt::ApplicationState state) const
{
    // Disable autoreload in development
#ifdef QMEDIA_NO_DEBUG
    if (state == Qt::ApplicationActive)
        m_tableView->reloadTorrentModel();
#endif

    // TODO message updates only when fully invisible silverqx
    /* Here is more advanced solution to decide, if window is partially visible, I can
       use this to decide, if window is fully invisible, so updates will keep running if
       partially visible and updates will be disabled, if fully invisible:
       https://stackoverflow.com/questions/3154214/can-i-detect-if-a-window-is-partly-hidden */

    // Inform qBittorrent about qMedia is in foreground
    if (state == Qt::ApplicationActive)
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMD_APPLICATION_ACTIVE, NULL, NULL);
    if ((state == Qt::ApplicationInactive) || (state == Qt::ApplicationSuspended)
        || (state == Qt::ApplicationHidden))
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMD_APPLICATION_DEACTIVE, NULL, NULL);
}

void MainWindow::setGeometry(const bool initial)
{
#ifdef LOG_GEOMETRY
    qDebug("setGeometry(initial = %s)", initial ? "true" : "false");
#endif

#ifdef VISIBLE_CONSOLE
    // Set up smaller, so I can see console output in the QtCreator, but only at initial
    resize(mainWindowWidth(isQBittorrentUp()),
           initial ? (geometry().height() - 200) : geometry().height());
#else
    resize(mainWindowWidth(isQBittorrentUp()), geometry().height());
#endif
    // Initial position
    move(screen()->availableSize().width() - width() - 10, 10);
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
        qDebug() << "Connect to database failed :"
                 << db.lastError().text();
}

void MainWindow::initFilterTorrentsLineEdit()
{
    m_searchButton = new QToolButton(ui->filterTorrentsLineEdit);
    const QIcon searchIcon(QStringLiteral(":/icons/search_w.svg"));
    m_searchButton->setIcon(searchIcon);
    m_searchButton->setCursor(Qt::ArrowCursor);
    m_searchButton->setStyleSheet(
                QStringLiteral("QToolButton {border: none; padding: 2px 0 2px 4px;}"));
    m_searchButton->setFocusPolicy(Qt::NoFocus);

    // Padding between text and widget borders
    ui->filterTorrentsLineEdit->setStyleSheet(QStringLiteral("QLineEdit {padding-left: %1px;}")
                                              .arg(m_searchButton->sizeHint().width()));

    const int frameWidth = ui->filterTorrentsLineEdit->style()
                           ->pixelMetric(QStyle::PM_DefaultFrameWidth);
    ui->filterTorrentsLineEdit->setMaximumHeight(
                std::max(ui->filterTorrentsLineEdit->sizeHint().height(),
                         m_searchButton->sizeHint().height()) + (frameWidth * 2));
    ui->filterTorrentsLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

namespace
{
    const auto QB_CONNECTED_TMPL = QStringLiteral("<span style='color: %1;'>•</span>");
    const auto QB_CONNECTED_TOOLTIP_TMPL =
            QStringLiteral("qBittorrent&nbsp;is&nbsp;<strong>%1</strong>");
} // namespace

void MainWindow::createStatusBar()
{
    auto *const container = new QWidget(this);
    auto *const layout = new QHBoxLayout(container);
    layout->setContentsMargins(11, 0, 16, 5);
    container->setLayout(layout);

    // Create widgets displayed in the statusbar
    m_torrentsCountLabel = new QLabel(QStringLiteral("Torrents: <strong>%1</strong>")
                                      .arg(m_tableView->getModelRowCount()), this);
    m_torrentFilesCountLabel = new QLabel(QStringLiteral("Video Files: <strong>%1</strong>")
                                          .arg(selectTorrentFilesCount()), this);
    m_qBittorrentConnectedLabel = new QLabel(QB_CONNECTED_TMPL.arg("#d65645"), this);

    connect(this, &MainWindow::torrentsAddedOrRemoved, this, &MainWindow::refreshStatusBar);
    connect(this, &MainWindow::qBittorrentDown, this, &MainWindow::qBittorrentDisconnected);
    connect(this, &MainWindow::qBittorrentUp, this, &MainWindow::qBittorrentConnected);

    // TODO when I set font size on qapplication instance, then font is not inherited from containers? I can not set font size on container silverqx
    // Set smaller font size by 1pt
    QFont font = m_torrentsCountLabel->font();
    font.setPointSize(8);
    m_torrentsCountLabel->setFont(font);
    m_torrentFilesCountLabel->setFont(font);
    font.setBold(true);
    m_qBittorrentConnectedLabel->setFont(font);
    m_qBittorrentConnectedLabel->setToolTip(QB_CONNECTED_TOOLTIP_TMPL.arg("Disconnected"));

    // Create needed splitters
    auto *const splitter1 = new QFrame(statusBar());
    splitter1->setFrameStyle(QFrame::VLine);
    splitter1->setFrameShadow(QFrame::Plain);
    // Make splitter little darker
    splitter1->setStyleSheet("QFrame { color: #8c8c8c; }");

    auto *const splitter2 = new QFrame(statusBar());
    splitter2->setFrameStyle(QFrame::VLine);
    splitter2->setFrameShadow(QFrame::Plain);
    // Make splitter little darker
    splitter2->setStyleSheet("QFrame { color: #8c8c8c; }");

    layout->addWidget(m_torrentsCountLabel);
    layout->addWidget(splitter1);
    layout->addWidget(m_torrentFilesCountLabel);
    layout->addWidget(splitter2);
    layout->addWidget(m_qBittorrentConnectedLabel);

    statusBar()->addPermanentWidget(container);
}

quint64 MainWindow::selectTorrentsCount() const
{
    QSqlQuery query;
    query.setForwardOnly(true);

    const bool ok = query.exec("SELECT COUNT(*) as count FROM torrents");
    if (!ok) {
        qDebug() << "Select of torrents count failed :"
                 << query.lastError().text();
        return 0;
    }

    query.first();
    return query.value(0).toULongLong();
}

quint64 MainWindow::selectTorrentFilesCount() const
{
    QSqlQuery query;
    query.setForwardOnly(true);

    const bool ok = query.exec("SELECT COUNT(*) as count FROM torrent_previewable_files");
    if (!ok) {
        qDebug() << "Select of torrent files count failed :"
                 << query.lastError().text();
        return 0;
    }

    query.first();
    return query.value(0).toULongLong();
}

void MainWindow::focusTorrentsFilterLineEdit() const
{
    ui->filterTorrentsLineEdit->setFocus();
    ui->filterTorrentsLineEdit->selectAll();
}

void MainWindow::qBittorrentConnected() const
{
    m_qBittorrentConnectedLabel->setText(QB_CONNECTED_TMPL.arg("#6fac3d"));
    m_qBittorrentConnectedLabel->setToolTip(QB_CONNECTED_TOOLTIP_TMPL.arg("Connected"));
}

void MainWindow::qBittorrentDisconnected() const
{
    m_qBittorrentConnectedLabel->setText(QB_CONNECTED_TMPL.arg("#d65645"));
    m_qBittorrentConnectedLabel->setToolTip(QB_CONNECTED_TOOLTIP_TMPL.arg("Disconnected"));
}
