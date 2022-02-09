#pragma once
#ifndef UTILS_STRING_H
#define UTILS_STRING_H

#include <QMetaEnum>
#include <QString>

namespace Utils
{

    /*! Miscellaneous library class. */
    class String
    {
        Q_DISABLE_COPY(String)

    public:
        /*! Deleted default constructor, this is a pure library class. */
        String() = delete;
        /*! Deleted destructor. */
        ~String() = delete;

        static QString fromDouble(double n, int precision);
        static QString wildcardToRegexPattern(const QString &pattern);

        template<typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
        static QString fromEnum(const T &value);

        template<typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
        static T toEnum(const QString &serializedValue, const T &defaultValue);
    };

    template<typename T, typename std::enable_if_t<std::is_enum_v<T>, int>>
    QString String::fromEnum(const T &value)
    {
        static_assert(std::is_same_v<int, typename std::underlying_type_t<T>>,
                      "Enumeration underlying type has to be int.");

        const auto metaEnum = QMetaEnum::fromType<T>();

        return QString::fromLatin1(metaEnum.valueToKey(static_cast<int>(value)));
    }

    template<typename T, typename std::enable_if_t<std::is_enum_v<T>, int>>
    T String::toEnum(const QString &serializedValue, const T &defaultValue)
    {
        static_assert(std::is_same_v<int, typename std::underlying_type_t<T>>,
                      "Enumeration underlying type has to be int.");

        const auto metaEnum = QMetaEnum::fromType<T>();

        bool ok = false;
        const T value = static_cast<T>(metaEnum.keyToValue(
                                           serializedValue.toLatin1().constData(), &ok));

        return ok ? value : defaultValue;
    }

} // namespace Utils

#endif // UTILS_STRING_H
