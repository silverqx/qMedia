#pragma once
#ifndef MOVIEDETAILDIALOG_H
#define MOVIEDETAILDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QtSql/QSqlRecord>

#include "types/moviedetail.h"

class QGridLayout;
class QSqlRecord;
class QVBoxLayout;

class CsfdDetailService;
class StatusHash;

namespace Ui
{
    class MovieDetailDialog;
}

class MovieDetailDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(MovieDetailDialog)

public:
    /*! Constructor. */
    explicit MovieDetailDialog(
            const std::shared_ptr<CsfdDetailService> &csfdDetailService,
            QWidget *parent = nullptr);

    /*! Prepare data from a torrent and populate UI with them. */
    void prepareData(const QSqlRecord &torrent);

signals:
    void readyToPreviewFile();

protected:
    void resizeEvent(QResizeEvent *event) final;
    void showEvent(QShowEvent *event) final;
    bool eventFilter(QObject *watched, QEvent *event) final;

private:
    void populateUi();
    void prepareMoviePosterSection();
    void renderTitleSection() const;
    void prepareTitlesSection();
    void renderTitlesSection(int maxLines = 0) const;
    void prepareCsfdLink() const;
    void prepareImdbLink() const;
    void prepareMovieInfoSection() const;
    void prepareCreatorsSection();
    void prepareMovieDetailComboBox();
    void populateMovieDetailComboBox() const;
    QIcon getFlagIcon(const QString &countryIsoCode) const;
    /*! Prepare data by movie id and populate ui with them. */
    void prepareData(quint64 filmId);
    /*! Enable/disable the save button. */
    void toggleSaveButton(bool enable);
    /*! Search movie detail on čsfd. */
    void getSearchMovieDetail(const QSqlRecord &torrent, bool skipCache);

    MovieDetail m_movieDetail;
    MovieSearchResults m_movieSearchResults;
    QNetworkAccessManager m_networkManager;
    bool m_resizeInProgress = false;
    mutable QHash<QString, QIcon> m_flagsCache;
    QSqlRecord m_selectedTorrent;
    /*! First show. */
    bool m_initialPopulate = true;
    /*! Currently showed movie detail index, used in movie detail combobox. */
    int m_movieDetailIndex = -1;
    std::shared_ptr<StatusHash> m_statusHash;

    std::unique_ptr<Ui::MovieDetailDialog> m_ui;
    QPointer<QTimer> m_resizeTimer;
    QPointer<QGridLayout> m_gridLayoutTitles;
    QPointer<QVBoxLayout> m_verticalLayoutCreators;
    /*! Save button in the buttonBox. */
    QPointer<QPushButton> m_saveButton;
    std::shared_ptr<CsfdDetailService> m_csfdDetailService;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
private slots:
    void finishedMoviePoster(QNetworkReply *reply) const;
    void resizeTimeout();
    void saveButtonClicked();
    void forceReloadButtonClicked();
    void movieDetailComboBoxChanged(int index);
};

#endif // MOVIEDETAILDIALOG_H
