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

MAKE_MOCK(sqlite3_load_extension, (sqlite3 *db, const char * file, const char * proc, char ** err), (db, file, proc, err));
#define sqlite3_load_extension mock_sqlite3_load_extension

MAKE_MOCK(sqlite3_progress_handler, (sqlite3 *db, int step_count, int(*handler)(void*), void*data), (db, step_count, handler, data));
#define sqlite3_progress_handler mock_sqlite3_progress_handler

#endif

