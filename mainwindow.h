#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;
class QToolButton;

class TorrentTransferTableView;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    inline HWND getQBittorrentHwnd() const noexcept { return m_qBittorrentHwnd; }

    static MainWindow *instance();

public slots:
    void show();

signals:
    void torrentsAddedOrRemoved();
    void torrentsChanged(const QVector<QString> &torrentInfoHashes);
    void qBittorrentHwndChanged(const HWND hwnd);
    void qBittorrentUp(bool initial = false);
    void qBittorrentDown(bool initial = false);

private:
    void connectToDb() const;
    void initFilterTorrentsLineEdit();
    void createStatusBar();
    quint64 selectTorrentsCount() const;
    quint64 selectTorrentFilesCount() const;
    inline bool isQBittorrentUp() const noexcept { return (m_qBittorrentHwnd != nullptr); };

    Ui::MainWindow *ui;
    QLabel *m_torrentsCountLabel;
    QLabel *m_torrentFilesCountLabel;
    QLabel *m_qBittorrentConnectedLabel;
    QToolButton *m_searchButton;
    HWND m_qBittorrentHwnd = nullptr;
    TorrentTransferTableView *m_tableView;

private slots:
    void focusTorrentsFilterLineEdit() const;
    void refreshStatusBar() const;
    void updateQBittorrentHwnd(const HWND hwnd) noexcept { m_qBittorrentHwnd = hwnd; };
    void applicationStateChanged(Qt::ApplicationState state) const;
    void setGeometry(bool initial = false);
    void qBittorrentConnected() const;
    void qBittorrentDisconnected() const;
};

#endif // MAINWINDOW_H
