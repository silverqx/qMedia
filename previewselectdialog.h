#pragma once
#ifndef PREVIEWSELECTDIALOG_H
#define PREVIEWSELECTDIALOG_H

#include <QDialog>
#include <QPointer>
#include <QtSql/QSqlRecord>

class PreviewListDelegate;
class QStandardItemModel;

namespace Ui
{
    class PreviewSelectDialog;
}

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

    /*! Contructor. */
    PreviewSelectDialog(QWidget *parent, const QSqlRecord &torrent,
                        const QSharedPointer<const QVector<QSqlRecord>> &torrentFiles);
    /*! Virtual destructor. */
    inline ~PreviewSelectDialog() final = default;

    /*! Assemble absolute file path for torrent file. */
    inline QString getTorrentFileFilePathAbs(const QString &relativePath) const;

signals:
    void readyToPreviewFile(const QString &filePath);

protected:
    void showEvent(QShowEvent *event) final;

private:
    void populatePreviewListModel() const;

    const QSqlRecord m_torrent;
    const QSharedPointer<const QVector<QSqlRecord>> m_torrentFiles;
    bool m_showEventInitialized = false;

    std::unique_ptr<Ui::PreviewSelectDialog> m_ui;
    QPointer<QStandardItemModel> m_previewListModel;
    QPointer<PreviewListDelegate> m_listDelegate;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
private slots:
    void previewButtonClicked();
};

#endif // PREVIEWSELECTDIALOG_H
