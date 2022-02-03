#include "utils/string.h"

#include <QLocale>

#include <cmath>

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

} // namespace Utils
