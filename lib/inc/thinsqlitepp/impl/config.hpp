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

#if SQLITE_VERSION_NUMBER < 3007015

#error This library requires SQLite 3.7.4 or greater

#endif

#if __cplusplus <= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG <= 201703L)
    
    #define SQLITEPP_HAS_VARARG_POUND_POUND_TRICK 1

    #ifdef __clang__
        #define SQLITEPP_SUPPRESS_POUND_POUND_WARNING_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")
        #define SQLITEPP_SUPPRESS_POUND_POUND_WARNING_END _Pragma("GCC diagnostic pop")
    #else
        #define SQLITEPP_SUPPRESS_POUND_POUND_WARNING_BEGIN
        #define SQLITEPP_SUPPRESS_POUND_POUND_WARNING_END
    #endif

#else

    #define SQLITEPP_HAS_VA_OPT 1
    
#endif


#endif

