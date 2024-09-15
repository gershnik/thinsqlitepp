/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_CONFIG_INCLUDED
#define HEADER_SQLITEPP_CONFIG_INCLUDED

#ifndef SQLITE_VERSION

    #include <sqlite3.h>

#endif

#define SQLITEPP_SQLITE_VERSION(x, y, z) ((x) * 1000000 + (y) * 1000 + (z))

#if SQLITE_VERSION_NUMBER < SQLITEPP_SQLITE_VERSION(3, 7, 15)

    #error This library requires SQLite 3.7.15 or greater

#endif

#if __cplusplus <= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG <= 201703L)
    
    #define SQLITEPP_USE_VARARG_POUND_POUND_TRICK 1

#endif

#ifdef __clang__
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END _Pragma("GCC diagnostic pop")
#else
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END
#endif

#if !defined(__clang__) && defined(__GNUC__)
    #define SQLITEPP_NO_PEDANTIC __extension__
#endif

#ifndef DOXYGEN
    #define SQLITEPP_ENABLE_IF(cond, t) std::enable_if_t<(cond), t>
#else
    #define SQLITEPP_ENABLE_IF(cond, t) t
#endif

#endif

