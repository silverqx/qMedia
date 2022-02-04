#include "previewselectdialog.h"
#include "ui_previewselectdialog.h"

#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QtSql/QSqlRecord>

#include "common.h"
#include "previewlistdelegate.h"
// TODO get rid of this include, move Column enum to separate file silverqx
#include "torrentsqltablemodel.h"
#include "utils/fs.h"
#include "utils/misc.h"

PreviewSelectDialog::PreviewSelectDialog(
        QWidget *const parent, const QSqlRecord torrent,
        const QSharedPointer<const QVector<QSqlRecord>> &torrentFiles
)
    : QDialog(parent)
    , m_ui(new Ui::PreviewSelectDialog)
    , m_torrent(torrent)
    , m_torrentFiles(torrentFiles)
{
    m_ui->setupUi(this);

    m_ui->infoLabel->setText(QStringLiteral("The following files from torrent <strong>%1</strong> "
                                          "support previewing, please select one of them:")
                       .arg(m_torrent.value("name").toString()));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("&Preview"));

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted,
            this, &PreviewSelectDialog::previewButtonClicked);
    connect(m_ui->previewList, &QAbstractItemView::doubleClicked,
            this, &PreviewSelectDialog::previewButtonClicked);

    // Create and apply delegate
    m_listDelegate = new PreviewListDelegate(this);
    m_ui->previewList->setItemDelegate(m_listDelegate);

    // Preview list model
    m_previewListModel = new QStandardItemModel(0, NB_COLUMNS, this);
    m_previewListModel->setHeaderData(TR_NAME, Qt::Horizontal, QStringLiteral("Name"));
    m_previewListModel->setHeaderData(TR_SIZE, Qt::Horizontal, QStringLiteral("Size"));
    m_previewListModel->setHeaderData(TR_PROGRESS, Qt::Horizontal, QStringLiteral("Progress"));

    // Preview list
    m_ui->previewList->setModel(m_previewListModel);
    m_ui->previewList->hideColumn(TR_FILE_INDEX);
    m_ui->previewList->setAlternatingRowColors(true);

    populatePreviewListModel();

    // Setup initial sorting
    m_previewListModel->sort(TR_NAME);
    m_ui->previewList->header()->setSortIndicator(0, Qt::AscendingOrder);

    // Header alignment
    m_ui->previewList->header()->setDefaultAlignment(Qt::AlignCenter);

    // TODO set height on the base of num rows, set min. and max. height silverqx

    // Initial focus
    m_ui->previewList->setFocus();
    // Preselect first line
    m_ui->previewList->selectionModel()->select(
                m_previewListModel->index(0, TR_NAME),
                QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

PreviewSelectDialog::~PreviewSelectDialog()
{
    delete m_ui;
}

void PreviewSelectDialog::previewButtonClicked()
{
    // Only one file is allowed to select
    const auto selectedIndexes = m_ui->previewList->selectionModel()->selectedRows(TR_NAME);
    if (selectedIndexes.isEmpty())
        return;
    const auto selectedIndex = selectedIndexes.first();
    if (!selectedIndex.isValid())
        return;

    // BUG can't preview if saveing incomplete directory is provided, I don't even have incomplete folder in torrents table, I have savepath column only 😭 silverqx
    // Get file path to preview
    const auto filePath = getTorrentFileFilePathAbs(
                              m_previewListModel->data(selectedIndex).toString());

    if (!QFile::exists(filePath)) {
        QMessageBox::critical(this, QStringLiteral("Preview impossible"),
                              QStringLiteral("Sorry, we can't preview this file:<br>"
                                             "<strong>%1</strong>")
                              .arg(Utils::Fs::toNativePath(filePath)));
        reject();
    }

    emit readyToPreviewFile(filePath);
    accept();
}

void PreviewSelectDialog::showEvent(QShowEvent *event)
{
    // Event originated from system
    if (event->spontaneous())
        return QDialog::showEvent(event);

    if (m_showEventInitialized)
        return;

    // Pixel perfectly sized previewList header
    // Set Name column width to all remaining area
    // Have to be called after show(), because previewList width is needed
    auto *const previewListHeader = m_ui->previewList->header();
    previewListHeader->resizeSections(QHeaderView::ResizeToContents);

    // Compute name column width
    auto nameColWidth = m_ui->previewList->width();
    const auto *const vScrollBar = m_ui->previewList->verticalScrollBar();
    if (vScrollBar->isVisible())
        nameColWidth -= vScrollBar->width();
    nameColWidth -= previewListHeader->sectionSize(TR_SIZE);
    nameColWidth -= previewListHeader->sectionSize(TR_PROGRESS);
    nameColWidth -= 2; // Borders

    previewListHeader->resizeSection(TR_NAME, nameColWidth);
    m_ui->previewList->header()->setStretchLastSection(true);

    m_showEventInitialized = true;
}

void PreviewSelectDialog::populatePreviewListModel() const
{
    // It is a const iterator
    QVectorIterator<QSqlRecord> itTorrentFiles(*m_torrentFiles);
    QString filePath;
    int rowCount;
    QSqlRecord torrentFile;
    while (itTorrentFiles.hasNext()) {
        torrentFile = itTorrentFiles.next();
        filePath = torrentFile.value("filepath").toString();
        // Remove qBittorrent ext when needed
        if (filePath.endsWith(::QB_EXT))
            filePath.chop(4);

        // Insert new row
        rowCount = m_previewListModel->rowCount();
        m_previewListModel->insertRow(rowCount);

        // Setup row data
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_NAME),
                                    filePath);
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_NAME),
                                    getTorrentFileFilePathAbs(filePath),
                                    Qt::ToolTipRole);
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_SIZE),
                                    torrentFile.value("size").toULongLong());
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_PROGRESS),
                                    torrentFile.value("progress").toUInt());
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_FILE_INDEX),
                                    torrentFile.value("id").toULongLong());
    }
}

QString PreviewSelectDialog::getTorrentFileFilePathAbs(const QString &relativePath) const
{
    const QDir saveDir(m_torrent.value(TorrentSqlTableModel::TR_SAVE_PATH).toString());

    return Utils::Fs::expandPathAbs(saveDir.absoluteFilePath(relativePath));
}
