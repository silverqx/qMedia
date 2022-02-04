#include "moviedetaildialog.h"
#include "ui_moviedetaildialog.h"

#include <QDesktopServices>
#include <QMouseEvent>
#include <QNetworkDiskCache>
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
    const auto delimiterSlash       = QStringLiteral(" / ");
    const auto delimiterNewLine     = QStringLiteral("\n");
    const auto delimiterComma       = QStringLiteral(", ");
    const auto delimiterHtmlNewLine = QStringLiteral("<br>");

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
    const auto moreLinkText =
            [](const bool withDots = true, const bool withParenthesis = true)
    {
        return QStringLiteral("%1%2%3%4").arg(
                    withDots ? QStringLiteral("<strong>...</strong> ") : "",
                    withParenthesis ? QStringLiteral("(") : "",
                    QStringLiteral("<a href='#show-more' "
                                  "style='font-size: 11pt; text-decoration: none;'"
                                  ">more</a>"),
                    withParenthesis ? QStringLiteral(")") : "");
    };

    const auto moreLinkSize = QStringLiteral("... (more)").size();

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
        const auto argsSize = sizeof ...(args);
        if (argsSize == 0)
            qCritical() << "Empty argsSize in joinJsonObjectArrayWithWrap()";

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
            auto wrapInTmp = wrappIn;
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
                    qWarning().noquote()
                            << QStringLiteral("Unsupported jsonValueType '%1' in "
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
    const auto joinStringList =
            [](const QStringList &stringList, const QString &delimiter) -> QString
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
    const auto titlesHash = []() -> const QHash<QString, TitlesValue> &
    {
        static const QHash<QString, TitlesValue> cached {
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
        return cached;
    };
    const auto titlesPriorityHashNew = titlesHash().size() + 1;
    const auto compareTitlesByLang = [](const QJsonValue &left, const QJsonValue &right) -> bool
    {
        // TODO if two keys are same and one is pracovní název, so flag it and give him titlesPriorityHashNew priority, see eg how to train dragon 3 silverqx
        const auto leftTmp = titlesHash().value(left["language"].toString()).priority;
        const auto rightTmp = titlesHash().value(right["language"].toString()).priority;
        return (leftTmp == 0 ? titlesPriorityHashNew : leftTmp)
            < (rightTmp == 0 ? titlesPriorityHashNew : rightTmp);
    };

    /*! Resulting string will contain max. letters defined by maxLetters parameter,
        including 'show more' link text. */
    Q_DECL_UNUSED
    const auto paginateString = [](const QString &string, const int maxLetters) -> QString
    {
        // Pagination not needed
        if (string.size() <= maxLetters)
            return string;

        return QString(string).remove(maxLetters - moreLinkSize, string.size()) +
                moreLinkText();
    };

    /*! Remove all items from layout. */
    const auto wipeOutLayout = [](QLayout &layout)
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

// Needed when sorting QJsonArray.
// I need it only here, so I defined it in the cpp file.
void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

MovieDetailDialog::MovieDetailDialog(QWidget *const parent)
    : QDialog(parent)
    , m_ui(new Ui::MovieDetailDialog)
    , m_statusHash(StatusHash::instance())
{
    m_ui->setupUi(this);

    // Override design values
#ifdef VISIBLE_CONSOLE
    // Set up smaller, so I can see console output
    const auto height = minimumHeight();
#else
    const auto height = 940;
#endif
    resize(1316, height);

    // Initialize ui widgets
    m_ui->saveButton->setEnabled(false);
    m_ui->saveButton->hide();
    // Preview button
    const auto previewButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    previewButton->setText(QStringLiteral("&Preview"));
    // Save button
    m_saveButton = m_ui->buttonBox->button(QDialogButtonBox::Save);
    m_saveButton->setEnabled(false);
    m_saveButton->hide();
    m_saveButton->setText(QStringLiteral("&Save"));
    // TODO create tooltip wrapper function for tooltips with shortcuts silverqx
    m_saveButton->setToolTip("<html><head/><body><p style='white-space:pre;'>"
                             "Save current movie detail as default "
                             "<br><span style='font-size:7pt; color:#8c8c8c;'>ALT+S</span></p>"
                             "</body></html>");
    m_saveButton->setToolTipDuration(2500);
    // Close button
    m_ui->buttonBox->button(QDialogButtonBox::Close)->setText(QStringLiteral("&Close"));

    // Ensure recenter of the dialog after resize
    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(100);
    connect(m_resizeTimer, &QTimer::timeout, this, &MovieDetailDialog::resizeTimeout);

    // Initialize NetworkAccessManager
    auto *const diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_networkManager.setCache(diskCache);

    // Connect events
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailDialog::finishedMoviePoster);
    // saveButton
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MovieDetailDialog::saveButtonClicked);
    // buttonBox
    connect(m_saveButton, &QPushButton::clicked, this, &MovieDetailDialog::saveButtonClicked);
    connect(previewButton, &QPushButton::clicked, this, &MovieDetailDialog::previewButtonClicked);
    // Order important, have to be last
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    // Hotkeys
    // movieDetailComboBox
#ifdef __clang__
#  pragma clang diagnostic push
#  if __has_warning("-Wdeprecated-enum-enum-conversion")
#    pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#  endif
#endif
    const auto *const doubleClickHotkeyCtrlM =
            new QShortcut(Qt::CTRL + Qt::Key_M, m_ui->movieDetailComboBox, nullptr, nullptr,
                          Qt::WindowShortcut);
    connect(doubleClickHotkeyCtrlM, &QShortcut::activated,
            m_ui->movieDetailComboBox, qOverload<>(&QComboBox::setFocus));
    const auto *const doubleClickHotkeyF12 =
            new QShortcut(Qt::Key_F12, m_ui->movieDetailComboBox, nullptr, nullptr,
                          Qt::WindowShortcut);
    connect(doubleClickHotkeyF12, &QShortcut::activated,
            m_ui->movieDetailComboBox, &QComboBox::showPopup);
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

    // Center on active screen
    Utils::Gui::centerDialog(this);
    // Event filter to detect resize done
    qApp->installEventFilter(this);
}

MovieDetailDialog::~MovieDetailDialog()
{
    delete m_ui;
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
    const auto movieDetail = CsfdDetailService::instance()->getMovieDetail(filmId);
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
        const auto &statusIcon = (*m_statusHash)[status].icon();
        const auto statusPixmap = statusIcon.pixmap(statusIcon.actualSize(QSize(30, 20)));
        m_ui->status->setPixmap(statusPixmap);
        m_ui->status->setToolTip((*m_statusHash)[status].title);
        m_ui->status->setToolTipDuration(2500);
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
    m_ui->score->setText(QString::number(m_movieDetail["rating"].toInt()) + QStringLiteral("%"));
    // Imdb link
    prepareImdbLink();
    // Creators section
    prepareCreatorsSection();
    // Storyline section
    m_ui->storyline->setText(m_movieDetail["descriptions"].toArray().first().toString());

    if (!m_initialPopulate)
        return;

    // Fill empty space
    m_ui->verticalLayoutInfo->addStretch(1);
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
            qDebug() << "Resizing of the movie detail dialog ended";
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
    auto poster = m_movieDetail["poster"].toString();
    Q_ASSERT_X(poster.count("/resized/w1080/") == 1,
               "prepareMoviePosterSection()",
               "poster url doesn't contain 'resized/w1080/'");

    // 220px width is enough
    poster.replace("/resized/w1080/", "/resized/w220/");

    QUrl url(poster, QUrl::StrictMode);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::PreferCache);
    m_networkManager.get(request);
}

void MovieDetailDialog::renderTitleSection() const
{
    const auto movieTitle = m_movieDetail["title"].toString();
    const auto movieTitleElided = m_ui->title->fontMetrics()
                                  .elidedText(movieTitle, Qt::ElideRight, m_ui->title->width());
    m_ui->title->setText(movieTitleElided);
    m_ui->title->setToolTip(QStringLiteral("Torrent name<br><strong>%1</strong>")
                          .arg(m_selectedTorrent.value("name").toString()));
    m_ui->title->setToolTipDuration(8500);
}

namespace
{
    const int flagWidth = 21;
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
        m_ui->verticalLayoutInfoLeft->addLayout(m_gridLayoutTitles);
    }

    // Remove all items from grid layout
    if (!m_initialPopulate)
        wipeOutLayout(*m_gridLayoutTitles);

    // Nothing to render
    if (m_movieDetail["titlesOther"].toArray().isEmpty()) {
        qDebug() << "Empty titles for movie :"
                 << m_movieDetail["title"].toString();
        return;
    }

    // Populate grid layout with flags and titles
    renderTitlesSection(3);
}

void MovieDetailDialog::renderTitlesSection(const int maxLines) const
{
    auto titlesOtherArr = m_movieDetail["titlesOther"].toArray();
    // My preferred sort
    std::sort(titlesOtherArr.begin(), titlesOtherArr.end(), compareTitlesByLang);

    // Populate grid layout with flags and titles
    QLabel *labelFlag;
    QLabel *labelTitle;
    QJsonObject titleObject;
    QString titleCountry;
    QIcon flagIcon;
    QPixmap flag;
    QFont titlesFont = this->font();
    titlesFont.setFamily(QStringLiteral("Arial"));
    titlesFont.setPointSize(12);
    titlesFont.setBold(true);
    titlesFont.setKerning(true);
    const auto paginateEnabled = (maxLines != 0);
    auto showMoreLink = false;
    const auto titlesArrSize = titlesOtherArr.size();
    int i = 0;
    for (; i < titlesArrSize ; ++i) {
        titleObject = titlesOtherArr[i].toObject();
        titleCountry = titleObject["country"].toString();
        // Flag
        labelFlag = new QLabel;
        if (titlesHash().contains(titleCountry)) {
            flagIcon = getFlagIcon(titlesHash().value(titleCountry).flag);
            flag = flagIcon.pixmap(flagIcon.actualSize(QSize(flagWidth, 16)));
            labelFlag->setPixmap(flag);
        } else {
            qDebug() << "titlesHash doesn't contain this language :"
                     << titleCountry;
        }
        labelFlag->setToolTip(titleCountry);
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
    auto *const labelMoreLink = new QLabel;
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

void MovieDetailDialog::prepareImdbLink() const
{
    const auto imdbIdRaw = m_movieDetail["imdbId"];
    if (imdbIdRaw.isNull() || imdbIdRaw.isUndefined()) {
        m_ui->imdbLink->setEnabled(false);
        m_ui->imdbLink->hide();
        return;
    }

    m_ui->imdbLink->setText(QStringLiteral("<a href='https://www.imdb.com/title/%1/' "
                                         "style='color: #64a1ac; text-decoration: none;'>imdb</a>")
                          .arg(imdbIdRaw.toString()));
    m_ui->imdbLink->setEnabled(true);
    m_ui->imdbLink->show();
}

void MovieDetailDialog::prepareMovieInfoSection() const
{
    // Movie info section - genre, shot places, year and length
    // Line 1
    // Genre
    const auto genres = joinJsonStringArray(m_movieDetail["genres"].toArray(), delimiterSlash);
    // Line 2
    // Shot places
    const auto origins = joinJsonStringArray(m_movieDetail["origins"].toArray(),
            delimiterSlash);
    // Length
    QString length;
    if (const auto duration = m_movieDetail["duration"];
        !duration.isNull()
    ) {
        const auto durationValue = duration.toInt();
        const auto lengthString = (durationValue > 180)
            ? Utils::Misc::userFriendlyDuration(
                  durationValue, Utils::Misc::DURATION_INPUT::MINUTES)
            : QString::number(durationValue) + " min";
        length = "<span style='color: palette(link);'>" + lengthString + "</span>";
    }
    QStringList movieInfoLine2List;
    movieInfoLine2List << origins
                       << QString::number(m_movieDetail["year"].toInt())
                       << length;
    const auto movieInfoLine2 = joinStringList(movieInfoLine2List, delimiterComma);

    // Assemble movie info section
    QStringList movieInfoList;
    movieInfoList << genres
                  << movieInfoLine2;
    const auto movieInfo = joinStringList(movieInfoList, delimiterHtmlNewLine);
    m_ui->movieInfo->setText(movieInfo);
    // TODO create logging system silverqx
    // TODO log empty movieInfo, to know how often it happens silverqx
}

namespace
{
    enum CREATOR_NAMES
    {
        NAMES_DIRECTORS,
        NAMES_WRITERS,
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
        {QLatin1String("writers"),    QStringLiteral("<strong>Scenár: </strong>%1")},
        {QLatin1String("music"),      QStringLiteral("<strong>Hudba: </strong>%1")},
        {QLatin1String("actors"),     QStringLiteral("<strong>Herci: </strong>%1")},
    };
}

void MovieDetailDialog::prepareCreatorsSection()
{
    // Creators section
    const auto keyName    = QStringLiteral("name");
    const auto keyId      = QStringLiteral("id");
    const auto wrapInLink = QStringLiteral("<a href='https://www.csfd.cz/tvurce/%2' "
                                           "style='text-decoration: none;'>%1</a>");
    // 60 for 960px
    // 105 for 1316px
    // TODO compute dynamically by width silverqx
    static const auto maxLetters = 105;

    const auto creators = m_movieDetail["creators"].toObject();

    // Directors
    const auto directors = joinJsonObjectArrayWithWrapPaged(
            creators[creatorsMap[NAMES_DIRECTORS].keyName].toArray(),
            delimiterComma, wrapInLink, maxLetters, keyName, keyId);
    // Screenplay
    const auto screenplay = joinJsonObjectArrayWithWrapPaged(
            creators[creatorsMap[NAMES_WRITERS].keyName].toArray(),
            delimiterComma, wrapInLink, maxLetters, keyName, keyId);
    // TODO camera is missing silverqx
    // Music
    const auto music = joinJsonObjectArrayWithWrapPaged(
            creators[creatorsMap[NAMES_MUSIC].keyName].toArray(),
            delimiterComma, wrapInLink, maxLetters, keyName, keyId);
    // Actors
    const auto actors = joinJsonObjectArrayWithWrapPaged(
            creators[creatorsMap[NAMES_ACTORS].keyName].toArray(),
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
        m_ui->verticalLayoutInfo->addLayout(m_verticalLayoutCreators);
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
        qDebug() << "Empty creators for movie :"
                 << m_movieDetail["title"].toString();
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
                       [creators, label, wrapInLink, keyName, keyId, i]
                       (const QString &link)
        {
            // Open URL with external browser
            if (link != QLatin1String("#show-more")) {
                QDesktopServices::openUrl(QUrl(link, QUrl::StrictMode));
                return;
            }

            // Re-populate label
            const auto joinedText = joinJsonObjectArrayWithWrap(
                    creators[creatorsMap[i].keyName].toArray(), delimiterComma,
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
        m_ui->movieDetailComboBox->setEnabled(false);
        m_ui->movieDetailComboBox->setVisible(false);
        return;
    }

    populateMovieDetailComboBox();

    connect(m_ui->movieDetailComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MovieDetailDialog::movieDetailComboBoxChanged);
}

void MovieDetailDialog::populateMovieDetailComboBox() const
{
    for (const auto &searchItem : std::as_const(m_movieSearchResult)) {
        const auto itemObject = searchItem.toObject();
        const auto title = itemObject["title"].toString();
        const auto year = itemObject["year"].toInt();
        const auto typeRaw = itemObject["type"];
        // Compose item
        auto item = QStringLiteral("%1 (%2)").arg(title).arg(year);
        if (!typeRaw.isNull())
            item += " - " + typeRaw.toString();

        m_ui->movieDetailComboBox->addItem(item, itemObject["id"].toVariant());
    }

    // Preselect right movie detail
    m_ui->movieDetailComboBox->setCurrentIndex(
                m_selectedTorrent.value("movie_detail_index").toInt());
}

QIcon MovieDetailDialog::getFlagIcon(const QString &countryIsoCode) const
{
    if (countryIsoCode.isEmpty()) return {};

    const auto key = countryIsoCode.toLower();
    // Return from flag cache
    const auto iter = m_flagCache.find(key);
    if (iter != m_flagCache.end())
        return *iter;

    const QIcon icon {QStringLiteral(":/icons/flags/") + key + QStringLiteral(".svg")};
    // Save to flag cache
    m_flagCache[key] = icon;
    return icon;
}

void MovieDetailDialog::finishedMoviePoster(QNetworkReply *reply) const
{
    // TODO handle network errors silverqx
    // TODO avangers poster not downloaded silverqx
    const auto moviePosterData = reply->readAll();
    QPixmap moviePoster;
    moviePoster.loadFromData(moviePosterData);
    // TODO try to avoid this, best would be scaled at loadFromData() right away silverqx
    moviePoster = moviePoster.scaledToWidth(m_ui->poster->maximumWidth(), Qt::SmoothTransformation);
    m_ui->poster->setPixmap(moviePoster);
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
    const auto movieDetailIndex = m_ui->movieDetailComboBox->currentIndex();
    const auto result =
            CsfdDetailService::instance()->updateObtainedMovieDetailInDb(
                m_selectedTorrent, m_movieDetail, m_movieSearchResult,
                movieDetailIndex);
    if (result != 0)
        return;

    m_movieDetailIndex = movieDetailIndex;
    m_ui->saveButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_ui->saveButton->hide();
    m_saveButton->hide();
}

void MovieDetailDialog::previewButtonClicked()
{
    emit readyToPreviewFile();
}

void MovieDetailDialog::movieDetailComboBoxChanged(const int index)
{
    const auto filmId = m_ui->movieDetailComboBox->currentData().toULongLong();
    qDebug() << "Movie detail ComboBox changed, current csfd id :"
             << filmId;
    qDebug() << "Selected movie detail index :"
             << index;

    prepareData(filmId);

    if (index != m_movieDetailIndex) {
        m_ui->saveButton->setEnabled(true);
        m_saveButton->setEnabled(true);
        m_ui->saveButton->show();
        m_saveButton->show();
        return;
    }

    m_ui->saveButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_ui->saveButton->hide();
    m_saveButton->hide();
}
