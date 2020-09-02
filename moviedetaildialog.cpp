#include "moviedetaildialog.h"
#include "ui_moviedetaildialog.h"

#include <QDesktopServices>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QShortcut>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "torrentstatus.h"
#include "utils/gui.h"
#include "utils/misc.h"

namespace
{
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

    // Used in pagination
    static const auto moreLinkText = [](const bool withDots = true,
                                     const bool withParenthesis = true)
    {
        return (withDots ? QStringLiteral("<strong>...</strong> ") : QStringLiteral("")) +
            (withParenthesis ? QStringLiteral("(") : QStringLiteral("")) +
            QStringLiteral("<a href='#show-more' "
                          "style='font-size: 11pt; text-decoration: none;'"
                          ">more</a>") +
            (withParenthesis ? QStringLiteral(")") : QStringLiteral(""));
    };
    static const auto moreLinkSize = QStringLiteral("... (more)").size();

    /*! Join QJsonArray, all values in array have to be QJsonObject, values will be searched
        in the jsonArray by keys stored in args pack and wrapped in wrapIn. If maxLetters
        contains value greater than 0, then result will be cutted and at the end will be
        shown (more) link, letters will be counted only for first substitution ( first key
        in the args pack ). Resulting string can contain less letters, because when
        maxLetters limit will be hit or overflow, then currently processed string will not be
        included at all. */
    template <typename ...Args>
    QString joinJsonObjectArrayWithWrapPaged(const QJsonArray &jsonArray, const QString &delimiter,
            const QString &wrappIn, const int maxLetters, const Args &...args)
    {
        const int argsSize = sizeof ...(args);
        if (argsSize == 0)
            qCritical() << QStringLiteral("Empty argsSize in joinJsonObjectArrayWithWrap()");

        QString result = "";
        QString value;
        QJsonValue jsonValueInner;
        int jsonValueType;
        int count = 0;
        int lettersCount = 0;
        int lettersCountTmp = 0;
        const auto paginateEnabled = (maxLetters != 0);
        auto showMoreLink = false;
        const auto delimiterSize = delimiter.size();
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
                // Letters are counted only for first substitution
                if (paginateEnabled && (arg == argsList[0]))
                    lettersCount += value.size() + delimiterSize;
                wrapInTmp = wrapInTmp.arg(value);
            }
            /* Last delimiter will never be in the result, also take a more text
               link size into account. */
            lettersCountTmp = lettersCount - delimiterSize + moreLinkSize;
            // lettersCount overflowed 😎
            if (paginateEnabled && (lettersCountTmp >= maxLetters)) {
                showMoreLink = true;
                break;
            }
            result += wrapInTmp + delimiter;
            ++count;
        }
        if (count > 0)
            result.chop(delimiter.size());
        // Append show more link if needed
        if (showMoreLink)
            result += moreLinkText();
        return result;
    };

    /*! Join QJsonArray, all values in array have to be QJsonObject, values will be searched
        in the jsonArray by keys stored in args pack and wrapped in wrapIn. */
    template <typename ...Args>
    QString joinJsonObjectArrayWithWrap(const QJsonArray &jsonArray, const QString &delimiter,
            const QString &wrappIn, const Args &...args)
    {
        return joinJsonObjectArrayWithWrapPaged(jsonArray, delimiter, wrappIn, 0, args...);
    }

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
    struct TitlesValue
    {
        uint priority;
        QString flag;
    };
    // TODO populate missing flags silverqx
    static const QHash<QString, TitlesValue> titlesHash
    {
        {QStringLiteral("Československo"), { 1, QStringLiteral("cz")}},
        {QStringLiteral("Česko"),          { 2, QStringLiteral("cz")}},
        {QStringLiteral("USA"),            { 3, QStringLiteral("us")}},
        {QStringLiteral("Slovensko"),      { 4, QStringLiteral("sk")}},
        {QStringLiteral("Velká Británie"), { 5, QStringLiteral("gb")}},
        {QStringLiteral("Kanada"),         { 6, QStringLiteral("ca")}},
        {QStringLiteral("Německo"),        { 7, QStringLiteral("de")}},
        {QStringLiteral("Francie"),        { 8, QStringLiteral("fr")}},
        {QStringLiteral("Nový Zéland"),    { 9, QStringLiteral("nz")}},
        {QStringLiteral("Austrálie"),      {10, QStringLiteral("au")}},
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

    /*! Resulting string will contain max. letters defined by maxLetters parameter,
        including 'show more' link text. */
    Q_DECL_UNUSED const auto paginateString =
            [](const QString &string, const int maxLetters) -> QString
    {
        // Pagination not needed
        if (string.size() <= maxLetters)
            return string;

        return QString(string).remove(maxLetters - moreLinkSize, string.size()) +
                moreLinkText();
    };

    /*! Remove all items from layout. */
    inline const auto wipeOutLayout = [](QLayout &layout)
    {
        // Nothing to remove
        if (layout.count() == 0)
            return;

        QLayoutItem *layoutItem;
        while ((layoutItem = layout.takeAt(0)) != nullptr) {
            delete layoutItem->widget();
            delete layoutItem;
        }
    };
}

// Needed when sorting QJsonArray
inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

MovieDetailDialog::MovieDetailDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MovieDetailDialog)
{
    ui->setupUi(this);

    // Override design values
    // TODO check how to correctly use my own DEFINES silverqx
#ifdef VISIBLE_CONSOLE
    // Set up smaller, so I can see console output
    const auto height = minimumHeight();
#else
    const auto height = 940;
#endif
    resize(1316, height);

    // Initialize ui widgets
    ui->saveButton->setEnabled(false);
    ui->saveButton->hide();
    // Preview button
    const auto previewButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    previewButton->setText(QStringLiteral("&Preview"));
    // Save button
    m_saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    m_saveButton->setEnabled(false);
    m_saveButton->hide();
    m_saveButton->setText(QStringLiteral("&Save"));
    // TODO create tooltip wrapper function for tooltips with shortcuts silverqx
    m_saveButton->setToolTip("<html><head/><body><p style='white-space:pre;'>Save current movie detail as default "
                             "<br><span style='font-size:7pt; color:#8c8c8c;'>ALT+S</span></p>"
                             "</body></html>");
    m_saveButton->setToolTipDuration(2500);
    // Close button
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(QStringLiteral("&Close"));

    // Ensure recenter of the dialog after resize
    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(100);
    connect(m_resizeTimer, &QTimer::timeout, this, &MovieDetailDialog::resizeTimeout);

    // Connect events
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &MovieDetailDialog::finishedMoviePoster);
    // saveButton
    connect(ui->saveButton, &QPushButton::clicked, this, &MovieDetailDialog::saveButtonClicked);
    // buttonBox
    connect(m_saveButton, &QPushButton::clicked, this, &MovieDetailDialog::saveButtonClicked);
    connect(previewButton, &QPushButton::clicked, this, &MovieDetailDialog::previewButtonClicked);
    // Order important, have to be last
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    // Hotkeys
    // movieDetailComboBox
    const auto *doubleClickHotkeyCtrlM = new QShortcut(Qt::CTRL + Qt::Key_M, ui->movieDetailComboBox, nullptr, nullptr, Qt::WindowShortcut);
    connect(doubleClickHotkeyCtrlM, &QShortcut::activated, ui->movieDetailComboBox, qOverload<>(&QComboBox::setFocus));
    const auto *doubleClickHotkeyF12 = new QShortcut(Qt::Key_F12, ui->movieDetailComboBox, nullptr, nullptr, Qt::WindowShortcut);
    connect(doubleClickHotkeyF12, &QShortcut::activated, ui->movieDetailComboBox, &QComboBox::showPopup);

    // Center on active screen
    Utils::Gui::centerDialog(this);
    // Event filter to detect resize done
    qApp->installEventFilter(this);
}

MovieDetailDialog::~MovieDetailDialog()
{
    delete ui;
}

void MovieDetailDialog::prepareData(const QSqlRecord &torrent)
{
    m_selectedTorrent = torrent;
    m_movieDetailIndex = torrent.value("movie_detail_index").toInt();
    const auto movieDetail = CsfdDetailService::instance()->getMovieDetail(torrent);
    m_movieDetail = movieDetail["detail"].toObject();
    m_movieSearchResult = movieDetail["search"].toArray();

    populateUi();
    m_initialPopulate = false;
}

void MovieDetailDialog::prepareData(const quint64 filmId)
{
    const auto movieDetail = CsfdDetailService::instance()
                             ->getMovieDetail(filmId);
    m_movieDetail = movieDetail.object();

    populateUi();
}

void MovieDetailDialog::populateUi()
{
    // Movie poster, start as soon as possible, because it's async
    prepareMoviePosterSection();

    // Modal dialog title
    const auto movieTitle = m_movieDetail["title"].toString();
    // TODO resolve utf8 boom and /source-charset:utf-8 silverqx
    setWindowTitle(QStringLiteral(" Detail filmu ") + movieTitle +
                   QStringLiteral("  ( čsfd.cz )"));

    // Title section
    // Status icon before movie titles
    if (m_initialPopulate) {
        const auto status = m_selectedTorrent.value("status").toString();
        const auto statusIcon = g_statusHash[status].getIcon();
        const auto statusPixmap = statusIcon.pixmap(statusIcon.actualSize(QSize(30, 20)));
        ui->status->setPixmap(statusPixmap);
        ui->status->setToolTip(g_statusHash[status].text);
        ui->status->setToolTipDuration(2500);
    }
    // Title
    // QLabel width needed, it's called from showEvent() during first show
    if (!m_initialPopulate)
        renderTitleSection();
    // Titles section
    prepareTitlesSection();
    // Movie info section - genre, shot places, year and length
    prepareMovieInfoSection();
    // Score section
    // TODO tune score QLabel color silverqx
    ui->score->setText(QString::number(m_movieDetail["score"].toInt()) + QStringLiteral("%"));
    // Imdb link
    prepareImdbLink();
    // Creators section
    prepareCreatorsSection();
    // Storyline section
    ui->storyline->setText(m_movieDetail["content"].toString());

    if (!m_initialPopulate)
        return;

    // Fill empty space
    ui->verticalLayoutInfo->addStretch(1);
    // Search results in ComboBox
    prepareMovieDetailComboBox();
}

void MovieDetailDialog::resizeEvent(QResizeEvent *event)
{
    m_resizeInProgress = true;

    QDialog::resizeEvent(event);
}

void MovieDetailDialog::showEvent(QShowEvent *)
{
    // Title
    renderTitleSection();
}

bool MovieDetailDialog::eventFilter(QObject *watched, QEvent *event)
{
    const auto eventType = event->type();
    /* We need to check for both types of mouse release, because it can vary on which type
       happened, when resizing. */
    if ((eventType == QEvent::MouseButtonRelease)
        || (eventType == QEvent::NonClientAreaMouseButtonRelease)
    ) {
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (m_resizeInProgress && (mouseEvent->button() == Qt::MouseButton::LeftButton)) {
            m_resizeInProgress = false;
            qDebug() << QStringLiteral("Resizing of the movie detail dialog ended");
            // I don't delete this timer logic and reuse it, even if it worked without it
            m_resizeTimer->start();
        }
    }

    return QObject::eventFilter(watched, event);
}

void MovieDetailDialog::prepareMoviePosterSection()
{
    // TODO handle errors silverqx
    // TODO add empty poster, if poster missing silverqx
    auto url = QUrl(m_movieDetail["poster"].toString(), QUrl::StrictMode);
    url.setQuery(QUrlQuery({{"w250", nullptr}}));
    m_networkManager.get(QNetworkRequest(url));
}

void MovieDetailDialog::renderTitleSection()
{
    const auto movieTitle = m_movieDetail["title"].toString();
    const auto movieTitleElided = ui->title->fontMetrics()
                                  .elidedText(movieTitle, Qt::ElideRight, ui->title->width());
    ui->title->setText(movieTitleElided);
    ui->title->setToolTip(QStringLiteral("Torrent name<br><strong>%1</strong>")
                          .arg(m_selectedTorrent.value("name").toString()));
    ui->title->setToolTipDuration(8500);
}

namespace
{
    static const int flagWidth = 21;
}

void MovieDetailDialog::prepareTitlesSection()
{
    // Create grid for flags and titles
    // Create even when there is nothing to render, so I don't have to manage positioning
    if (m_gridLayoutTitles == nullptr) {
        m_gridLayoutTitles = new QGridLayout;
        m_gridLayoutTitles->setColumnMinimumWidth(0, flagWidth);
        m_gridLayoutTitles->setColumnStretch(1, 1);
        m_gridLayoutTitles->setHorizontalSpacing(9);
        m_gridLayoutTitles->setVerticalSpacing(0);
        ui->verticalLayoutInfoLeft->addLayout(m_gridLayoutTitles);
    }

    // Remove all items from grid layout
    if (!m_initialPopulate)
        wipeOutLayout(*m_gridLayoutTitles);

    // Nothing to render
    if (m_movieDetail["titles"].toArray().isEmpty()) {
        qDebug() << "Empty titles for movie :" << m_movieDetail["title"].toString();
        return;
    }

    // Populate grid layout with flags and titles
    renderTitlesSection(3);
}

void MovieDetailDialog::renderTitlesSection(const int maxLines)
{
    auto titlesArr = m_movieDetail["titles"].toArray();
    // My prefered sort
    std::sort(titlesArr.begin(), titlesArr.end(), compareTitlesByLang);

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
    const auto paginateEnabled = (maxLines != 0);
    auto showMoreLink = false;
    const auto titlesArrSize = titlesArr.size();
    int i = 0;
    for (; i < titlesArrSize ; ++i) {
        titleObject = titlesArr[i].toObject();
        titleLanguage = titleObject["language"].toString();
        // Flag
        labelFlag = new QLabel;
        if (titlesHash.contains(titleLanguage)) {
            flagIcon = getFlagIcon(titlesHash[titleLanguage].flag);
            flag = flagIcon.pixmap(flagIcon.actualSize(QSize(flagWidth, 16)));
            labelFlag->setPixmap(flag);
        } else {
            qDebug() << QStringLiteral("titlesHash doesn't contain this language :")
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

        m_gridLayoutTitles->addWidget(labelFlag, i, 0);
        m_gridLayoutTitles->addWidget(labelTitle, i, 1);

        // Max. lines overflowed, take into account also show more link, because -1
        if (paginateEnabled && ((maxLines - 1) != titlesArrSize)
            && (i >= (maxLines - 2))) {
            showMoreLink = true;
            break;
        }
    }

    // No pagination needed
    if (!paginateEnabled || !showMoreLink)
        return;

    // Show more link
    auto labelMoreLink = new QLabel;
    labelMoreLink->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                           Qt::LinksAccessibleByKeyboard);
    labelMoreLink->setText(moreLinkText(false, false));
    m_gridLayoutTitles->addWidget(labelMoreLink, ++i, 1);
    labelMoreLink->connect(labelMoreLink, &QLabel::linkActivated, [this](const QString &link)
    {
        if (link != QLatin1String("#show-more"))
            return;

        // Remove all items from grid layout
        wipeOutLayout(*m_gridLayoutTitles);

        // Re-render whole section again
        renderTitlesSection();
    });
}

void MovieDetailDialog::prepareImdbLink()
{
    const auto imdbIdRaw = m_movieDetail["imdbId"];
    if (imdbIdRaw.isNull()) {
        ui->imdbLink->setEnabled(false);
        ui->imdbLink->hide();
        return;
    }

    ui->imdbLink->setText(QStringLiteral("<a href='https://www.imdb.com/title/%1/' "
                                         "style='color: #64a1ac; text-decoration: none;'>imdb</a>")
                          .arg(imdbIdRaw.toString()));
    ui->imdbLink->setEnabled(true);
    ui->imdbLink->show();
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
    // TODO create logging system silverqx
    // TODO log empty movieInfo, to know how often it happens silverqx
}

namespace
{
    enum CREATOR_NAMES
    {
        NAMES_DIRECTORS,
        NAMES_SCREENPLAY,
        NAMES_MUSIC,
        NAMES_ACTORS,
    };

    struct CreatorsValue
    {
        const QLatin1String keyName;
        const QString label;
    };

    static const CreatorsValue creatorsMap[]
    {
        {QLatin1String("directors"),  QStringLiteral("<strong>Réžia: </strong>%1")},
        {QLatin1String("screenplay"), QStringLiteral("<strong>Scenár: </strong>%1")},
        {QLatin1String("music"),      QStringLiteral("<strong>Hudba: </strong>%1")},
        {QLatin1String("actors"),     QStringLiteral("<strong>Herci: </strong>%1")},
    };
}

void MovieDetailDialog::prepareCreatorsSection()
{
    // Creators section
    const auto keyName = QStringLiteral("name");
    const auto keyId = QStringLiteral("id");
    const auto wrapInLink = QStringLiteral("<a href='https://www.csfd.cz/tvurce/%2' "
                                           "style='text-decoration: none;'>%1</a>");
    // 60 for 960px
    // 105 for 1316px
    // TODO compute dynamically by width silverqx
    static const auto maxLetters = 105;

    // Directors
    const QString directors = joinJsonObjectArrayWithWrapPaged(
            m_movieDetail[creatorsMap[NAMES_DIRECTORS].keyName].toArray(),
            delimiterComma, wrapInLink, maxLetters, keyName, keyId);
    // Screenplay
    const QString screenplay = joinJsonObjectArrayWithWrapPaged(
            m_movieDetail[creatorsMap[NAMES_SCREENPLAY].keyName].toArray(),
            delimiterComma, wrapInLink, maxLetters, keyName, keyId);
    // TODO camera is missing silverqx
    // Music
    const QString music = joinJsonObjectArrayWithWrapPaged(
            m_movieDetail[creatorsMap[NAMES_MUSIC].keyName].toArray(),
            delimiterComma, wrapInLink, maxLetters, keyName, keyId);
    // Actors
    const QString actors = joinJsonObjectArrayWithWrapPaged(
            m_movieDetail[creatorsMap[NAMES_ACTORS].keyName].toArray(),
            // 200 for 960px
            // 320 for 1316px
            delimiterComma, wrapInLink, 320, keyName, keyId);

    // Assemble creators section
    // TODO fix the same width for every section title silverqx
    // Order is important, have to be same as order in creatorsMap enum
    const QStringList creatorsList {
        directors,
        screenplay,
        music,
        actors,
    };

    // Create the layout for creators
    // Create even when there is nothing to render, so I don't have to manage positioning
    if (m_verticalLayoutCreators == nullptr) {
        m_verticalLayoutCreators = new QVBoxLayout;
        m_verticalLayoutCreators->setSpacing(2);
        ui->verticalLayoutInfo->addLayout(m_verticalLayoutCreators);
    }

    // Remove all items from box layout
    if (!m_initialPopulate)
        wipeOutLayout(*m_verticalLayoutCreators);

    // If all are empty, then nothing to render
    const auto result = std::find_if(creatorsList.constBegin(), creatorsList.constEnd(),
                                     [](const QString &creators)
    {
        // If any have some text, so will render section
        if (!creators.isEmpty())
            return true;
        return false;
    });
    // All was empty
    if (result == creatorsList.constEnd()) {
        qDebug() << "Empty creators for movie :" << m_movieDetail["title"].toString();
        return;
    }

    // Render each creators section into the layout
    QLabel *label;
    auto font = this->font();
    font.setFamily("Arial");
    font.setPointSize(12);
    font.setKerning(true);
    for (int i = 0; i < creatorsList.size(); ++i) {
        // Nothing to render
        if (creatorsList[i].isEmpty())
            continue;

        label = new QLabel;
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                       Qt::TextSelectableByKeyboard |
                                       Qt::LinksAccessibleByMouse |
                                       Qt::LinksAccessibleByKeyboard);
        label->setFont(font);
        label->setOpenExternalLinks(false);
        label->setText(creatorsMap[i].label.arg(creatorsList[i]));
        label->connect(label, &QLabel::linkActivated, this,
                       [this, label, wrapInLink, keyName, keyId, i](const QString &link)
        {
            // Open URL with external browser
            if (link != QLatin1String("#show-more")) {
                QDesktopServices::openUrl(QUrl(link, QUrl::StrictMode));
                return;
            }

            // Re-populate label
            const QString joinedText = joinJsonObjectArrayWithWrap(
                    m_movieDetail[creatorsMap[i].keyName].toArray(), delimiterComma,
                    wrapInLink, keyName, keyId);
            label->setText(creatorsMap[i].label.arg(joinedText));
        });

        m_verticalLayoutCreators->addWidget(label);
    }
}

void MovieDetailDialog::prepareMovieDetailComboBox()
{
    // Nothing to render
    if (m_movieSearchResult.isEmpty()) {
        ui->movieDetailComboBox->setEnabled(false);
        ui->movieDetailComboBox->setVisible(false);
        return;
    }

    populateMovieDetailComboBox();

    connect(ui->movieDetailComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MovieDetailDialog::movieDetailComboBoxChanged);
}

void MovieDetailDialog::populateMovieDetailComboBox()
{
    for (const auto &searchItem : qAsConst(m_movieSearchResult)) {
        const auto itemObject = searchItem.toObject();
        const auto name = itemObject["name"].toString();
        const auto year = itemObject["year"].toInt();
        const auto typeRaw = itemObject["type"];
        // Compose item
        auto item = QString("%1 (%2)").arg(name).arg(year);
        if (!typeRaw.isNull())
            item += " - " + typeRaw.toString();

        ui->movieDetailComboBox->addItem(item, itemObject["id"].toVariant());
    }

    // Preselect right movie detail
    ui->movieDetailComboBox->setCurrentIndex(
                m_selectedTorrent.value("movie_detail_index").toInt());
}

QIcon MovieDetailDialog::getFlagIcon(const QString &countryIsoCode) const
{
    if (countryIsoCode.isEmpty()) return {};

    const QString key = countryIsoCode.toLower();
    // Return from flag cache
    const auto iter = m_flagCache.find(key);
    if (iter != m_flagCache.end())
        return *iter;

    const QIcon icon {QStringLiteral(":/icons/flags/") + key + QStringLiteral(".svg")};
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
    // Recenter movie detail dialog
    Utils::Gui::centerDialog(this);
    // Recompute title elide
    renderTitleSection();
}

void MovieDetailDialog::saveButtonClicked()
{
    const auto movieDetailIndex = ui->movieDetailComboBox->currentIndex();
    const auto result =
            CsfdDetailService::instance()->updateObtainedMovieDetailInDb(
                m_selectedTorrent, m_movieDetail, m_movieSearchResult,
                movieDetailIndex);
    if (result != 0)
        return;

    m_movieDetailIndex = movieDetailIndex;
    ui->saveButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    ui->saveButton->hide();
    m_saveButton->hide();
}

void MovieDetailDialog::previewButtonClicked()
{
    emit readyToPreviewFile();
}

void MovieDetailDialog::movieDetailComboBoxChanged(const int index)
{
    const auto filmId = ui->movieDetailComboBox->currentData().toULongLong();
    qDebug() << "Movie detail ComboBox changed, current csfd id :"
             << filmId;
    qDebug() << "Selected movie detail index :" << index;

    prepareData(filmId);

    if (index != m_movieDetailIndex) {
        ui->saveButton->setEnabled(true);
        m_saveButton->setEnabled(true);
        ui->saveButton->show();
        m_saveButton->show();
        return;
    }

    ui->saveButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    ui->saveButton->hide();
    m_saveButton->hide();
}
