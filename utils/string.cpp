#include "utils/string.h"

#include <QLocale>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

#include <cmath>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
/* This is marked as internal in QRegExp.cpp, but is exported. The alternative would be to
   copy the code from QRegExp::wc2rx(). */
QString qt_regexp_toCanonical(const QString &pattern, QRegExp::PatternSyntax patternSyntax);
#endif

namespace Utils
{

QString Utils::String::fromDouble(const double n, const int precision)
{
    /* HACK because QString rounds up. Eg QString::number(0.999*100.0, 'f' ,1) == 99.9
       but QString::number(0.9999*100.0, 'f' ,1) == 100.0 The problem manifests when
       the number has more digits after the decimal than we want AND the digit after
       our 'wanted' is >= 5. In this case our last digit gets rounded up. So for each
       precision we add an extra 0 behind 1 in the below algorithm. */

    const auto prec = std::pow(10.0, precision);

    return QLocale::system().toString(std::floor(n * prec) / prec, 'f', precision);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QString String::wildcardToRegexPattern(const QString &pattern)
{
    return QRegularExpression::wildcardToRegularExpression(
                pattern, QRegularExpression::UnanchoredWildcardConversion);
}
#else
QString String::wildcardToRegexPattern(const QString &pattern)
{
    return ::qt_regexp_toCanonical(pattern, QRegExp::Wildcard);
}
#endif

} // namespace Utils
