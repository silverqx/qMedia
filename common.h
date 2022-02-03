#ifndef COMMON_H
#define COMMON_H

#include <QString>

inline const QString QB_EXT {".!qB"};
/*! Info hash size in bytes. */
inline const int INFOHASH_SIZE = 40;
/*! Above this cap show ∞ symbol. */
inline const qint64 MAX_ETA = 8640000;
/*! Hide zero values in the main transfer view. */
inline const auto HIDE_ZERO_VALUES = true;

inline const QChar C_NON_BREAKING_SPACE = ' ';
inline const QChar C_THIN_SPACE         = L' ';
inline const QChar C_INFINITY           = L'∞';

#endif // COMMON_H
