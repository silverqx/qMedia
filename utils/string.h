#pragma once
#ifndef UTILS_STRING_H
#define UTILS_STRING_H

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
    };

} // namespace Utils

#endif // UTILS_STRING_H
