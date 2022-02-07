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

#include <array>

#include "csfddetailservice.h"
#include "torrentstatus.h"
#include "utils/gui.h"
#include "utils/misc.h"

MovieDetailDialog::MovieDetailDialog(
        const std::shared_ptr<CsfdDetailService> &csfdDetailService, // NOLINT(modernize-pass-by-value)
        QWidget *const parent
)
    : QDialog(parent)
    , m_statusHash(StatusHash::instance())
    , m_ui(std::make_unique<Ui::MovieDetailDialog>())
    , m_csfdDetailService(csfdDetailService)
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

    // CUR add force reload without cache button silverqx
    // Initialize ui widgets
    m_ui->saveButton->setEnabled(false);
    m_ui->saveButton->hide();

    // Preview button
    auto *const previewButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    previewButton->setText(QStringLiteral("&Preview"));

    // Save button
    m_saveButton = m_ui->buttonBox->button(QDialogButtonBox::Save);
    m_saveButton->setEnabled(false);
    m_saveButton->hide();
    m_saveButton->setText(QStringLiteral("&Save"));
    // CUR create tooltip wrapper function for tooltips with shortcuts, also set tooltipduration silverqx
    m_saveButton->setToolTip("<html><head/><body><p style='white-space: pre;'>"
                             "Save current movie detail as default "
                             "<br><span style='font-size:7pt; color:#8c8c8c;'>ALT+S</span></p>"
                             "</body></html>");
    m_saveButton->setToolTipDuration(2500);

    // Close button
    m_ui->buttonBox->button(QDialogButtonBox::Close)->setText(QStringLiteral("&Close"));

    // Ensure recenter of the dialog after resize
    m_resizeTimer = new QTimer(this); // NOLINT(cppcoreguidelines-owning-memory)
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(100); // clazy:exclude=use-chrono-in-qtimer
    connect(m_resizeTimer, &QTimer::timeout, this, &MovieDetailDialog::resizeTimeout);

    // Initialize NetworkAccessManager
    auto *const diskCache = new QNetworkDiskCache(this); // NOLINT(cppcoreguidelines-owning-memory)
    diskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_networkManager.setCache(diskCache);

    // Connect events
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailDialog::finishedMoviePoster);
    // saveButton
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MovieDetailDialog::saveButtonClicked);
    // buttonBox
    connect(m_saveButton, &QPushButton::clicked, this, &MovieDetailDialog::saveButtonClicked);
    connect(previewButton, &QPushButton::clicked, this, &MovieDetailDialog::readyToPreviewFile);
    // Order is important, have to be last
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

    // Center on an active screen
    Utils::Gui::centerDialog(this);
    // Event filter to detect resizing done
    qApp->installEventFilter(this); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

void MovieDetailDialog::prepareData(const QSqlRecord &torrent)
{
    // Create/save as data members for easier access
    m_selectedTorrent = torrent;
    m_movieDetailIndex = torrent.value("movie_detail_index").toInt();

    // Obtain movie detail from čsfd
    const auto movieDetail = m_csfdDetailService->getMovieDetail(torrent);
    m_movieDetail = movieDetail["detail"].toObject();
    m_movieSearchResults = movieDetail["search"].toArray();

    populateUi();
    m_initialPopulate = false;
}

void MovieDetailDialog::resizeEvent(QResizeEvent *const event)
{
    m_resizeInProgress = true;

    QDialog::resizeEvent(event);
}

void MovieDetailDialog::showEvent(QShowEvent *const /*unused*/)
{
    // Title
    renderTitleSection();
}

bool MovieDetailDialog::eventFilter(QObject *const watched, QEvent *const event)
{
    const auto eventType = event->type();
    /* We need to check for both types of mouse release, because it can vary on which type
       happened, when resizing. */
    if (eventType == QEvent::MouseButtonRelease ||
        eventType == QEvent::NonClientAreaMouseButtonRelease
    ) {
        auto *const mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (m_resizeInProgress && mouseEvent->button() == Qt::MouseButton::LeftButton) {
            m_resizeInProgress = false;
            qDebug() << "Resizing of the movie detail dialog ended";
            // I don't delete this timer logic and reuse it, even if it worked without it
            m_resizeTimer->start();
        }
    }

    return QDialog::eventFilter(watched, event);
}

void MovieDetailDialog::populateUi()
{
    // Movie poster, start as soon as possible, because it's async
    prepareMoviePosterSection();

    // Modal dialog title
    const auto movieTitle = m_movieDetail["title"].toString();
    // TODO resolve utf8 boom and /source-charset:utf-8 silverqx
    setWindowTitle(QStringLiteral(" Detail filmu %1  (čsfd.cz)").arg(movieTitle));

    // Title section
    /* Torrent status icon before movie titles, it is the same for all movie details
       so rendered on the initial populate only. */
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

void MovieDetailDialog::prepareMoviePosterSection()
{
    // TODO handle errors silverqx
    // TODO add empty poster, if poster missing silverqx
    auto poster = m_movieDetail["poster"].toString();
    Q_ASSERT_X(poster.count("/resized/w") == 0 ||
               (poster.count("/resized/w") == 1 && poster.count("/resized/w1080/") == 1),
               "prepareMoviePosterSection()",
               "poster url resized check");

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

    /*! Remove all items from passed layout. */
    const auto wipeOutLayout = [](QLayout &layout)
    {
        // Nothing to remove
        if (layout.count() == 0)
            return;

        QLayoutItem *layoutItem = nullptr;
        while ((layoutItem = layout.takeAt(0)) != nullptr) {
            delete layoutItem->widget(); // NOLINT(cppcoreguidelines-owning-memory)
            delete layoutItem; // NOLINT(cppcoreguidelines-owning-memory)
        }
    };
} // namespace

void MovieDetailDialog::prepareTitlesSection()
{
    // Create grid for flags and titles
    // Create even when there is nothing to render, so I don't have to manage positioning
    if (m_gridLayoutTitles.isNull()) {
        m_gridLayoutTitles = new QGridLayout(this); // NOLINT(cppcoreguidelines-owning-memory)
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
        qDebug().noquote() << "Empty titles for movie :"
                           << m_movieDetail["title"].toString();
        return;
    }

    // Populate grid layout with flags and titles
    renderTitlesSection(3);
}

namespace
{
    // Code below is for my prefered sorting of the movie titles and for a flag lookup
    /*! Title value used in titles sorting. */
    struct TitleValue
    {
        uint priority = 0;
        QString flag;
    };

    // TODO populate missing flags silverqx
    /*! Maps countries to positions, determines how countries will be sorted. */
    const auto titlesHash = []() -> const QHash<QString, TitleValue> &
    {
        static const QHash<QString, TitleValue> cached {
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
    /*! Position in sorting if country was not found in the titlesHash, the position
        will be after the last title, so at the end. */
    const auto titlesPriorityHashNew = titlesHash().size() + 1;

    /*! Compare titles callback, used by std::sort(). */
    const auto compareTitlesByLang = [](const QJsonValue &left, const QJsonValue &right)
    {
        // TODO if two keys are same and one is pracovní název, so flag it and give him titlesPriorityHashNew priority, see eg how to train dragon 3 silverqx
        const auto leftTmp = titlesHash().value(left["country"].toString()).priority;
        const auto rightTmp = titlesHash().value(right["country"].toString()).priority;

        return (leftTmp == 0 ? titlesPriorityHashNew : leftTmp) <
                (rightTmp == 0 ? titlesPriorityHashNew : rightTmp);
    };

    /*! Create a more link, used in the pagination. */
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
} // namespace

/*! swap function for QJsonValueRef, needed for sorting QJsonArray. */
void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

void MovieDetailDialog::renderTitlesSection(const int maxLines) const
{
    auto titlesOtherArr = m_movieDetail["titlesOther"].toArray();

    // My preferred sort
    std::sort(titlesOtherArr.begin(), titlesOtherArr.end(), compareTitlesByLang);

    // Setup font
    auto titlesFont = this->font();
    titlesFont.setFamily(QStringLiteral("Arial"));
    titlesFont.setPointSize(12);
    titlesFont.setBold(true);
    titlesFont.setKerning(true);

    const auto titlesArrSize = titlesOtherArr.size();

    // Determine whether to show pagination
    auto showMoreLink = maxLines > 0 && titlesArrSize >= maxLines;
    // How much titles to render
    const auto titlesToRender = showMoreLink ? maxLines - 1 : titlesArrSize;

    // Populate grid layout with flags and titles
    for (auto i = 0; i < titlesToRender ; ++i) {
        const auto titleObject = titlesOtherArr[i].toObject();
        const auto titleCountry = titleObject["country"].toString();

        // Flag
        auto *const labelFlag = new QLabel; // NOLINT(cppcoreguidelines-owning-memory)
        if (titlesHash().contains(titleCountry)) {
            const auto flagIcon = getFlagIcon(titlesHash().value(titleCountry).flag);
            const auto flag = flagIcon.pixmap(flagIcon.actualSize(QSize(flagWidth, 16)));
            labelFlag->setPixmap(flag);
        } else {
            qDebug().noquote() << "titlesHash doesn't contain this language :"
                               << titleCountry;
        }
        labelFlag->setToolTip(titleCountry);
        labelFlag->setToolTipDuration(2000);

        // Title
        auto *const labelTitle = new QLabel; // NOLINT(cppcoreguidelines-owning-memory)
        labelTitle->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                            Qt::TextSelectableByKeyboard);
        labelTitle->setFont(titlesFont);
        labelTitle->setText(titleObject["title"].toString());

        // Render
        m_gridLayoutTitles->addWidget(labelFlag, i, 0);
        m_gridLayoutTitles->addWidget(labelTitle, i, 1);
    }

    // No pagination needed
    if (!showMoreLink)
        return;

    // Render more link
    auto *const labelMoreLink = new QLabel; // NOLINT(cppcoreguidelines-owning-memory)
    labelMoreLink->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                           Qt::LinksAccessibleByKeyboard);
    labelMoreLink->setText(moreLinkText(false, false));
    m_gridLayoutTitles->addWidget(labelMoreLink, maxLines, 1);
    // Connect click event
    QObject::connect(labelMoreLink, &QLabel::linkActivated, [this](const QString &link)
    {
        if (link != QLatin1String("#show-more"))
            return;

        // Remove all items from grid layout
        wipeOutLayout(*m_gridLayoutTitles);

        // Re-render whole section again
        renderTitlesSection();
    });
}

namespace
{
    // Common delimiters
    const auto delimiterSlash       = QStringLiteral(" / ");
    const auto delimiterComma       = QStringLiteral(", ");
    const auto delimiterHtmlNewLine = QStringLiteral("<br>");

    /*! Join QJsonArray, all values in array have to be strings. */
    const auto joinJsonStringArray =
            [](const QJsonArray &jsonArray, const auto &delimiter)
    {
        QStringList result;

        for (const auto &jsonValue : jsonArray) {
            if (!jsonValue.isString() || jsonValue.isNull() || jsonValue.isUndefined())
                continue;

            const auto value = jsonValue.toString();
            if (value.isEmpty())
                continue;

            result << value;
        }

        return result.join(delimiter);
    };

    /*! Join QStringList, exclude empty or null values. */
    const auto joinStringList = [](const auto &stringList, const auto &delimiter)
    {
        QString result = "";
        int count = 0;

        for (const auto &value : stringList) {
            if (value.isEmpty() || value.isNull())
                continue;

            result += (value + delimiter);
            ++count;
        }

        if (count > 0)
            result.chop(delimiter.size());

        return result;
    };
} // namespace

void MovieDetailDialog::prepareMovieInfoSection() const
{
    // Movie info section - genres, origins, year and length
    // Line 1
    // Genres
    auto genres = joinJsonStringArray(m_movieDetail["genres"].toArray(), delimiterSlash);

    // Line 2
    // Shot places
    auto origins = joinJsonStringArray(m_movieDetail["origins"].toArray(), delimiterSlash);

    // Duration
    QString duration;
    if (const auto durationRaw = m_movieDetail["duration"];
        !durationRaw.isNull() && !durationRaw.isUndefined()
    ) {
        const auto durationValue = durationRaw.toInt();
        const auto lengthString =
                durationValue > 180
                ? Utils::Misc::userFriendlyDuration(
                      durationValue, Utils::Misc::DURATION_INPUT::MINUTES)
                : QStringLiteral("%1 min").arg(durationValue);

        duration = QStringLiteral("<span style='color: palette(link);'>%1</span>")
                   .arg(lengthString);
    }

    // Join line 2 segments
    QVector<QString> movieInfoLine2List;
    movieInfoLine2List << std::move(origins)
                       << QString::number(m_movieDetail["year"].toInt())
                       << std::move(duration);
    const auto movieInfoLine2 = joinStringList(movieInfoLine2List, delimiterComma);

    // Assemble movie info section
    QVector<QString> movieInfoList;
    movieInfoList << std::move(genres)
                  << movieInfoLine2;
    const auto movieInfo = joinStringList(movieInfoList, delimiterHtmlNewLine);

    // Render
    m_ui->movieInfo->setText(movieInfo);
    // TODO create logging system silverqx
    // TODO log empty movieInfo, to know how often it happens silverqx
}

void MovieDetailDialog::prepareImdbLink() const
{
    // Currently, čsfd provides imdb id for logged users only, so it will always be hidden
    const auto imdbIdRaw = m_movieDetail["imdbId"];
    if (imdbIdRaw.isNull() || imdbIdRaw.isUndefined()) {
        m_ui->imdbLink->setEnabled(false);
        m_ui->imdbLink->hide();
        return;
    }

    m_ui->imdbLink->setText(
                QStringLiteral("<a href='https://www.imdb.com/title/%1/' "
                               "style='color: #64a1ac; text-decoration: none;'>imdb</a>")
                .arg(imdbIdRaw.toString()));
    m_ui->imdbLink->setEnabled(true);
    m_ui->imdbLink->show();
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

    struct CreatorValue
    {
        QString keyName;
        QString label;
    };

    const std::array<CreatorValue, 4> creatorsMap {{
        {QStringLiteral("directors"), QStringLiteral("<strong>Réžia: </strong>%1")},
        {QStringLiteral("writers"),   QStringLiteral("<strong>Scenár: </strong>%1")},
        {QStringLiteral("music"),     QStringLiteral("<strong>Hudba: </strong>%1")},
        {QStringLiteral("actors"),    QStringLiteral("<strong>Herci: </strong>%1")},
    }};

    const auto moreLinkSize = QStringLiteral("... (more)").size();

    /*! Join QJsonArray, all values in the array have to be QJsonObject, values will be
        searched in the QJsonObject by keys stored in the args pack and wrapped in the wrapIn.
        If the maxLetters argument contains a value greater than 0, then the result will be
        cut and the more link will be shown at the end. Letters will be counted only during
        the first substitution (first key in the args pack). The resulting string can
        contain fewer letters, because when the maxLetters limit will be hit or overflowed,
        then the currently processed string will not be included at all. */
    const auto joinJsonObjectArrayWithWrapPaged =
            []<typename ...Args>
            (const QJsonArray &jsonArray, const QString &delimiter,
             const QString &wrappIn, const int maxLetters, const Args &...args)
    {
        const auto argsSize = sizeof ...(args);
        if (argsSize == 0)
            qCritical() << "Empty argsSize in joinJsonObjectArrayWithWrap()";

        QString result;
        int itemsCounter = 0;
        int lettersCounter = 0;
        const auto paginateEnabled = (maxLetters != 0);
        auto showMoreLink = false;
        const auto delimiterSize = delimiter.size();
        const QStringList argsList {(args)...};

        for (const auto &jsonValue : jsonArray) {
            if (!jsonValue.isObject() || jsonValue.isNull() || jsonValue.isUndefined())
                continue;

            auto wrapped = wrappIn;

            // Obtain a value from QJsonObject by arg and replace tokens in wrapped
            for (const auto &arg : argsList) {
                const auto jsonValueInner = jsonValue[arg];
                const auto jsonValueType = jsonValueInner.type();

                QString value;
                switch (jsonValueType) {
                case QJsonValue::String:
                    value = jsonValueInner.toString();
                    break;
                case QJsonValue::Double:
                    value = QString::number(jsonValueInner.toDouble());
                    break;
                default:
                    qWarning().noquote()
                            << QStringLiteral("Unsupported jsonValueType '%1' in "
                                              "joinJsonObjectArrayWithWrap()")
                               .arg(jsonValueType);
                    break;
                }

                // Letters are counted only for the first substitution
                if (paginateEnabled && (arg == argsList[0]))
                    lettersCounter += value.size() + delimiterSize;

                wrapped = wrapped.arg(value);
            }

            /* Last delimiter will never be in the result, also take a more text
               link size into account. */
            const auto lettersCount = lettersCounter - delimiterSize + moreLinkSize;

            // lettersCount overflowed 😎
            if (paginateEnabled && (lettersCount >= maxLetters)) {
                showMoreLink = true;
                break;
            }

            result += wrapped.append(delimiter);
            ++itemsCounter;
        }

        if (itemsCounter > 0)
            result.chop(delimiter.size());

        // Append show more link if needed
        if (showMoreLink)
            result.append(moreLinkText());

        return result;
    };

    /*! Join QJsonArray without pagination. All values in an array have to be QJsonObject,
        values will be searched in the jsonArray by keys stored in the args pack and
        wrapped in wrapIn. */
    const auto joinJsonObjectArrayWithWrap =
            []<typename ...Args>
            (const QJsonArray &jsonArray, const QString &delimiter,
             const QString &wrappIn, Args &&...args)
    {
        return joinJsonObjectArrayWithWrapPaged(
                jsonArray, delimiter, wrappIn, 0, std::forward<Args>(args)...);
    };
} // namespace

void MovieDetailDialog::prepareCreatorsSection()
{
    // Creators section
    const auto keyName    = QStringLiteral("name");
    const auto keyUrl     = QStringLiteral("url");
    const auto wrapInLink = QStringLiteral("<a href='%2' "
                                           "style='text-decoration: none;'>%1</a>");
    // 60 for 960px
    // 105 for 1316px
    // TODO compute dynamically by width silverqx
    static const auto maxLetters = 105;

    const auto creators = m_movieDetail["creators"].toObject();

    // Directors
    const auto directors =
            joinJsonObjectArrayWithWrapPaged(
                creators[creatorsMap[NAMES_DIRECTORS].keyName].toArray(),
                delimiterComma, wrapInLink, maxLetters, keyName, keyUrl);
    // Screenplay
    const auto screenplay =
            joinJsonObjectArrayWithWrapPaged(
                creators[creatorsMap[NAMES_WRITERS].keyName].toArray(),
                delimiterComma, wrapInLink, maxLetters, keyName, keyUrl);
    // TODO camera is missing silverqx
    // Music
    const auto music =
            joinJsonObjectArrayWithWrapPaged(
                creators[creatorsMap[NAMES_MUSIC].keyName].toArray(),
                delimiterComma, wrapInLink, maxLetters, keyName, keyUrl);
    // Actors
    const auto actors =
            joinJsonObjectArrayWithWrapPaged(
                creators[creatorsMap[NAMES_ACTORS].keyName].toArray(),
                // 200 for 960px
                // 320 for 1316px
                delimiterComma, wrapInLink, 320, keyName, keyUrl);

    // Assemble creators section
    // TODO fix the same width for every section title silverqx
    // Order is important, have to be same as order in creatorsMap enum
    const QVector<QString> creatorsList {
        directors,
        screenplay,
        music,
        actors,
    };

    // Create a layout for creators
    // Create even when there is nothing to render, so I don't have to manage positioning
    if (m_verticalLayoutCreators.isNull()) {
        m_verticalLayoutCreators = new QVBoxLayout(this); // NOLINT(cppcoreguidelines-owning-memory)
        m_verticalLayoutCreators->setSpacing(2);
        m_ui->verticalLayoutInfo->addLayout(m_verticalLayoutCreators);
    }

    // Remove all items from box layout
    if (!m_initialPopulate)
        wipeOutLayout(*m_verticalLayoutCreators);

    // If all creator sections are empty, then nothing to render
    const auto result = std::find_if(creatorsList.constBegin(), creatorsList.constEnd(),
                                     [](const QString &creators)
    {
        return !creators.isEmpty();
    });
    // All was empty
    if (result == creatorsList.constEnd()) {
        qDebug().noquote() << "Empty creators for movie :"
                           << m_movieDetail["title"].toString();
        return;
    }

    // Render each creators section into the layout
    QLabel *label = nullptr;
    // Prepare font
    auto font = this->font();
    font.setFamily("Arial");
    font.setPointSize(12);
    font.setKerning(true);

    // Render creators sections
    for (int i = 0; i < creatorsList.size(); ++i) {
        // Skip empty section
        if (creatorsList[i].isEmpty())
            continue;

        // Prepare section label
        label = new QLabel(this); // NOLINT(cppcoreguidelines-owning-memory)
        label->setWordWrap(true);
        label->setTextInteractionFlags(
                    Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard |
                    Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
        label->setFont(font);
        label->setOpenExternalLinks(false);
        label->setText(creatorsMap.at(i).label.arg(creatorsList[i]));
        // Connect click event
        QObject::connect(label, &QLabel::linkActivated, this,
                         [creators, label, wrapInLink, keyName, keyUrl, i]
                         (const QString &link)
        {
            // Open URL with external browser
            if (link != QLatin1String("#show-more")) {
                QDesktopServices::openUrl(QUrl(link, QUrl::StrictMode));
                return;
            }

            // Show more link clicked, re-populate current section label
            const auto &creatorValue = creatorsMap.at(i);
            const auto joinedText =
                    joinJsonObjectArrayWithWrap(
                        creators[creatorValue.keyName].toArray(), delimiterComma,
                        wrapInLink, keyName, keyUrl);

            label->setText(creatorValue.label.arg(joinedText));
        });

        m_verticalLayoutCreators->addWidget(label);
    }
}

void MovieDetailDialog::prepareMovieDetailComboBox()
{
    // Nothing to render
    if (m_movieSearchResults.isEmpty()) {
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
    for (const auto &searchItem : std::as_const(m_movieSearchResults)) {
        const auto itemObject = searchItem.toObject();
        const auto title = itemObject["title"].toString();
        const auto year = itemObject["year"].toInt();
        const auto typeRaw = itemObject["type"];

        // Movie type
        QString type;
        if (!typeRaw.isNull() && !typeRaw.isUndefined())
            type = QStringLiteral(" - %1").arg(typeRaw.toString());

        // Compose item
        auto item = QStringLiteral("%1 (%2)%3")
                    .arg(title).arg(year)
                    .arg(type.isEmpty() ? "" : type);

        m_ui->movieDetailComboBox->addItem(item, itemObject["id"].toVariant());
    }

    // Preselect right movie detail
    m_ui->movieDetailComboBox->setCurrentIndex(
                m_selectedTorrent.value("movie_detail_index").toInt());
}

QIcon MovieDetailDialog::getFlagIcon(const QString &countryIsoCode) const
{
    if (countryIsoCode.isEmpty())
        return {};

    const auto key = countryIsoCode.toLower();

    // Return from the flags cache
    if (m_flagsCache.contains(key))
        return m_flagsCache.value(key);

    QIcon icon {QStringLiteral(":/icons/flags/%1.svg").arg(key)};
    // Save to the flags cache
    m_flagsCache[key] = icon;

    return icon;
}

void MovieDetailDialog::prepareData(const quint64 filmId)
{
    // Used when combobox changed
    const auto movieDetail = m_csfdDetailService->getMovieDetail(filmId);
    m_movieDetail = movieDetail.object();

    populateUi();
}

void MovieDetailDialog::toggleSaveButton(const bool enable)
{
    m_ui->saveButton->setEnabled(enable);
    m_saveButton->setEnabled(enable);

    if (enable) {
        m_ui->saveButton->show();
        m_saveButton->show();
    } else {
        m_ui->saveButton->hide();
        m_saveButton->hide();
    }
}

void MovieDetailDialog::finishedMoviePoster(QNetworkReply *const reply) const
{
    // TODO handle network errors silverqx
    const auto moviePosterData = reply->readAll();

    QPixmap moviePoster;
    if (!moviePoster.loadFromData(moviePosterData))
        return;

    m_ui->poster->setPixmap(moviePoster.scaledToWidth(
                                m_ui->poster->maximumWidth(), Qt::SmoothTransformation));
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
            m_csfdDetailService->updateObtainedMovieDetailInDb(
                m_selectedTorrent, m_movieDetail, m_movieSearchResults,
                movieDetailIndex);
    if (result != 0)
        return;

    m_movieDetailIndex = movieDetailIndex;

    // Disable save button
    toggleSaveButton(false);
}

void MovieDetailDialog::movieDetailComboBoxChanged(const int index)
{
    const auto filmId = m_ui->movieDetailComboBox->currentData().toULongLong();

    qDebug() << "Movie detail ComboBox changed, current čsfd id :" << filmId;
    qDebug() << "Selected movie detail index :" << index;

    prepareData(filmId);

    // Enable save buttons if a new movie detail was selected
    if (index != m_movieDetailIndex) {
        toggleSaveButton(true);
        return;
    }

    // Hide if it is the same movie as saved in db
    toggleSaveButton(false);
}
