/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_GLOBAL_IFACE_INCLUDED
#define HEADER_SQLITEPP_GLOBAL_IFACE_INCLUDED

#include "exception_iface.hpp"

namespace thinsqlitepp
{

    inline void initialize()
    {
        int res = sqlite3_initialize();
        if (res != SQLITE_OK)
            throw exception(res);
    }

    inline void shutdown() noexcept
    {
        sqlite3_shutdown();
    }

    namespace internal
    {
        template<int Code, class ...Args>
        struct config_option
        {
            static void apply(Args && ...args)
            {
                int res = sqlite3_config(Code, std::forward<Args>(args)...);
                if (res != SQLITE_OK)
                    throw exception(res);
            }
        };

        template<int Code> struct config_mapping;

    }
    
    template<int Code, class ...Args>
    inline
    auto config(Args && ...args) -> decltype(internal::config_mapping<Code>::type::apply(std::forward<decltype(args)>(args)...))
    {
        internal::config_mapping<Code>::type::apply(std::forward<Args>(args)...);
    }


    namespace internal 
    {
        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN

        #if SQLITEPP_USE_VARARG_POUND_POUND_TRICK

            //Idiotic GCC in pedantic mode warns on MACRO(arg) for MARCO(x,...) in < C++20 mode
            //with no way to disable the warning(!!!). 
            #define SQLITEPP_DEFINE_OPTION_0(code) \
                template<> struct config_mapping<code> { using type = config_option<code>; };
            #define SQLITEPP_DEFINE_OPTION_N(code, ...) \
                template<> struct config_mapping<code> { using type = config_option<code, ##__VA_ARGS__>; };

        #else

            #define SQLITEPP_DEFINE_OPTION(code, ...) \
                template<> struct config_mapping<code> { using type = config_option<code __VA_OPT__(,) __VA_ARGS__>; };

            #define SQLITEPP_DEFINE_OPTION_0(code) SQLITEPP_DEFINE_OPTION(code)
            #define SQLITEPP_DEFINE_OPTION_N(code, ...) SQLITEPP_DEFINE_OPTION(code __VA_OPT__(,) __VA_ARGS__)

        #endif

        SQLITEPP_DEFINE_OPTION_0( SQLITE_CONFIG_SINGLETHREAD          );
        SQLITEPP_DEFINE_OPTION_0( SQLITE_CONFIG_MULTITHREAD           );
        SQLITEPP_DEFINE_OPTION_0( SQLITE_CONFIG_SERIALIZED            );
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MALLOC,               sqlite3_mem_methods *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_GETMALLOC,            sqlite3_mem_methods *);
        #ifdef SQLITE_CONFIG_SMALL_MALLOC
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_SMALL_MALLOC,         int);
        #endif
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MEMSTATUS,            int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PAGECACHE,            void *, int, int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_HEAP,                 void *, int, int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MUTEX,                sqlite3_mutex_methods *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_GETMUTEX,             sqlite3_mutex_methods *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_LOOKASIDE,            int, int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PCACHE2,              sqlite3_pcache_methods2 *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_GETPCACHE2,           sqlite3_pcache_methods2 *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_LOG,                  void (*)(void*, int, const char *), void *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_URI,                  int);
        #ifdef SQLITE_CONFIG_COVERING_INDEX_SCAN
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_COVERING_INDEX_SCAN,  int);
        #endif
        #ifdef SQLITE_CONFIG_SQLLOG
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_SQLLOG,               void (*)(void *, sqlite3 *, const char *, int), void *);
        #endif
        #ifdef SQLITE_CONFIG_MMAP_SIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MMAP_SIZE,            sqlite3_int64, sqlite3_int64);
        #endif
        #ifdef SQLITE_CONFIG_WIN32_HEAPSIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_WIN32_HEAPSIZE,       int);
        #endif
        #ifdef SQLITE_CONFIG_PCACHE_HDRSZ
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PCACHE_HDRSZ,         int *);
        #endif
        #ifdef SQLITE_CONFIG_PMASZ
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PMASZ,                unsigned int);
        #endif
        #ifdef SQLITE_CONFIG_STMTJRNL_SPILL
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_STMTJRNL_SPILL,       int);
        #endif
        #ifdef SQLITE_CONFIG_SORTERREF_SIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_SORTERREF_SIZE,       int);
        #endif
        #ifdef SQLITE_CONFIG_MEMDB_MAXSIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MEMDB_MAXSIZE,        sqlite3_int64);
        #endif

        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END
        #undef SQLITEPP_DEFINE_OPTION

    }

}


#endif

