#pragma once
#ifndef SETTINGS_SETTINGVALUE_H
#define SETTINGS_SETTINGVALUE_H

#include <QString>

#include "settings/settingsstorage.h"

namespace Settings
{

    /*! Thiny wrapper over SettingsStorage. Use it when store/load value rarely occurs,
        otherwise use CachedSettingValue. */
    template<typename T>
    class SettingValue
    {
    public:
        /*! Constructor. */
        inline explicit SettingValue(const char *keyName);

        /*! Obtain a setting value. */
        inline T get(const T &defaultValue = {}) const;
        /*! Conversion operator. */
        inline operator T() const; // NOLINT(google-explicit-constructor)
        /*! Assign a new setting value and cache it. */
        inline SettingValue<T> &operator=(const T &value);

    private:
        /*! Setting key name. */
        QString m_keyName;
        /*! Settings storage. */
        std::shared_ptr<SettingsStorage> m_settingsStorage;
    };

    /*! Cached setting value, caches a value in the constructor and always returns this
        cached value. */
    template<typename T>
    class CachedSettingValue
    {
    public:
        /*! Constructor. */
        inline explicit CachedSettingValue(
                const char *keyName, const T &defaultValue = {});
        /*! Constructor, calls a proxy function before a setting value is cached.
            The signature of the ProxyFunc must be: T proxyFunc(const T &value); */
        template<typename ProxyFunc>
        inline explicit CachedSettingValue(
                const char *keyName, const T &defaultValue, ProxyFunc &&proxyFunc);

        /*! Obtain a cached setting value. */
        inline T get() const;
        /*! Conversion operator. */
        inline operator T() const; // NOLINT(google-explicit-constructor)
        /*! Assign a new setting value and cache it. */
        inline CachedSettingValue<T> &operator=(const T &value);

    private:
        /*! Setting value wrapper. */
        SettingValue<T> m_setting;
        /*! Cached setting value. */
        T m_cache;
    };

    /* SettingValue */

    template<typename T>
    SettingValue<T>::SettingValue(const char *keyName)
        : m_keyName(QLatin1String(keyName))
        , m_settingsStorage(SettingsStorage::instance())
    {}

    template<typename T>
    T SettingValue<T>::get(const T &defaultValue) const
    {
        return m_settingsStorage->loadValue(m_keyName, defaultValue);
    }

    template<typename T>
    SettingValue<T>::operator T() const
    {
        return get();
    }

    template<typename T>
    SettingValue<T> &SettingValue<T>::operator=(const T &value)
    {
        m_settingsStorage->storeValue(m_keyName, value);

        return *this;
    }

    /* CachedSettingValue */

    template<typename T>
    CachedSettingValue<T>::CachedSettingValue(const char *keyName, const T &defaultValue)
        : m_setting(keyName)
        , m_cache(m_setting.get(defaultValue))
    {}

    template<typename T>
    template<typename ProxyFunc>
    CachedSettingValue<T>::CachedSettingValue(
            const char *keyName, const T &defaultValue, ProxyFunc &&proxyFunc
    )
        : m_setting(keyName)
        , m_cache(std::invoke(std::forward<ProxyFunc>(proxyFunc),
                              m_setting.get(defaultValue)))
    {}

    template<typename T>
    T CachedSettingValue<T>::get() const
    {
        return m_cache;
    }

    template<typename T>
    CachedSettingValue<T>::operator T() const
    {
        return get();
    }

    template<typename T>
    CachedSettingValue<T> &CachedSettingValue<T>::operator=(const T &value)
    {
        if (m_cache == value)
            return *this;

        m_setting = value;
        m_cache = value;

        return *this;
    }

} // namespace Settings

#endif // SETTINGS_SETTINGVALUE_H
