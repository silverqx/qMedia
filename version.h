#pragma once
#ifndef QMEDIA_VERSION_H
#define QMEDIA_VERSION_H

#define QMEDIA_VERSION_MAJOR 0
#define QMEDIA_VERSION_MINOR 1
#define QMEDIA_VERSION_BUGFIX 0

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define QMEDIA__STRINGIFY(x) #x
#define QMEDIA_STRINGIFY(x) QMEDIA__STRINGIFY(x)

#define QMEDIA_PROJECT_VERSION QMEDIA_STRINGIFY( \
    QMEDIA_VERSION_MAJOR.QMEDIA_VERSION_MINOR.QMEDIA_VERSION_BUGFIX)

/* Version Legend:
   M = Major, m = minor, p = patch, t = tweak, s = status ; [] - excluded if 0 */

// Format - M.m.p.t (used in Windows RC file)
#define QMEDIA_FILEVERSION_STR QMEDIA_STRINGIFY( \
    QMEDIA_VERSION_MAJOR.QMEDIA_VERSION_MINOR.QMEDIA_VERSION_BUGFIX.0)
// Format - M.m.p[.t]-s
#define QMEDIA_VERSION_STR QMEDIA_PROJECT_VERSION
// Format - vM.m.p[.t]-s
#define QMEDIA_VERSION_STR_2 "v" QMEDIA_PROJECT_VERSION

/*! Version number macro, can be used to check API compatibility, format - MMmmpp. */
#define QMEDIA_VERSION \
    (QMEDIA_VERSION_MAJOR * 10000 + QMEDIA_VERSION_MINOR * 100 + QMEDIA_VERSION_BUGFIX)

#endif // QMEDIA_VERSION_H
