#include "moviedetaildialog.h"
#include "ui_moviedetaildialog.h"

#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "utils/gui.h"
#include "utils/misc.h"

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

    // Code below is for my prefered sorting for the movie titles and for a flag lookup
    struct titlesValue
    {
        uint priority;
        QString flag;
    };
    // TODO populate missing flags silverqx
    static const QHash<QString, titlesValue> titlesHash
    {
        {"Československo", { 1, "cz"}},
        {"Česko",          { 2, "cz"}},
        {"USA",            { 3, "us"}},
        {"Slovensko",      { 4, "sk"}},
        {"Velká Británie", { 5, "gb"}},
        {"Kanada",         { 6, "ca"}},
        {"Německo",        { 7, "de"}},
        {"Francie",        { 8, "fr"}},
        {"Nový Zéland",    { 9, "nz"}},
        {"Austrálie",      {10, "au"}},
    };
    static const auto titlesPriorityHashNew = titlesHash.size() + 1;
    const auto compareTitlesByLang = [](const QJsonValue &left, const QJsonValue &right) -> bool
    {
        // TODO if two keys are same and one is pracovní název, so flag it and give him titlesPriorityHashNew priority, see eg how to train dragon 3 silverqx
        const auto leftTmp = titlesHash[left["language"].toString()].priority;
        const auto rightTmp = titlesHash[right["language"].toString()].priority;
        return (leftTmp == 0 ? titlesPriorityHashNew : leftTmp)
            < (rightTmp == 0 ? titlesPriorityHashNew : rightTmp);
    };
}

// TODO check if std::move() can be used for swap silverqx
inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
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

    // Title section
    ui->title->setText(movieTitle);
    // Titles section
    prepareTitlesSection();
    // Movie info section - genre, shot places, year and length
    prepareMovieInfoSection();
    // Score section
    ui->score->setText(QString::number(m_movieDetail["score"].toInt()) + QStringLiteral("%"));
    // Creators section
    prepareCreatorsSection();
    // Storyline section
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
    auto url = QUrl(m_movieDetail["poster"].toString(), QUrl::StrictMode);
    url.setQuery(QUrlQuery({{"w250", nullptr}}));
    m_networkManager.get(QNetworkRequest(url));
}

void MovieDetailDialog::prepareTitlesSection()
{
    // Titles section
    auto titlesArr = m_movieDetail["titles"].toArray();
    // My prefered sort
    std::sort(titlesArr.begin(), titlesArr.end(), compareTitlesByLang);

    // Create grid for flags and titles
    auto gridLayoutTitles = new QGridLayout;
    const int flagWidth = 21;
    gridLayoutTitles->setColumnMinimumWidth(0, flagWidth);
    gridLayoutTitles->setColumnStretch(1, 1);
    gridLayoutTitles->setHorizontalSpacing(9);
    gridLayoutTitles->setVerticalSpacing(0);
    ui->verticalLayoutTitles->addLayout(gridLayoutTitles);

    // Populate grid layout with flags and titles
    QLabel *labelFlag;
    QLabel *labelTitle;
    QJsonObject titleObject;
    QString titleLanguage;
    QIcon flagIcon;
    QPixmap flag;
    QFont titlesFont = this->font();
    titlesFont.setFamily(QStringLiteral("Arial"));
    titlesFont.setPointSize(12);
    titlesFont.setBold(true);
    titlesFont.setKerning(true);
    for (int i = 0; i < titlesArr.size() ; ++i) {
        titleObject = titlesArr[i].toObject();
        titleLanguage = titleObject["language"].toString();
        // Flag
        labelFlag = new QLabel;
        if (titlesHash.contains(titleLanguage)) {
            flagIcon = getFlagIcon(titlesHash[titleLanguage].flag);
            flag = flagIcon.pixmap(flagIcon.actualSize(QSize(flagWidth, 16)));
            labelFlag->setPixmap(flag);
        } else {
            qDebug() << "titlesHash doesn't contain this language :"
                     << titleLanguage;
        }
        labelFlag->setToolTip(titleLanguage);
        labelFlag->setToolTipDuration(2000);
        // Title
        labelTitle = new QLabel;
        labelTitle->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                            Qt::TextSelectableByKeyboard);
        labelTitle->setFont(titlesFont);
        labelTitle->setText(titleObject["title"].toString());

        gridLayoutTitles->addWidget(labelFlag, i, 0);
        gridLayoutTitles->addWidget(labelTitle, i, 1);
    }
}

void MovieDetailDialog::prepareMovieInfoSection()
{
    // Movie info section - genre, shot places, year and length
    // Line 1
    // Genre
    const auto genre = joinJsonStringArray(m_movieDetail["genre"].toArray(), delimiterSlash);
    // Line 2
    // Shot places
    const auto shotPlaces = joinJsonStringArray(m_movieDetail["shotPlaces"].toArray(),
            delimiterSlash);
    // Length
    const auto lengthValue = m_movieDetail["length"].toInt();
    const auto lengthString = (lengthValue > 180)
        ? Utils::Misc::userFriendlyDuration(
              lengthValue, Utils::Misc::DURATION_INPUT::MINUTES)
        : QString::number(lengthValue) + " min";
    const auto length = "<span style='color: palette(link);'>" +
                        lengthString + "</span>";
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
    // Creators section
    // Directors
    const auto keyName = QStringLiteral("name");
    const auto keyId = QStringLiteral("id");
    const auto wrapInLink = "<a href='https://www.csfd.cz/tvurce/%2' "
                            "style='text-decoration: none;'>%1</a>";

    const QString directors = joinJsonObjectArrayWithWrap(m_movieDetail["directors"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);
    // Screenplay
    const QString screenplay = joinJsonObjectArrayWithWrap(m_movieDetail["screenplay"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);
    // TODO camera is missing silverqx
    // Music
    const QString music = joinJsonObjectArrayWithWrap(m_movieDetail["music"].toArray(),
            delimiterComma, wrapInLink, keyName, keyId);
    // Actors
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

QIcon MovieDetailDialog::getFlagIcon(const QString &countryIsoCode) const
{
    if (countryIsoCode.isEmpty()) return {};

    const QString key = countryIsoCode.toLower();
    // Return from flag cache
    const auto iter = m_flagCache.find(key);
    if (iter != m_flagCache.end())
        return *iter;

    const QIcon icon {QLatin1String(":/icons/flags/") + key + QLatin1String(".svg")};
    // Save to flag cache
    m_flagCache[key] = icon;
    return icon;
}

void MovieDetailDialog::finishedMoviePoster(QNetworkReply *reply)
{
    // TODO handle network errors silverqx
    // TODO avangers poster not downloaded silverqx
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
