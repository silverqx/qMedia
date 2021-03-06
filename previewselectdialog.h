/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2011  Christophe Dumez <chris@qbittorrent.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

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
                        const QSharedPointer<const QVector<QSqlRecord>> torrentFiles);
    ~PreviewSelectDialog();

    /*! Assemble absolute file path for torrent file. */
    inline QString getTorrentFileFilePathAbs(const QString &relativePath) const;

signals:
    void readyToPreviewFile(QString);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void populatePreviewListModel() const;

    Ui::PreviewSelectDialog *ui;
    QStandardItemModel *m_previewListModel;
    const QSqlRecord m_torrent;
    const QSharedPointer<const QVector<QSqlRecord>> m_torrentFiles;
    bool m_showEventInitialized = false;
    PreviewListDelegate *m_listDelegate;

private slots:
    void previewButtonClicked();
};

#endif // PREVIEWSELECTDIALOG_H
