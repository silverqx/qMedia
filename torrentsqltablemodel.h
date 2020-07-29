#ifndef TORRENTSQLTABLEMODEL_H
#define TORRENTSQLTABLEMODEL_H

#include <QSqlTableModel>

class TorrentSqlTableModel final : public QSqlTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(TorrentSqlTableModel)

public:
    enum Column
    {
        TR_ID,
        TR_NAME,
        TR_SIZE,
        TR_PROGRESS,
        TR_ADDED_ON,
        TR_HASH,

        NB_COLUMNS
    };

    enum DataRole
    {
        UnderlyingDataRole = Qt::UserRole,
    };

    explicit TorrentSqlTableModel(QObject *parent = nullptr, const QSqlDatabase db = QSqlDatabase());
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

private:
    QString displayValue(const QModelIndex &modelIndex, int column) const;
};

#endif // TORRENTSQLTABLEMODEL_H
