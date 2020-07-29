#include "previewselectdialog.h"
#include "ui_previewselectdialog.h"

#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QtSql/QSqlRecord>
#include <QStandardItemModel>

#include "common.h"
#include "utils/fs.h"
#include "utils/misc.h"

PreviewSelectDialog::PreviewSelectDialog(QWidget *parent, const QSqlRecord torrent,
                                         const QVector<QSqlRecord> *const torrentFiles)
    : QDialog(parent)
    , ui(new Ui::PreviewSelectDialog)
    , m_torrent(torrent)
    , m_torrentFiles(torrentFiles)
{
    ui->setupUi(this);

    // Remove help button
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->label->setText(QString("The following files from torrent <strong>%1</strong> support "
                               "previewing, please select one of them:")
                       .arg(m_torrent.value("name").toString()));

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Preview"));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PreviewSelectDialog::previewButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->previewList, &QAbstractItemView::doubleClicked, this, &PreviewSelectDialog::previewButtonClicked);

    // Preview list model
    m_previewListModel = new QStandardItemModel(0, NB_COLUMNS, this);
    m_previewListModel->setHeaderData(TR_NAME, Qt::Horizontal, QStringLiteral("Name"));
    m_previewListModel->setHeaderData(TR_SIZE, Qt::Horizontal, QStringLiteral("Size"));
    m_previewListModel->setHeaderData(TR_PROGRESS, Qt::Horizontal, QStringLiteral("Progress"));

    // Preview list
    ui->previewList->setModel(m_previewListModel);
    ui->previewList->hideColumn(TR_FILE_INDEX);
    ui->previewList->setAlternatingRowColors(true);

    populatePreviewListModel();

    // Setup initial sorting
    m_previewListModel->sort(TR_NAME);
    ui->previewList->header()->setSortIndicator(0, Qt::AscendingOrder);

    // Initial focus
    ui->previewList->setFocus();
    // Preselect first line
    ui->previewList->selectionModel()->select(
                m_previewListModel->index(0, TR_NAME),
                QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

PreviewSelectDialog::~PreviewSelectDialog()
{
    delete ui;
}

void PreviewSelectDialog::previewButtonClicked()
{
    QModelIndexList selectedIndexes = ui->previewList->selectionModel()->selectedRows(TR_NAME);
    if (selectedIndexes.isEmpty())
        return;
    QModelIndex selectedIndex = selectedIndexes.first();
    if (!selectedIndex.isValid())
        return;

    // Only one file is allowed to select
    const QString path = m_previewListModel->data(selectedIndex).toString();
    if (!QFile::exists(path)) {
        QMessageBox::critical(this, QStringLiteral("Preview impossible"),
                              QString("Sorry, we can't preview this file:\n%1.")
                              .arg(Utils::Fs::toNativePath(path)));
        reject();
    }

    emit readyToPreviewFile(path);
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
    QHeaderView *const previewListHeader = ui->previewList->header();
    previewListHeader->resizeSections(QHeaderView::ResizeToContents);

    // Compute name column width
    int nameColWidth = ui->previewList->width();
    const QScrollBar *const vScrollBar = ui->previewList->verticalScrollBar();
    if (vScrollBar->isVisible())
        nameColWidth -= vScrollBar->width();
    nameColWidth -= previewListHeader->sectionSize(TR_SIZE);
    nameColWidth -= previewListHeader->sectionSize(TR_PROGRESS);
    nameColWidth -= 2; // Borders

    previewListHeader->resizeSection(TR_NAME, nameColWidth);
    ui->previewList->header()->setStretchLastSection(true);

    m_showEventInitialized = true;
}

void PreviewSelectDialog::populatePreviewListModel()
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
        if (filePath.endsWith(QB_EXT))
            filePath.chop(4);

        // Insert new row
        rowCount = m_previewListModel->rowCount();
        m_previewListModel->insertRow(rowCount);

        // Setup row data
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_NAME),
                                    filePath);
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_SIZE),
                                    torrentFile.value("size").toULongLong());
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_PROGRESS),
                                    torrentFile.value("progress").toUInt());
        m_previewListModel->setData(m_previewListModel->index(rowCount, TR_FILE_INDEX),
                                    torrentFile.value("id").toULongLong());
    }
}
