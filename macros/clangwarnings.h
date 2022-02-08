#pragma once
#ifndef MACROS_CLANGWARNINGS_H
#define MACROS_CLANGWARNINGS_H

#ifndef QMEDIA_DISABLE_CLANG_WARNING_ENUM_CONVERSION
#  ifdef __clang__
#    if __has_warning("-Wdeprecated-enum-enum-conversion")
#      define QMEDIA_DISABLE_CLANG_WARNING_ENUM_CONVERSION \
         _Pragma("clang diagnostic push")                  \
         _Pragma("clang diagnostic ignored \"-Wdeprecated-enum-enum-conversion\"")
#    endif
#  endif
#endif

#ifndef QMEDIA_DISABLE_CLANG_WARNING_ENUM_CONVERSION
#  define QMEDIA_DISABLE_CLANG_WARNING_ENUM_CONVERSION
#endif

#ifndef QMEDIA_RESTORE_CLANG_WARNINGS
#  ifdef __clang__
#    define QMEDIA_RESTORE_CLANG_WARNINGS _Pragma("clang diagnostic pop")
#  else
#    define QMEDIA_RESTORE_CLANG_WARNINGS
#  endif
#endif

#endif // MACROS_CLANGWARNINGS_H
