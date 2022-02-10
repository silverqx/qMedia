#include "settings/settingsstorage.h"

#include <QDateTime>
#include <QDebug>
#include <QtSql/QSqlError>

#include <chrono>

#include "macros/likely.h"

namespace Settings
{

std::shared_ptr<SettingsStorage> SettingsStorage::m_instance;

SettingsStorage::SettingsStorage()
    : m_data(readAll())
{
    // Prepare save timer
    using namespace std::chrono_literals;

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(5s);
    connect(&m_saveTimer, &QTimer::timeout, this, &SettingsStorage::save);
}

SettingsStorage::~SettingsStorage()
{
    save();
}

std::shared_ptr<SettingsStorage> SettingsStorage::instance()
{
    if (m_instance) T_LIKELY
        return m_instance;
    else T_UNLIKELY
        return m_instance = std::shared_ptr<SettingsStorage>(new SettingsStorage());
}

void SettingsStorage::freeInstance()
{
    if (m_instance)
        m_instance.reset();
}

void SettingsStorage::removeValue(const QString &key)
{
    const QWriteLocker locker(&m_lock);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_data.remove(key))
#else
    if (m_data.remove(key) > 0)
#endif
    {
        m_dirty = true;

        m_saveTimer.start();
    }
}

bool SettingsStorage::hasKey(const QString &key) const
{
    const QReadLocker locker {&m_lock};

    return m_data.contains(key);
}

bool SettingsStorage::save()
{
    // Guard for `m_dirty` too
    const QWriteLocker locker(&m_lock);

    // Nothing to save
    if (!m_dirty)
        return true;

    if (!writeAll())
    {
        m_saveTimer.start();
        return false;
    }

    // Done
    m_dirty = false;

    return true;
}

QVariant
SettingsStorage::loadValueImpl(const QString &key, const QVariant &defaultValue) const
{
    const QReadLocker locker(&m_lock);

    return m_data.value(key, defaultValue);
}

void SettingsStorage::storeValueImpl(const QString &key, const QVariant &value)
{
    const QWriteLocker locker(&m_lock);

    auto &currentValue = m_data[key]; // clazy:exclude=detaching-member

    // Nothing to save, same value
    if (currentValue == value)
        return;

    m_dirty = true;
    currentValue = value;

    m_saveTimer.start();
}

QVariantHash SettingsStorage::readAll()
{
    QSqlQuery query;
    query.setForwardOnly(true);

    const bool ok = query.exec(QStringLiteral("SELECT * FROM settings"));
    if (!ok) {
        qDebug().noquote() << "Select of all settings failed :"
                           << query.lastError().text();
        return {};
    }

    QVariantHash data;

    while (query.next()) {
        data[query.value("name").toString()] = query.value("value");
    }

    return data;
}

bool SettingsStorage::writeAll() const
{
    // WriteLocker called in save() method

    // I have decided not to use transactions

    auto [updatedSettings, newSettings] = prepareWriteData();

    const auto dataSize = m_data.size();
    Q_ASSERT(dataSize >= updatedSettings.size() &&
             dataSize >= newSettings.size());

    /* I will not create to variables for these, they have three meanings, store whether
       QHash is empty and after that are used to store a return value and also if
       QHash is empty then a default true value is important, weirdo. 😮😎 */
    auto updateResult = updatedSettings.isEmpty();
    auto newResult = newSettings.isEmpty();

    if (!updateResult)
        updateResult = writeUpdated(std::move(updatedSettings));

    if (!newResult)
        newResult = writeNew(std::move(newSettings));

    return updateResult && newResult;
}

SettingsStorage::WriteSettingsData SettingsStorage::prepareWriteData() const
{
    // Data stored in the database
    const auto dbData = readAll();

    QVariantHash updatedSettings;
    QVariantHash newSettings;

    for (auto itData = m_data.constKeyValueBegin();
         itData != m_data.constKeyValueEnd(); ++itData
    ) {
        const auto &name = itData->first;
        const auto &value = itData->second;

        if (!dbData.contains(name))
            newSettings[name] = value;

        else
            if (const auto &dbValue = dbData.find(name).value();
                value != dbValue
            )
                updatedSettings[name] = value;
    }

    return {std::move(updatedSettings), std::move(newSettings)};
}

bool SettingsStorage::writeUpdated(QVariantHash &&data) const
{
    Q_ASSERT(!data.isEmpty());

    const auto dataSize = data.size();

    // Assemble query binding placeholders
    auto placeholders = QStringLiteral("value = ?, updated_at = ?, ").repeated(dataSize);
    placeholders.chop(2);

    auto queryStringTmpl = QStringLiteral("UPDATE settings SET %1 WHERE name = ?");

    int updated = 0;

    for (auto itData = data.constKeyValueBegin();
         itData != data.constKeyValueEnd(); ++itData
    ) {
        QSqlQuery query;
        query.prepare(queryStringTmpl.arg(placeholders));

        query.addBindValue(itData->second);
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(itData->first);

        if (!query.exec()) {
            qDebug().noquote()
                    << QStringLiteral("Update of changed setting value '%1' failed : %2")
                       .arg(itData->first, query.lastError().text());
        }

        ++updated;
    }

    return updated == dataSize;
}

bool SettingsStorage::writeNew(QVariantHash &&data) const
{
    Q_ASSERT(!data.isEmpty());

    // Assemble query binding placeholders
    auto placeholders = QStringLiteral("(?, ?, ?, ?), ").repeated(data.size());
    placeholders.chop(2);

    QSqlQuery query;
    query.prepare(QStringLiteral(
                      "INSERT INTO settings (name, value, created_at, updated_at) VALUES %1")
                  .arg(placeholders));

    for (auto itData = data.constKeyValueBegin();
         itData != data.constKeyValueEnd(); ++itData
    ) {
        query.addBindValue(itData->first);
        query.addBindValue(itData->second);
        const auto timestamp = QDateTime::currentDateTime();
        query.addBindValue(timestamp);
        query.addBindValue(timestamp);
    }

    if (!query.exec()) {
        qDebug().noquote() << "Insert of a new setting values failed :"
                           << query.lastError().text();
        return false;
    }

    return true;
}

} // namespace Settings
