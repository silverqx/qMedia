#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

class QLabel;
class QToolButton;

class TorrentTransferTableView;

namespace Ui
{
    class MainWindow;
}

class MainWindow final : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    /*! Constructor. */
    explicit MainWindow(QWidget *parent = nullptr);
    /*! Virtual destructor. */
    ~MainWindow() noexcept final;

    inline HWND qBittorrentHwnd() const noexcept;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
    void show();

signals:
    void torrentsAddedOrRemoved();
    void torrentsChanged(const QVector<QString> &torrentInfoHashes);
    void qBittorrentHwndChanged(HWND hwnd);
    void qBittorrentUp(bool initial = false);
    void qBittorrentDown(bool initial = false);

private:
    void connectToDb() const;
    void initFilterTorrentsLineEdit();
    void createStatusBar();
    quint64 selectTorrentsCount() const;
    quint64 selectTorrentFilesCount() const;
    inline bool isqBittorrentUp() const noexcept;

    HWND m_qBittorrentHwnd = nullptr;

    std::unique_ptr<Ui::MainWindow> m_ui;
    QPointer<QLabel> m_torrentsCountLabel;
    QPointer<QLabel> m_torrentFilesCountLabel;
    QPointer<QLabel> m_qBittorrentConnectedLabel;
    QPointer<QToolButton> m_searchButton;
    QPointer<TorrentTransferTableView> m_tableView;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
private slots:
    void focusTorrentsFilterLineEdit() const;
    void refreshStatusBar() const;
    inline void updateqBittorrentHwnd(HWND hwnd) noexcept;
    void applicationStateChanged(Qt::ApplicationState state) const;
    void setGeometry(bool initial = false);
    void qBittorrentConnected() const;
    void qBittorrentDisconnected() const;
};

HWND MainWindow::qBittorrentHwnd() const noexcept
{
    return m_qBittorrentHwnd;
}

bool MainWindow::isqBittorrentUp() const noexcept
{
    return m_qBittorrentHwnd != nullptr;
}

void MainWindow::updateqBittorrentHwnd(HWND hwnd) noexcept
{
    m_qBittorrentHwnd = hwnd;
}

#endif // MAINWINDOW_H
