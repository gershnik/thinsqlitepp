/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_DATABASE_IFACE_INCLUDED
#define HEADER_SQLITEPP_DATABASE_IFACE_INCLUDED

#include "handle.hpp"
#include "exception_iface.hpp"
#include "mutex_iface.hpp"
#include "string_param.hpp"
#include "row_iterator.hpp"
#include "meta.hpp"

#include <memory>
#include <functional>
#include <optional>

namespace thinsqlitepp
{
    class context;

    class database final : public handle<sqlite3, database>
    {
    private:
        template<int Code, class ...Args>
        struct config_option
        {
            static void apply(database & db, Args && ...args)
            {
                int res = sqlite3_db_config(db.c_ptr(), Code, std::forward<Args>(args)...);
                if (res != SQLITE_OK)
                    throw exception(res, db);
            }
        };
        template<int Code> struct config_mapping;
        
    public:
        static std::unique_ptr<database> open(const string_param & filename, int flags, const char * vfs = nullptr);
        ~database() noexcept
            { sqlite3_close_v2(c_ptr()); }
        
        
        //MARK:- busy_handler
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> busy_handler(int (* handler)(type_identity_t<T> data, int count_invoked) noexcept, T data)
            { check_error(sqlite3_busy_handler(this->c_ptr(), (int (*)(void*,int))handler, data)); }
        
        template<class T>
        std::enable_if_t<is_pointer_to_callback<bool, T, int>,
        void> busy_handler(T handler);
        
        
        //MARK:-
        void busy_timeout(int ms)
            { check_error(sqlite3_busy_timeout(c_ptr(), ms)); }
        
        //MARK:-
        int changes() const noexcept
            { return sqlite3_changes(c_ptr()); }
        
        //MARK:- collation_needed
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> collation_needed(T data, void (* handler)(type_identity_t<T> data, database *, int encoding, const char * name) noexcept)
            { check_error(sqlite3_collation_needed(this->c_ptr(), data, (void(*)(void*,sqlite3*,int,const char*))handler)); }
        
        template<class T>
        std::enable_if_t<is_pointer_to_callback<void, T, database *, int, const char *>,
        void> collation_needed(T handler);
        
        //MARK:- commit_hook
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> commit_hook(int (* handler)(type_identity_t<T> data) noexcept, T data) noexcept
            { sqlite3_commit_hook(this->c_ptr(), (int(*)(void*))handler, data); }
        
        template<class T>
        std::enable_if_t<is_pointer_to_callback<bool, T>,
        void> commit_hook(T handler) noexcept;
        
        
        //MARK:- rollback_hook
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> rollback_hook(void (* handler)(type_identity_t<T> data) noexcept, T data) noexcept
            { sqlite3_rollback_hook(this->c_ptr(), (void(*)(void*))(handler), data); }
        
        template<class T>
        std::enable_if_t<is_pointer_to_callback<void, T>,
        void> rollback_hook(T handler) noexcept;
        
        
        //MARK:- create_collation
        
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> create_collation(const string_param & name, int encoding,
                               T collator,
                               int (* compare)(type_identity_t<T> collator, int lhs_len, const void * lhs_bytes, int rhs_len, const void * rhs_bytes) noexcept,
                               void (*deleter)(type_identity_t<T> obj) noexcept);
        
        template<class T>
        std::enable_if_t<is_pointer_to_callback<void, T, span<const std::byte>, span<const std::byte>>,
        void> create_collation(const string_param & name, int encoding, T collator, void (*deleter)(type_identity_t<T> obj) noexcept = nullptr);
        
        //MARK:- create_function
        
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> create_function(const char * name, int arg_count, int flags, T data,
                              void (*func)(context *, int, value **) noexcept,
                              void (*step)(context *, int, value **) noexcept,
                              void (*last)(context*) noexcept,
                              void (*deleter)(type_identity_t<T> obj) noexcept);
        
        
        
        template<class T>
        std::enable_if_t<std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> &&
                (
                   std::is_nothrow_invocable_r_v<void, std::remove_pointer_t<T>, context *, int, value **> ||
                   is_aggregate_function<std::remove_pointer_t<T>>
                )
            ),
        void> create_function(const char * name, int arg_count, int flags, T impl, void (*deleter)(type_identity_t<T> obj) noexcept = nullptr);
        
        //MARK:- create_window_function
#if SQLITE_VERSION_NUMBER >= 3025000
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> create_window_function(const char * name, int arg_count, int flags, T data,
                                     void (*step)(context *, int, value **) noexcept,
                                     void (*last)(context*) noexcept,
                                     void (*current)(context*) noexcept,
                                     void (*inverse)(context *, int, value **) noexcept,
                                     void (*deleter)(type_identity_t<T> obj) noexcept);
        
        
        template<class T>
        std::enable_if_t<std::is_null_pointer_v<T> ||
                (std::is_pointer_v<T> && is_aggregate_window_function<std::remove_pointer_t<T>>),
        void> create_window_function(const char * name, int arg_count, int flags, T impl, void (*deleter)(type_identity_t<T> obj) noexcept = nullptr);
#endif
        
        //MARK:-
#if SQLITE_VERSION_NUMBER >= 3010000
        void cacheflush()
        {
            if (int res = sqlite3_db_cacheflush(c_ptr()); res != SQLITE_OK)
                throw exception(res); //sic! sqlite3_db_cacheflush doesn't set DB error
        }
#endif
        
        template<int Code, class ...Args>
        auto config(Args && ...args) -> decltype(config_mapping<Code>::type::apply(*this, std::forward<decltype(args)>(args)...))
            { config_mapping<Code>::type::apply(*this, std::forward<Args>(args)...); }
        
        
        //MARK:- drop_modules
#if SQLITE_VERSION_NUMBER >= 3030000
        void drop_modules()
            { check_error(sqlite3_drop_modules(c_ptr(), nullptr)); }
                          
        void drop_modules_except(const char * const * keep)
            { check_error(sqlite3_drop_modules(c_ptr(), (const char **)keep)); }
        
        template<size_t N>
        std::enable_if_t<(N > 0),
        void> drop_modules_except(const char * const (&keep)[N])
        {
            if (keep[N-1] != nullptr) throw exception(SQLITE_MISUSE);
            check_error(sqlite3_drop_modules(this->c_ptr(), (const char **)keep));
        }
        
        template<class ...Args>
        std::enable_if_t<std::conjunction_v<std::is_convertible<Args, string_param>...>,
        void> drop_modules_except(Args && ...args)
        {
            const char * buf[] = {string_param(std::forward<Args>(args)).c_str() ..., nullptr};
            check_error(sqlite3_drop_modules(this->c_ptr(), buf));
        }
#endif
        
        //MARK:-
#ifndef SQLITE_OMIT_LOAD_EXTENSION
        void enable_load_extension(bool val)
            { check_error(sqlite3_enable_load_extension(this->c_ptr(), val)); }
#endif
        
        void extended_result_codes(bool onoff)
            { check_error(sqlite3_extended_result_codes(c_ptr(), onoff)); }
        
        //MARK:- exec
        void exec(const string_param & sql);
        
        template<class T>
        T exec(std::string_view sql, T callback);

    #if __cpp_char8_t >= 201811
        template<class T>
        T exec(std::u8string_view sql, T callback)
            {  return exec(std::string_view((const char *)sql.data(), sql.size()), callback); }
    #endif
        
        //MARK:-
        
        void file_control(const string_param & db_name, int op, void * data)
            { check_error(sqlite3_file_control(c_ptr(), db_name.c_str(), op, data)); }
        
        const char * filename(const string_param & db_name) const noexcept
        {
            auto ret = sqlite3_db_filename(c_ptr(), db_name.c_str());
            return ret ? ret : "";
        }
        
        bool get_autocommit() const noexcept
            { return sqlite3_get_autocommit(c_ptr()); }
        
        void interrupt() noexcept
            { sqlite3_interrupt(c_ptr()); }
        
        int64_t last_insert_rowid() const noexcept
            { return sqlite3_last_insert_rowid(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= 3018000
        void set_last_insert_rowid(int64_t value) noexcept
            { sqlite3_set_last_insert_rowid(c_ptr(), value); }
#endif
        
        //returns -1 on bad limit or other issues
        int limit(int id, int newVal) noexcept
            { return sqlite3_limit(c_ptr(), id, newVal); }
        
#ifndef SQLITE_OMIT_LOAD_EXTENSION
        
        void load_extension(const string_param & file, const string_param & proc);
#endif
        
        class mutex * mutex() const noexcept
            { return (class mutex *)sqlite3_db_mutex(c_ptr()); }
        
        const class statement * next_statement(const class statement * prev) const noexcept
            { return (class statement *)sqlite3_next_stmt(c_ptr(), (sqlite3_stmt *)prev); }
        
        class statement * next_statement(const class statement * prev) noexcept
            { return (class statement *)sqlite3_next_stmt(c_ptr(), (sqlite3_stmt *)prev); }
        
        void overload_function(const string_param & name, int arg_count) noexcept
            { check_error(sqlite3_overload_function(c_ptr(), name.c_str(), arg_count)); }
        
        //MARK:- progress_handler
#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
        template<class T>
        std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void> progress_handler(int step_count, int(*func)(type_identity_t<T>) noexcept, T data) const noexcept
            { sqlite3_progress_handler(this->c_ptr(), step_count, (int(*)(void*))func, data); }
        
        template<class T>
        std::enable_if_t<std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<bool, std::remove_pointer_t<T>>),
        void> progress_handler(int step_count, T func) const noexcept;
#endif
        
        //MARK: -
        std::optional<bool> readonly(const string_param & db_name) const noexcept;
        
        void release_memory() const
            { check_error(sqlite3_db_release_memory(c_ptr())); }
        
        //MARK:- status
        struct status
        {
            int current;
            int high;
        };
        struct status status(int op, bool reset = false) const;
        
        //MARK:- table_column_metadata
        
        struct column_metadata
        {
            const char * data_type;
            const char * collation_sequence;
            bool not_null;
            bool primary_key;
            bool auto_increment;
        };
        column_metadata table_column_metadata(const string_param & db_name, const string_param & table_name, const string_param & column_name) const;
        
        //MARK:-
        
        int total_changes() const noexcept
            { return sqlite3_total_changes(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= 3034000
        int txn_state(const string_param & schema) const noexcept
            { return sqlite3_txn_state(c_ptr(), schema.c_str()); }
#endif
        
        //MARK:-
        
    private:
        void check_error(int res) const
        {
            if (res != SQLITE_OK)
                throw exception(res, this);
        }
    };

    

    #if SQLITEPP_HAS_VARARG_POUND_POUND_TRICK

        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN

        #define SQLITEPP_DEFINE_DB_OPTION(code, ...) \
            template<> struct database::config_mapping<code> { using type = database::config_option<code, ##__VA_ARGS__>; };

        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END

    #elif SQLITEPP_HAS_VA_OPT

        #define SQLITEPP_DEFINE_DB_OPTION(code, ...) \
            template<> struct database::config_mapping<code> { using type = database::config_option<code __VA_OPT__(,) __VA_ARGS__>; };

    #else

        #error Neither SQLITEPP_HAS_VARARG_POUND_POUND_TRICK nor SQLITEPP_HAS_VA_OPT defined. Cannot make vararg macro

    #endif


    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_LOOKASIDE,               void *, int, int);
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_FKEY,             int, int *);
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_TRIGGER,          int, int *);
#ifdef SQLITE_DBCONFIG_ENABLE_VIEW
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_VIEW,             int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER,   int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION,   int, int *);
#endif
#ifdef SQLITE_DBCONFIG_MAINDBNAME
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_MAINDBNAME,              const char *);
#endif
#ifdef SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE,        int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_QPSG
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_QPSG,             int, int *);
#endif
#ifdef SQLITE_DBCONFIG_TRIGGER_EQP
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_TRIGGER_EQP,             int, int *);
#endif
#ifdef SQLITE_DBCONFIG_RESET_DATABASE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_RESET_DATABASE,          int, int *);
#endif
#ifdef SQLITE_DBCONFIG_DEFENSIVE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_DEFENSIVE,               int, int *);
#endif
#ifdef SQLITE_DBCONFIG_WRITABLE_SCHEMA
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_WRITABLE_SCHEMA,         int, int *);
#endif
#ifdef SQLITE_DBCONFIG_LEGACY_ALTER_TABLE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_LEGACY_ALTER_TABLE,      int, int *);
#endif

    #undef SQLITEPP_DEFINE_OPTION


}

#endif

