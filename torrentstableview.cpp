#include "torrentstableview.h"

#include <QLineEdit>
#include <QHeaderView>

#include "mainwindow.h"
#include "torrentsqltablemodel.h"

TorrentsTableView::TorrentsTableView(QWidget *parent, MainWindow *mainWindow)
    : QTableView(parent)
    , m_mainWindow(mainWindow)
{
    m_model = new TorrentSqlTableModel(this);
    m_model->setTable("torrents");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->select();
    m_model->setHeaderData(0, Qt::Horizontal, "Id");
    m_model->setHeaderData(1, Qt::Horizontal, "Name");
    m_model->setHeaderData(2, Qt::Horizontal, "Size");
    m_model->setHeaderData(3, Qt::Horizontal, "Done");
    m_model->setHeaderData(4, Qt::Horizontal, "Added on");

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(1);

    setModel(m_proxyModel);

    sortByColumn(1, Qt::DescendingOrder);
//    m_proxyModel->sort(1, Qt::DescendingOrder);
//    model->sort(1, Qt::AscendingOrder);

//    horizontalHeader()
//        ->setSectionResizeMode(QHeaderView::ResizeToContents);
//    horizontalHeader()
//            ->setSectionResizeMode(QHeaderView::Interactive);

    horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    horizontalHeader()->resizeSection(1, 350);

//    connect(m_mainWindow->lineEdit, &QLineEdit::textChanged, this, &MainWindow::applyTextChanged);
}

TorrentsTableView::~TorrentsTableView()
{

}

void TorrentsTableView::applyTextChanged(const QString& name)
{
    m_proxyModel->setFilterRegExp(
        QRegExp(name, Qt::CaseInsensitive, QRegExp::WildcardUnix)
    );
}
