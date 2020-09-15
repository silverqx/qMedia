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

    static MainWindow *instance();

signals:
    void torrentsAddedOrRemoved();
    void torrentsChanged(const QVector<QString> &torrentInfoHashes);
    void qBittorrentHwndChanged(const HWND hwnd);

private:
    void connectToDb() const;
    void initFilterTorrentsLineEdit();
    void createStatusBar();
    uint selectTorrentsCount() const;
    uint selectTorrentFilesCount() const;
    bool isQBittorrentUp() const { return (m_qBittorrentHwnd != nullptr); };

    Ui::MainWindow *ui;
    QLabel *m_torrentsCountLabel;
    QLabel *m_torrentFilesCountLabel;
    QToolButton *m_searchButton;
    HWND m_qBittorrentHwnd = nullptr;
    TorrentTransferTableView *m_tableView;

private slots:
    void focusTorrentsFilterLineEdit();
    void refreshStatusBar();
    void updateQBittorrentHwnd(const HWND hwnd) { m_qBittorrentHwnd = hwnd; };
    void applicationStateChanged(Qt::ApplicationState state);
};

#endif // MAINWINDOW_H
