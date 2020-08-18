#include "moviedetaildialog.h"
#include "ui_moviedetaildialog.h"

#include <QJsonArray>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>

#include "utils/gui.h"

namespace {
    // Popular delimiters
    static const auto delimiterSlash       = " / ";
    static const QString delimiterNewLine  = "\n";
    static const auto delimiterComma       = ", ";
    static const auto delimiterHtmlNewLine = "<br>";

    /*! Join QJsonArray, all values in array have to be strings. */
    const auto joinJsonStringArray = [](const QJsonArray &jsonArray, const QString &delimiter)
                                     -> QString
    {
        QString result = "";
        QString value;
        int count = 0;
        for (const auto &jsonValue : jsonArray) {
            if (!jsonValue.isString() || jsonValue.isNull()
                || jsonValue.isUndefined())
                continue;
            value = jsonValue.toString();
            if (value.isEmpty())
                continue;
            result += value + delimiter;
            ++count;
        }
        if (count > 0)
            result.chop(delimiter.size());
        return result;
    };

    /*! Join QJsonArray, all values in array have to be QJsonObject, values will be searched
        in the jsonArray by keys stored in args pack and wrapped in wrapIn. */
    template <typename ...Args>
    QString joinJsonObjectArrayWithWrap(const QJsonArray &jsonArray, const QString &delimiter,
            const QString &wrappIn, const Args &...args)
    {
        const int argsSize = sizeof ...(args);
        if (argsSize == 0)
            qCritical() << "Empty argsSize in joinJsonObjectArrayWithWrap()";

        QString result = "";
        QString value;
        QJsonValue jsonValueInner;
        int jsonValueType;
        int count = 0;
        const QStringList argsList {(args)...};
        for (const auto &jsonValue : jsonArray) {
            if (!jsonValue.isObject() || jsonValue.isNull()
                || jsonValue.isUndefined())
                continue;
            QString wrapInTmp = wrappIn;
            // Replace all occurences
            for (const auto &arg : argsList) {
                jsonValueInner = jsonValue[arg];
                jsonValueType = jsonValueInner.type();
                switch (jsonValueType) {
                case QJsonValue::String:
                    value = jsonValueInner.toString();
                    break;
                case QJsonValue::Double:
                    value = QString::number(jsonValueInner.toDouble());
                    break;
                default:
                    value = "";
                    qWarning() << QStringLiteral("Unsupported jsonValueType '%1' in "
                                                 "joinJsonObjectArrayWithWrap()")
                                  .arg(jsonValueType);
                    break;
                }
                wrapInTmp = wrapInTmp.arg(value);
            }
            result += wrapInTmp + delimiter;
            ++count;
        }
        if (count > 0)
            result.chop(delimiter.size());
        return result;
    };

    /*! Join QStringList, exclude empty or null values. */
    const auto joinStringList = [](const QStringList &stringList, const QString &delimiter)
                                -> QString
    {
        QString result = "";
        int count = 0;
        for (const auto &value : stringList) {
            if (value.isEmpty() || value.isNull())
                continue;
            result += value + delimiter;
            ++count;
        }
        if (count > 0)
            result.chop(delimiter.size());
        return result;
    };
}

MovieDetailDialog::MovieDetailDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MovieDetailDialog)
{
    ui->setupUi(this);

    // Ensure recenter of the dialog after resize
    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(1000);
    connect(m_resizeTimer, &QTimer::timeout, this, &MovieDetailDialog::resizeTimeout);

    // Connect events
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &MovieDetailDialog::finishedMoviePoster);

    // Center on active screen
    Utils::Gui::centerDialog(this);
}

MovieDetailDialog::~MovieDetailDialog()
{
    delete ui;
}

void MovieDetailDialog::prepareData(const QSqlRecord &torrent)
{
    m_movieDetail = CsfdDetailService::instance()->getMovieDetail(torrent);

    // Movie poster, start as soon as possible, because it's async
    prepareMoviePosterSection();

    // Modal dialog title
    const auto movieTitle = m_movieDetail["title"].toString();
    setWindowTitle(" Detail filmu " + movieTitle + "  ( čsfd.cz )");

    // title section
    ui->title->setText(movieTitle);
    // titles section
    prepareTitlesSection();
    // movie info section - genre, shot places, year and length
    prepareMovieInfoSection();
    // Score section
    ui->score->setText(QString::number(m_movieDetail["score"].toInt()) + QStringLiteral("%"));
    // creators section
    prepareCreatorsSection();
    // storyline section
    ui->storyline->setText(m_movieDetail["content"].toString());
}

void MovieDetailDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    m_resizeTimer->start();
}

void MovieDetailDialog::prepareMoviePosterSection()
{
    // TODO handle errors silverqx
    m_networkManager.get(QNetworkRequest(QUrl(m_movieDetail["poster"].toString())));
}

void MovieDetailDialog::prepareTitlesSection()
{
    // titles section
    const auto titlesArr = m_movieDetail["titles"].toArray();
    ui->titles->setText(titlesArr[0]["title"].toString() + delimiterNewLine +
            titlesArr[1]["title"].toString());
    ui->storyline->setWordWrap(true);
}

void MovieDetailDialog::prepareMovieInfoSection()
{
    // movie info section - genre, shot places, year and length
    // line 1
    // genre
    const auto genre = joinJsonStringArray(m_movieDetail["genre"].toArray(), delimiterSlash);
    // line 2
    // shot places
    const auto shotPlaces = joinJsonStringArray(m_movieDetail["shotPlaces"].toArray(),
            delimiterSlash);
    // length
    const auto length = "<span style='color: palette(link);'>" +
                        QString::number(m_movieDetail["length"].toInt()) + " min</span>";
    QStringList movieInfoLine2List;
    movieInfoLine2List << shotPlaces
                       << QString::number(m_movieDetail["year"].toInt())
                       << length;
    const auto movieInfoLine2 = joinStringList(movieInfoLine2List, delimiterComma);

    // Assemble movie info section
    QStringList movieInfoList;
    movieInfoList << genre
                  << movieInfoLine2;
    const auto movieInfo = joinStringList(movieInfoList, delimiterHtmlNewLine);
    ui->movieInfo->setText(movieInfo);
}

void MovieDetailDialog::prepareCreatorsSection()
{
    // creators section
    // directors
    const auto keyName = QStringLiteral("name");
    const auto keyId = QStringLiteral("id");
    const auto wrapInLink = "<a href='https://www.csfd.cz/tvurce/%2' "
                            "style='text-decoration: none;'>%1</a>";

    const QString directors = joinJsonObjectArrayWithWrap(m_movieDetail["directors"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);
    // screenplay
    const QString screenplay = joinJsonObjectArrayWithWrap(m_movieDetail["screenplay"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);
    // TODO camera is missing silverqx
    // music
    const QString music = joinJsonObjectArrayWithWrap(m_movieDetail["music"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);
    // actors
    const QString actors = joinJsonObjectArrayWithWrap(m_movieDetail["actors"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);

    // Assemble creators section
    QStringList creatorsList;
    // TODO if QStringLiteral used, encoding is coruppted, ivestigate why, silverqx
    // TODO fix the same width for every section title silverqx
    creatorsList << QString("<strong>Réžia: </strong>%1").arg(directors)
                 << QString("<strong>Scenár: </strong>%1").arg(screenplay)
                 << QString("<strong>Hudba: </strong>%1").arg(music)
                 << QString("<strong>Herci: </strong>%1").arg(actors);
    const auto creators = joinStringList(creatorsList, delimiterHtmlNewLine);
    ui->creators->setText(creators);
}

void MovieDetailDialog::finishedMoviePoster(QNetworkReply *reply)
{
    const auto moviePosterData = reply->readAll();
    QPixmap moviePoster;
    moviePoster.loadFromData(moviePosterData);
    // TODO try to avoid this, best would be scaled at loadFromData() right away silverqx
    moviePoster = moviePoster.scaledToWidth(ui->poster->maximumWidth(), Qt::SmoothTransformation);
    ui->poster->setPixmap(moviePoster);
}

void MovieDetailDialog::resizeTimeout()
{
    // Resize event is also triggered, when the dialog is shown, this prevents unwanted resize
    if (m_firstResizeCall) {
        m_firstResizeCall = false;
        return;
    }

    Utils::Gui::centerDialog(this);
}
