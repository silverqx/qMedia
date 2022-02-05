#pragma once
#ifndef MOVIEDETAILDIALOG_H
#define MOVIEDETAILDIALOG_H

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPointer>

#include "csfddetailservice.h"

class QGridLayout;
class QSqlRecord;
class QVBoxLayout;

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
    explicit MovieDetailDialog(QWidget *parent = nullptr);

    /*! Prepare data from a torrent and populate UI with them. */
    void prepareData(const QSqlRecord &torrent);

signals:
    void readyToPreviewFile();

protected:
    void resizeEvent(QResizeEvent *event) final;
    void showEvent(QShowEvent *event) final;
    bool eventFilter(QObject *watched, QEvent *event) final;

private:
    void prepareMoviePosterSection();
    void renderTitleSection() const;
    void prepareTitlesSection();
    void renderTitlesSection(int maxLines = 0) const;
    void prepareImdbLink() const;
    void prepareMovieInfoSection() const;
    void prepareCreatorsSection();
    void prepareMovieDetailComboBox();
    void populateMovieDetailComboBox() const;
    QIcon getFlagIcon(const QString &countryIsoCode) const;
    /*! Prepare data by movie id and populate ui with them. */
    void prepareData(quint64 filmId);
    void populateUi();

    QJsonObject m_movieDetail;
    QJsonArray m_movieSearchResults;
    QNetworkAccessManager m_networkManager;
    bool m_resizeInProgress = false;
    mutable QHash<QString, QIcon> m_flagsCache;
    QSqlRecord m_selectedTorrent;
    /*! First show. */
    bool m_initialPopulate = true;
    /*! Currently showed movie detail index, used in movie detail combobox. */
    int m_movieDetailIndex = -1; // CUR check if validate -1 is needed silverqx
    std::shared_ptr<StatusHash> m_statusHash;

    std::unique_ptr<Ui::MovieDetailDialog> m_ui;
    QPointer<QTimer> m_resizeTimer;
    QGridLayout *m_gridLayoutTitles = nullptr; // QPointer causes clang-tidy freeze 😕 silverqx
    QPointer<QVBoxLayout> m_verticalLayoutCreators;
    /*! Save button in the buttonBox. */
    QPointer<QPushButton> m_saveButton;

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
private slots:
    void finishedMoviePoster(QNetworkReply *reply) const;
    void resizeTimeout();
    void saveButtonClicked();
    void movieDetailComboBoxChanged(int index);
};

#endif // MOVIEDETAILDIALOG_H
