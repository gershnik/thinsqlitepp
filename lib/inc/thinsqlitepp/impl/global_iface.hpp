/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_GLOBAL_IFACE_INCLUDED
#define HEADER_SQLITEPP_GLOBAL_IFACE_INCLUDED

#include "exception_iface.hpp"

/**
 * ThinSQLite++ namespace
 */
namespace thinsqlitepp
{
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Initialize the SQLite library
     * 
     * Equivalent to ::sqlite3_initialize
     * 
     * `#include <thinsqlitepp/global.hpp>`
     */
    inline void initialize()
    {
        int res = sqlite3_initialize();
        if (res != SQLITE_OK)
            throw exception(res);
    }

    /**
     * Deinitialize the SQLite library
     * 
     * Equivalent to ::sqlite3_shutdown
     * 
     * `#include <thinsqlitepp/global.hpp>`
     */
    inline void shutdown() noexcept
    {
        sqlite3_shutdown();
    }

    /** @cond PRIVATE */

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

    /** @endcond */
    
    /**
     * Configures SQLite library.
     * 
     * Wraps ::sqlite3_config
     * 
     * @tparam Code One of the SQLITE_CONFIG_ options. Needs to be explicitly specified
     * @tparam Args depend on the `Code` template parameter
     * 
     * `#include <thinsqlitepp/global.hpp>`
     * 
     * The following table lists required argument types for each option.
     * Supplying wrong argument types will result in compile-time error.
     * 
     * @include{doc} global-options.md
     */
    template<int Code, class ...Args>
    inline
    auto config(Args && ...args) -> 
    #ifndef DOXYGEN
        //void but prevents instantiation with wrong types 
        decltype(
          internal::config_mapping<Code>::type::apply(std::forward<decltype(args)>(args)...)
        )
    #else
        void
    #endif
        { internal::config_mapping<Code>::type::apply(std::forward<Args>(args)...); }

    /** @} */

    /** @cond PRIVATE */

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

            #define SQLITEPP_DEFINE_OPTION_N(code, ...) SQLITEPP_DEFINE_OPTION(code __VA_OPT__(,) __VA_ARGS__)
            #define SQLITEPP_DEFINE_OPTION_0(code) SQLITEPP_DEFINE_OPTION_N(code)

        #endif

        //@ [Config Options]

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
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MMAP_SIZE,            int64_t, int64_t);
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
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MEMDB_MAXSIZE,        int64_t);
        #endif

        //@ [Config Options]

        #undef SQLITEPP_DEFINE_OPTION_0
        #undef SQLITEPP_DEFINE_OPTION_N

        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END

    }
    /** @endcond */
}


#endif

