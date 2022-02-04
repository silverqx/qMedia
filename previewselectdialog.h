#pragma once
#ifndef PREVIEWSELECTDIALOG_H
#define PREVIEWSELECTDIALOG_H

#include <QDialog>
#include <QtSql/QSqlRecord>

class PreviewListDelegate;
class QStandardItemModel;

QT_BEGIN_NAMESPACE
namespace Ui { class PreviewSelectDialog; }
QT_END_NAMESPACE

class PreviewSelectDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(PreviewSelectDialog)

public:
    enum PreviewColumn
    {
        TR_NAME,
        TR_SIZE,
        TR_PROGRESS,
        TR_FILE_INDEX,

        NB_COLUMNS
    };

    PreviewSelectDialog(QWidget *parent, QSqlRecord torrent,
                        const QSharedPointer<const QVector<QSqlRecord>> &torrentFiles);
    ~PreviewSelectDialog();

    /*! Assemble absolute file path for torrent file. */
    inline QString getTorrentFileFilePathAbs(const QString &relativePath) const;

signals:
    void readyToPreviewFile(const QString &filePath);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void populatePreviewListModel() const;

    Ui::PreviewSelectDialog *m_ui;
    QStandardItemModel *m_previewListModel;
    const QSqlRecord m_torrent;
    const QSharedPointer<const QVector<QSqlRecord>> m_torrentFiles;
    bool m_showEventInitialized = false;
    PreviewListDelegate *m_listDelegate;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
private slots:
    void previewButtonClicked();
};

#endif // PREVIEWSELECTDIALOG_H
