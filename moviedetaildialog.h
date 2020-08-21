#ifndef MOVIEDETAILDIALOG_H
#define MOVIEDETAILDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>

#include "csfddetailservice.h"

class QGridLayout;
class QSqlRecord;
class QVBoxLayout;

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void prepareMoviePosterSection();
    void prepareTitlesSection();
    void renderTitlesSection(int maxLines = 0);
    void prepareMovieInfoSection();
    void prepareCreatorsSection();
    QIcon getFlagIcon(const QString &countryIsoCode) const;

    Ui::MovieDetailDialog *ui;
    MovieDetail m_movieDetail;
    // TODO implement QNetworkDiskCache silverqx
    QNetworkAccessManager m_networkManager;
    QTimer *m_resizeTimer;
    bool m_firstResizeCall = true;
    // TODO use mutable where appropriate silverqx
    mutable QHash<QString, QIcon> m_flagCache;
    QGridLayout *m_gridLayoutTitles;
    QVBoxLayout *m_verticalLayoutCreators;

private slots:
    void finishedMoviePoster(QNetworkReply *reply);
    void resizeTimeout();
};

#endif // MOVIEDETAILDIALOG_H
