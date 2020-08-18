#ifndef MOVIEDETAILDIALOG_H
#define MOVIEDETAILDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>

#include "csfddetailservice.h"

class QSqlRecord;

namespace Ui {
    class MovieDetailDialog;
}

class MovieDetailDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(MovieDetailDialog)

public:
    explicit MovieDetailDialog(QWidget *parent = nullptr);
    ~MovieDetailDialog();

    /*! Prepare data and populate ui with them. */
    void prepareData(const QSqlRecord &torrent);

private:
    void prepareMoviePosterSection();
    void prepareTitlesSection();
    void prepareMovieInfoSection();
    void prepareCreatorsSection();

    Ui::MovieDetailDialog *ui;
    MovieDetail m_movieDetail;
    // TODO implement QNetworkDiskCache silverqx
    QNetworkAccessManager m_networkManager;

private slots:
    void finishedMoviePoster(QNetworkReply *reply);
};

#endif // MOVIEDETAILDIALOG_H
