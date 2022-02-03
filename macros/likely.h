#pragma once
#ifndef MACROS_LIKELY_HPP
#define MACROS_LIKELY_HPP

#ifndef __has_cpp_attribute
#  define T_LIKELY
#elif __has_cpp_attribute(likely) >= 201803L
#  define T_LIKELY [[likely]]
#else
#  define T_LIKELY
#endif

#ifndef __has_cpp_attribute
#  define T_UNLIKELY
#elif __has_cpp_attribute(unlikely) >= 201803L
#  define T_UNLIKELY [[unlikely]]
#else
#  define T_UNLIKELY
#endif

#endif // MACROS_LIKELY_HPP
