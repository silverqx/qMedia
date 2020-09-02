#ifndef MOVIEDETAILDIALOG_H
#define MOVIEDETAILDIALOG_H

#include <QJsonArray>
#include <QJsonObject>
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

    /*! Prepare data from torrent and populate ui with them. */
    void prepareData(const QSqlRecord &torrent);

signals:
    void readyToPreviewFile();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void prepareMoviePosterSection();
    void renderTitleSection();
    void prepareTitlesSection();
    void renderTitlesSection(int maxLines = 0);
    void prepareImdbLink();
    void prepareMovieInfoSection();
    void prepareCreatorsSection();
    void prepareMovieDetailComboBox();
    void populateMovieDetailComboBox();
    QIcon getFlagIcon(const QString &countryIsoCode) const;
    /*! Prepare data by movie id and populate ui with them. */
    void prepareData(quint64 filmId);
    void populateUi();

    Ui::MovieDetailDialog *ui;
    QJsonObject m_movieDetail;
    QJsonArray m_movieSearchResult;
    // TODO implement QNetworkDiskCache silverqx
    QNetworkAccessManager m_networkManager;
    QTimer *m_resizeTimer;
    bool m_resizeInProgress = false;
    // TODO use mutable where appropriate silverqx
    mutable QHash<QString, QIcon> m_flagCache;
    QGridLayout *m_gridLayoutTitles = nullptr;
    QVBoxLayout *m_verticalLayoutCreators = nullptr;
    QSqlRecord m_selectedTorrent;
    /*! First show. */
    bool m_initialPopulate = true;
    /*! Currently showed movie detail index, used in movie detail combobox. */
    int m_movieDetailIndex;
    /*! Save button in the buttonBox. */
    QPushButton *m_saveButton;

private slots:
    void finishedMoviePoster(QNetworkReply *reply);
    void resizeTimeout();
    void saveButtonClicked();
    void previewButtonClicked();
    void movieDetailComboBoxChanged(int index);
};

#endif // MOVIEDETAILDIALOG_H
