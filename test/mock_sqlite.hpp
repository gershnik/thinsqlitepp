#ifndef HEADER_MOCK_SQLITE_INCLUDED
#define HEADER_MOCK_SQLITE_INCLUDED

#include <sqlite3.h>

#include <map>
#include <any>
#include <functional>


extern std::map<void *, std::any> g_mock_map;

inline void clear_mocks()
{
    g_mock_map.clear();
}

struct mock_cleanup
{
    mock_cleanup() = default;
    mock_cleanup(const mock_cleanup &) = delete;
    void operator=(mock_cleanup) = delete;
    
    ~mock_cleanup()
        { clear_mocks(); }
};

#define MAKE_MOCK(name, args_decl, args) \
    auto * const real_##name = name; \
    \
    inline std::function<decltype(name)> get_mock_##name() { \
        auto it = g_mock_map.find((void*)name); \
        if (it == g_mock_map.end()) \
            return name; \
        return std::any_cast<std::function<decltype(name)>>(it->second); \
    } \
    \
    inline void set_mock_##name(std::function<decltype(name)> val) { \
        g_mock_map[(void*)name] = std::move(val); \
    }\
    inline void clear_mock_##name() { \
        g_mock_map.erase((void*)name); \
    }\
    \
    inline auto mock_##name args_decl { \
        return get_mock_##name() args; \
    }


MAKE_MOCK(sqlite3_open_v2, (const char *filename, sqlite3 **ppDb, int flags, const char *zVfs), (filename, ppDb, flags, zVfs))
#define sqlite3_open_v2 mock_sqlite3_open_v2

MAKE_MOCK(sqlite3_close_v2, (sqlite3 *db), (db))
#define sqlite3_close_v2 mock_sqlite3_close_v2

MAKE_MOCK(sqlite3_busy_handler, (sqlite3 *db, int(*handler)(void*,int), void *data), (db, handler, data));
#define sqlite3_busy_handler mock_sqlite3_busy_handler

MAKE_MOCK(sqlite3_busy_timeout, (sqlite3 *db, int ms), (db, ms));
#define sqlite3_busy_timeout mock_sqlite3_busy_timeout

MAKE_MOCK(sqlite3_changes, (sqlite3 *db), (db));
#define sqlite3_changes mock_sqlite3_changes

MAKE_MOCK(sqlite3_commit_hook, (sqlite3 *db, int(*handler)(void*), void *data), (db, handler, data));
#define sqlite3_commit_hook mock_sqlite3_commit_hook

MAKE_MOCK(sqlite3_rollback_hook, (sqlite3 *db, void(*handler)(void*), void *data), (db, handler, data));
#define sqlite3_rollback_hook mock_sqlite3_rollback_hook

MAKE_MOCK(sqlite3_update_hook, (sqlite3 *db,
                                void(*handler)(void *,int,char const *,char const *,sqlite3_int64),
                                void * data), (db, handler, data));
#define sqlite3_update_hook mock_sqlite3_update_hook

#if SQLITE_VERSION_NUMBER >= 3016000 && defined(SQLITE_ENABLE_PREUPDATE_HOOK)
MAKE_MOCK(sqlite3_preupdate_hook, (sqlite3 *db,
                                void(*handler)(void *,sqlite3 *,int,char const *,char const *,sqlite3_int64,sqlite3_int64),
                                void * data), (db, handler, data));
#define sqlite3_preupdate_hook mock_sqlite3_preupdate_hook
#endif

MAKE_MOCK(sqlite3_wal_hook, (sqlite3 *db,
                             int(*handler)(void *,sqlite3 *,char const *,int),
                             void * data), (db, handler, data));

MAKE_MOCK(sqlite3_create_collation_v2, (sqlite3 *db, const char * name, int flags,
                                        void * arg,
                                        int(*compare)(void*,int,const void*,int,const void*),
                                        void(*destroy)(void*)), (db, name, flags, arg, compare, destroy));
#define sqlite3_create_collation_v2 mock_sqlite3_create_collation_v2

MAKE_MOCK(sqlite3_create_function_v2, (sqlite3 *db, const char * name, int count, int flags,
                                       void * arg,
                                       void (*func)(sqlite3_context*,int,sqlite3_value**),
                                       void (*step)(sqlite3_context*,int,sqlite3_value**),
                                       void (*done)(sqlite3_context*),
                                       void(*destroy)(void*)), (db, name, count, flags, arg, func, step, done, destroy));
#define sqlite3_create_function_v2 mock_sqlite3_create_function_v2

#if SQLITE_VERSION_NUMBER >= 3025000

MAKE_MOCK(sqlite3_create_window_function, (sqlite3 *db, const char * name, int count, int flags,
                                           void * arg,
                                           void (*step)(sqlite3_context*,int,sqlite3_value**),
                                           void (*done)(sqlite3_context*),
                                           void (*value)(sqlite3_context*),
                                           void (*inverse)(sqlite3_context*,int,sqlite3_value**),
                                           void(*destroy)(void*)), (db, name, count, flags, arg, step, done, value, inverse, destroy));
#define sqlite3_create_window_function mock_sqlite3_create_window_function

#endif

#if SQLITE_VERSION_NUMBER >= 3030000

MAKE_MOCK(sqlite3_drop_modules, (sqlite3 *db, const char ** keep), (db, keep));
#define sqlite3_drop_modules mock_sqlite3_drop_modules

#endif

#if ! THINSQLITEPP_OMIT_LOAD_EXTENSION

MAKE_MOCK(sqlite3_load_extension, (sqlite3 *db, const char * file, const char * proc, char ** err), (db, file, proc, err));
#define sqlite3_load_extension mock_sqlite3_load_extension

#endif

MAKE_MOCK(sqlite3_progress_handler, (sqlite3 *db, int step_count, int(*handler)(void*), void*data), (db, step_count, handler, data));
#define sqlite3_progress_handler mock_sqlite3_progress_handler

MAKE_MOCK(sqlite3_free, (void * ptr), (ptr));
#define sqlite3_free mock_sqlite3_free

MAKE_MOCK(sqlite3_malloc, (int size), (size));
#define sqlite3_malloc mock_sqlite3_malloc

#if SQLITE_VERSION_NUMBER >= 3008007
MAKE_MOCK(sqlite3_malloc64, (sqlite3_uint64 size), (size));
#define sqlite3_malloc64 mock_sqlite3_malloc64
#endif

MAKE_MOCK(sqlite3_extended_result_codes, (sqlite3 *db, int onoff), (db, onoff));
#define sqlite3_extended_result_codes mock_sqlite3_extended_result_codes

MAKE_MOCK(sqlite3_file_control, (sqlite3 *db, const char *zDbName, int op, void *pArg), (db, zDbName, op, pArg));
#define sqlite3_file_control mock_sqlite3_file_control

MAKE_MOCK(sqlite3_interrupt, (sqlite3 *db), (db));
#define sqlite3_interrupt mock_sqlite3_interrupt

MAKE_MOCK(sqlite3_overload_function, (sqlite3 *db, const char *zName, int nArg), (db, zName, nArg));
#define sqlite3_overload_function mock_sqlite3_overload_function

MAKE_MOCK(sqlite3_db_release_memory, (sqlite3 *db), (db));
#define sqlite3_db_release_memory mock_sqlite3_db_release_memory

#if SQLITE_VERSION_NUMBER >= 3010000
MAKE_MOCK(sqlite3_db_cacheflush, (sqlite3 *db), (db));
#define sqlite3_db_cacheflush mock_sqlite3_db_cacheflush
#endif

#if SQLITE_VERSION_NUMBER >= 3050000
MAKE_MOCK(sqlite3_setlk_timeout, (sqlite3 *db, int ms, int flags), (db, ms, flags));
#define sqlite3_setlk_timeout mock_sqlite3_setlk_timeout
#endif

#if !THINSQLITEPP_OMIT_SNAPSHOT && SQLITE_VERSION_NUMBER >= 3010000
MAKE_MOCK(sqlite3_snapshot_recover, (sqlite3 *db, const char *zDb), (db, zDb));
#define sqlite3_snapshot_recover mock_sqlite3_snapshot_recover
#endif

#endif

