#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>

class QLabel;
class QSortFilterProxyModel;
class QSqlRecord;
class QSqlTableModel;
class QToolButton;

class TorrentTableDelegate;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setQBittorrentHwnd(const HWND hwnd);

public slots:
    void previewFile(const QString &filePath);

signals:
    void torrentsAddedOrRemoved();
    void torrentsChanged(const QVector<QString> &torrentInfoHashes);

protected:
    void showEvent(QShowEvent *event) override;
    bool event(QEvent *event) override;

private:
    enum TorrentColumn
    {
        TR_ID,
        TR_NAME,
        TR_SIZE,
        TR_PROGRESS,
        TR_ADDED_ON,
        TR_HASH,

        NB_COLUMNS
    };

    void connectToDb() const;
    void initTorrentTableView();
    void initFilterLineEdit();
    QVector<QSqlRecord> *selectTorrentFilesById(quint64 id);
    void createStatusBar();
    uint selectTorrentsCount() const;
    uint selectTorrentFilesCount() const;
    QModelIndex getSelectedTorrentIndex() const;
    QSqlRecord getSelectedTorrentRecord() const;

    Ui::MainWindow *ui;
    QSqlTableModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    TorrentTableDelegate *m_tableDelegate;
    /*!
       \brief Contains torrent files by torrent id.
     */
    QHash<quint64, QVector<QSqlRecord> *> m_torrentFilesCache;
    QLabel *m_torrentsCountLabel;
    QLabel *m_torrentFilesCountLabel;
    QToolButton *m_searchButton;
    bool m_showEventInitialized = false;
    HWND m_qbittorrentHwnd = nullptr;

private slots:
    void filterTextChanged(const QString &name);
    void previewSelectedTorrent();
    void focusSearchFilter();
    void focusTableView();
    void refreshStatusBar();
    void reloadTorrentModel();
    void displayListMenu(const QPoint &);
    void deleteSelectedTorrent();
    void showCsfdDetail();
    void showImdbDetail();
    void updateChangedTorrents(const QVector<QString> &torrentInfoHashes);
};

#endif // MAINWINDOW_H
