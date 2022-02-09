#pragma once
#ifndef SETTINGS_SETTINGSSTORAGE_H
#define SETTINGS_SETTINGSSTORAGE_H

#include <QObject>
#include <QReadWriteLock>
#include <QTimer>
#include <QVariantHash>
#include <QtSql/QSqlQuery>

#include <type_traits>

#include "utils/string.h"

namespace Settings
{

    template<typename T>
    struct IsQFlags : std::false_type
    {};

    template<typename T>
    struct IsQFlags<QFlags<T>> : std::true_type
    {};

    class SettingsStorage final : public QObject
    {
        Q_OBJECT

        /*! Private constructor. */
        SettingsStorage();

    public:
        /*! Virtual destructor. */
        ~SettingsStorage() final;

        static std::shared_ptr<SettingsStorage> instance();
        static void freeInstance();

        template<typename T>
        T loadValue(const QString &key, const T &defaultValue = {}) const;
        template <typename T>
        void storeValue(const QString &key, const T &value);

        void removeValue(const QString &key);
        bool hasKey(const QString &key) const;

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
    public slots:
        bool save();

    private:
        QVariant loadValueImpl(const QString &key, const QVariant &defaultValue = {}) const;
        void storeValueImpl(const QString &key, const QVariant &value);

        struct WriteSettingsData
        {
            QVariantHash updatedSettings;
            QVariantHash newSettings;
        };

        static QVariantHash readAll();
        bool writeAll() const;
        WriteSettingsData prepareWriteData() const;
        bool writeUpdated(QVariantHash &&data) const;
        bool writeNew(QVariantHash &&data) const;

        static std::shared_ptr<SettingsStorage> m_instance;

        bool m_dirty = false;
        QVariantHash m_data;
        QTimer m_saveTimer;
        mutable QReadWriteLock m_lock;
    };

    template<typename T>
    T SettingsStorage::loadValue(const QString &key, const T &defaultValue) const
    {
        // Load enum type
        if constexpr (std::is_enum_v<T>) {
            const auto value = loadValue<QString>(key);

            return Utils::String::toEnum(value, defaultValue);
        }

        // Load QFlag
        else if constexpr (IsQFlags<T>::value) {
            const typename T::Int value =
                    loadValue(key, static_cast<typename T::Int>(defaultValue));

            return T {value};
        }

        // Fast path for loading QVariant
        else if constexpr (std::is_same_v<T, QVariant>)
            return loadValueImpl(key, defaultValue);

        // Load T type
        else {
            const auto value = loadValueImpl(key);

            // Return a default value if a retrieved value is not convertible to T
            return value.template canConvert<T>() ? value.template value<T>()
                                                  : defaultValue;
        }
    }

    template<typename T>
    void SettingsStorage::storeValue(const QString &key, const T &value)
    {
        // Save enum type
        if constexpr (std::is_enum_v<T>)
            storeValueImpl(key, Utils::String::fromEnum(value));

        // Save QFlag
        else if constexpr (IsQFlags<T>::value)
            storeValueImpl(key, static_cast<typename T::Int>(value));

        // Save T type
        else
            storeValueImpl(key, value);
    }

} // namespace Settings

#endif // SETTINGS_SETTINGSSTORAGE_H
