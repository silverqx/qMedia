#ifndef COMMON_H
#define COMMON_H

#include <QString>

// TODO In qBittorent was this comment, investigaste: Make it inline in C++17 silverqx
static const QString QB_EXT {".!qB"};
/*! Info hash size in bytes. */
Q_DECL_UNUSED static const int INFOHASH_SIZE = 40;
/*! Above this cap show ∞ symbol. */
Q_DECL_UNUSED static const qint64 MAX_ETA = 8640000;
/*! Hide zero values in the main transfer view. */
Q_DECL_UNUSED static const auto HIDE_ZERO_VALUES = true;

Q_DECL_UNUSED static const char C_NON_BREAKING_SPACE[] = " ";
Q_DECL_UNUSED static const char C_THIN_SPACE[] = " ";
Q_DECL_UNUSED static const char C_INFINITY[] = "∞";

#endif // COMMON_H
