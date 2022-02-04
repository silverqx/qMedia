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

#include <array>
#include <regex>

#include "commonglobal.h"

#include "torrenttransfertableview.h"
#include "utils/fs.h"
#include "version.h"

/* Required order of qBittorrentHwndChanged() or qBittorentUp/Down() slots:
   updateqBittorrentHwnd()
   reloadTorrentModel()
   setGeometry()
   togglePeerColumns()
   resizeColumns()

   There can be more combinations like an initial show and when qBittorrent is up or down,
   etc, but the above order is crucial. */

namespace
{
    // Needed in EnumWindowsProc()
    MainWindow *l_mainWindow = nullptr;

    /*! Main window width by isqBittorrentUp().*/
    const auto mainWindowWidth = [](const auto isqBittorrentUp)
    {
        static const QHash<bool, int> cached {
            {false, 1136},
            {true,  1300},
        };

        return cached.value(isqBittorrentUp);
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM /*unused*/)
    {
        // Obtain window title directly to the std::wstring
        // For reference, very important:
        // https://docs.microsoft.com/en-us/archive/msdn-magazine/2015/july/c-using-stl-strings-at-win32-api-boundaries
        const int windowTextLength = ::GetWindowTextLength(hwnd) + 1;
        std::wstring windowText;
        windowText.resize(windowTextLength);

        ::GetWindowText(hwnd, windowText.data(), windowTextLength);
        // Resize down the string to avoid bogus double-NUL-terminated strings
        windowText.resize(windowTextLength - 1);

        // Example: [D: 0 B/s, U: 1,3 MiB/s] qBittorrent v4.2.5
        const std::wregex re(L"^(\\[D: .*, U: .*\\] )?qBittorrent "
                             "(v\\d+\\.\\d+\\.\\d+([a-zA-Z]+\\d{0,2})?)$");
        if (!std::regex_match(windowText, re))
            return true; // NOLINT(readability-implicit-bool-conversion)

        // Obtain PID
        DWORD pid = 0;
        ::GetWindowThreadProcessId(hwnd, &pid);
        HANDLE processHandle =
                // NOLINTNEXTLINE(readability-implicit-bool-conversion)
                ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
        if (processHandle == nullptr) {
            qDebug() << "OpenProcess() in EnumWindows() failed :"
                     << ::GetLastError();
            return true; // NOLINT(readability-implicit-bool-conversion)
        }

        // Obtain module filepath
        std::array<wchar_t, MAX_PATH> moduleFilePath {NULL};
        ::GetModuleFileNameEx(processHandle, nullptr, moduleFilePath.data(),
                              static_cast<DWORD>(moduleFilePath.size()));
        // More instances of qBittorrent can run, so find proper one
#ifdef QMEDIA_DEBUG
        // String has to start with moduleFileName
        if (::wcsstr(&moduleFilePath[0],
                     L"O:\\Code\\c\\qbittorrent_64-dev\\qBittorrent\\qBittorrent-builds")
            != &moduleFilePath[0]
        )
            return true; // NOLINT(readability-implicit-bool-conversion)
#else
        if (::wcsstr(&moduleFilePath[0], L"C:\\Program Files\\qBittorrent") != &moduleFilePath[0])
            return true; // NOLINT(readability-implicit-bool-conversion)
#endif
        // Obtain module filename
        const auto moduleFileName = Utils::Fs::fileName(
                                        QString::fromWCharArray(moduleFilePath.data()));
        // TODO create finally helper https://www.modernescpp.com/index.php/c-core-guidelines-when-you-can-t-throw-an-exception silverqx
        // Or https://www.codeproject.com/Tips/476970/finally-clause-in-Cplusplus
        ::CloseHandle(processHandle);

        // Compare module filename
        if (moduleFileName != "qbittorrent.exe")
            return true; // NOLINT(readability-implicit-bool-conversion)

        if (l_mainWindow == nullptr) {
            qDebug() << "l_mainWindow == nullptr";

            return false; // NOLINT(readability-implicit-bool-conversion)
        }

        qDebug() << "HWND for qBittorrent window found :"
                 << hwnd;

        // Handle a new HWND
        if (l_mainWindow->qBittorrentHwnd() != hwnd)
            emit l_mainWindow->qBittorrentHwndChanged(hwnd);

        // Done
        return false; // NOLINT(readability-implicit-bool-conversion)
    }
} // namespace

MainWindow::MainWindow(QWidget *const parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
{
    l_mainWindow = this;

    m_ui->setupUi(this);

    // MainWindow
    setWindowTitle(QStringLiteral(" qMedia %1").arg(QMEDIA_VERSION_STR_2));
    const QIcon appIcon(QStringLiteral(":/icons/qmedia.svg"));
    setWindowIcon(appIcon);

    connectToDb();

    // Create and initialize widgets
    // Main torrent transfer view
    m_tableView = new TorrentTransferTableView(m_qBittorrentHwnd, m_ui->centralwidget); // NOLINT(cppcoreguidelines-owning-memory)
    m_ui->verticalLayout->addWidget(m_tableView);
    // Searchbox
    initFilterTorrentsLineEdit();
    // StatusBar
    createStatusBar();

    // Connect events
    connect(this, &MainWindow::qBittorrentHwndChanged, this, &MainWindow::updateqBittorrentHwnd);
    connect(this, &MainWindow::qBittorrentHwndChanged,
            m_tableView, &TorrentTransferTableView::updateqBittorrentHwnd);
    /* If qBittorrent was closed, reload the model to display updated ETA ∞ and seeds/leechers
       for every torrent. Order is crucial here! */
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
    connect(qApp, &QGuiApplication::applicationStateChanged, // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
            this, &MainWindow::applicationStateChanged);
    connect(m_ui->filterTorrentsLineEdit, &QLineEdit::textChanged,
            m_tableView, &TorrentTransferTableView::filterTextChanged);
    connect(m_ui->reloadTorrentsButton, &QPushButton::clicked,
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
            qApp, &QCoreApplication::quit, Qt::QueuedConnection); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
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
            new QShortcut(Qt::Key_Down, m_ui->filterTorrentsLineEdit, nullptr, nullptr,
                          Qt::WidgetShortcut);
    connect(doubleClickHotkeyDown, &QShortcut::activated,
            m_tableView, qOverload<>(&TorrentTransferTableView::setFocus));
    const auto *const doubleClickHotkeyEsc =
            new QShortcut(Qt::Key_Escape, m_ui->filterTorrentsLineEdit, nullptr, nullptr,
                          Qt::WidgetShortcut);
    connect(doubleClickHotkeyEsc, &QShortcut::activated,
            m_ui->filterTorrentsLineEdit, &QLineEdit::clear);
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
    setTabOrder(m_tableView, m_ui->filterTorrentsLineEdit);
    setTabOrder(m_ui->filterTorrentsLineEdit, m_ui->reloadTorrentsButton);

    // Initial focus
    m_tableView->setFocus();

    // TODO node.exe on path checker in qtimer 1sec after start, may be also nodejs version silverqx
}

MainWindow::~MainWindow() noexcept
{
    if (isqBittorrentUp())
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMEDIA_DOWN, NULL, NULL);
}

void MainWindow::show()
{
    // I had to put this code outside from ctor, to prevent clazy-incorrect-emit
    // Find qBittorent's main window HWND
    ::EnumWindows(EnumWindowsProc, NULL);

    // Send hwnd of MainWindow to qBittorrent, aka. inform that qMedia is running
    if (isqBittorrentUp()) {
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMEDIA_UP, static_cast<WPARAM>(winId()), NULL);
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

    // qMedia in background
    else if (state == Qt::ApplicationInactive || state == Qt::ApplicationSuspended ||
             state == Qt::ApplicationHidden
    )
        ::PostMessage(m_qBittorrentHwnd, ::MSG_QMD_APPLICATION_DEACTIVE, NULL, NULL);
}

void MainWindow::setGeometry(const bool initial)
{
#ifdef QMEDIA_NO_DEBUG
    Q_UNUSED(initial)
#endif

#ifdef LOG_GEOMETRY
    qDebug("setGeometry(initial = %s)", initial ? "true" : "false");
#endif

#ifdef VISIBLE_CONSOLE
    // Set up smaller, so I can see console output in the QtCreator, but only at initial
    resize(mainWindowWidth(isqBittorrentUp()),
           initial ? (geometry().height() - 200) : geometry().height());
#else
    resize(mainWindowWidth(isqBittorrentUp()), geometry().height());
#endif

    // Initial position to the top right corner
    move(screen()->availableSize().width() - width() - 10, 10);
}

void MainWindow::connectToDb() const
{
    // BUG remove from github and use values from env. silverqx
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
        qDebug().noquote() << "Connect to database failed :"
                           << db.lastError().text();
}

void MainWindow::initFilterTorrentsLineEdit()
{
    // Nice icon in the left
    m_searchButton = new QToolButton(m_ui->filterTorrentsLineEdit); // NOLINT(cppcoreguidelines-owning-memory)
    const QIcon searchIcon(QStringLiteral(":/icons/search_w.svg"));
    m_searchButton->setIcon(searchIcon);
    m_searchButton->setCursor(Qt::ArrowCursor);
    m_searchButton->setStyleSheet(
                QStringLiteral("QToolButton {border: none; padding: 2px 0 2px 4px;}"));
    m_searchButton->setFocusPolicy(Qt::NoFocus);

    // Padding between text and widget borders
    m_ui->filterTorrentsLineEdit->setStyleSheet(
                QStringLiteral("QLineEdit {padding-left: %1px;}")
                .arg(m_searchButton->sizeHint().width()));

    const int frameWidth = m_ui->filterTorrentsLineEdit->style()
                           ->pixelMetric(QStyle::PM_DefaultFrameWidth);
    m_ui->filterTorrentsLineEdit->setMaximumHeight(
                std::max(m_ui->filterTorrentsLineEdit->sizeHint().height(),
                         m_searchButton->sizeHint().height()) + (frameWidth * 2));
    m_ui->filterTorrentsLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

namespace
{
    const auto QB_CONNECTED_TMPL = QStringLiteral("<span style='color: %1;'>•</span>");
    const auto QB_CONNECTED_TOOLTIP_TMPL =
            QStringLiteral("qBittorrent&nbsp;is&nbsp;<strong>%1</strong>");
} // namespace

void MainWindow::createStatusBar()
{
    // Create layout
    auto *const container = new QWidget(this); // NOLINT(cppcoreguidelines-owning-memory)
    auto *const layout = new QHBoxLayout(container); // NOLINT(cppcoreguidelines-owning-memory)
    layout->setContentsMargins(11, 0, 16, 5);
    container->setLayout(layout);

    // Create widgets displayed in the statusbar
    m_torrentsCountLabel = new QLabel(QStringLiteral("Torrents: <strong>%1</strong>") // NOLINT(cppcoreguidelines-owning-memory)
                                      .arg(m_tableView->getModelRowCount()), this);
    m_torrentFilesCountLabel = new QLabel(QStringLiteral("Video Files: <strong>%1</strong>") // NOLINT(cppcoreguidelines-owning-memory)
                                          .arg(selectTorrentFilesCount()), this);
    m_qBittorrentConnectedLabel = new QLabel(QB_CONNECTED_TMPL.arg("#d65645"), this); // NOLINT(cppcoreguidelines-owning-memory)

    // Events
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

    // Others
    m_qBittorrentConnectedLabel->setToolTip(QB_CONNECTED_TOOLTIP_TMPL.arg("Disconnected"));

    // Create needed splitters
    auto *const splitter1 = new QFrame(statusBar()); // NOLINT(cppcoreguidelines-owning-memory)
    splitter1->setFrameStyle(QFrame::VLine);
    splitter1->setFrameShadow(QFrame::Plain);
    // Make splitter little darker
    splitter1->setStyleSheet("QFrame { color: #8c8c8c; }");

    auto *const splitter2 = new QFrame(statusBar()); // NOLINT(cppcoreguidelines-owning-memory)
    splitter2->setFrameStyle(QFrame::VLine);
    splitter2->setFrameShadow(QFrame::Plain);
    // Make splitter little darker
    splitter2->setStyleSheet("QFrame { color: #8c8c8c; }");

    // Add to the layout
    layout->addWidget(m_torrentsCountLabel);
    layout->addWidget(splitter1);
    layout->addWidget(m_torrentFilesCountLabel);
    layout->addWidget(splitter2);
    layout->addWidget(m_qBittorrentConnectedLabel);

    // Done
    statusBar()->addPermanentWidget(container);
}

quint64 MainWindow::selectTorrentsCount() const
{
    QSqlQuery query;
    query.setForwardOnly(true);

    const bool ok = query.exec(QStringLiteral("SELECT COUNT(*) as count FROM torrents"));
    if (!ok) {
        qDebug().noquote() << "Select of torrents count failed :"
                           << query.lastError().text();
        return 0;
    }

    query.first();
    return query.value("count").toULongLong();
}

quint64 MainWindow::selectTorrentFilesCount() const
{
    QSqlQuery query;
    query.setForwardOnly(true);

    const bool ok = query.exec("SELECT COUNT(*) as count FROM torrent_previewable_files");
    if (!ok) {
        qDebug().noquote() << "Select of torrent files count failed :"
                           << query.lastError().text();
        return 0;
    }

    query.first();
    return query.value("count").toULongLong();
}

void MainWindow::focusTorrentsFilterLineEdit() const
{
    m_ui->filterTorrentsLineEdit->setFocus();
    m_ui->filterTorrentsLineEdit->selectAll();
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
