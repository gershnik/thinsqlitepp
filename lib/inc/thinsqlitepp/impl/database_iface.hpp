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
#include "blob_iface.hpp"
#include "snapshot_iface.hpp"
#include "memory_iface.hpp"
#include "string_param.hpp"
#include "span.hpp"
#include "meta.hpp"

#include <memory>
#include <functional>
#include <optional>

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
    
    #if defined(__APPLE__) && defined(__clang__) && defined(SQLITE_AVAILABLE)
        #pragma GCC diagnostic ignored "-Wunguarded-availability-new"
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif



namespace thinsqlitepp
{
    class context;
    class row;
    class value;

    /** @cond PRIVATE */

    struct database_detector
    {
        SQLITEPP_CALL_DETECTOR(enable_load_extension, int, sqlite3_enable_load_extension, (T *)nullptr, int{});
        SQLITEPP_CALL_DETECTOR(load_extension, int, sqlite3_load_extension, (T *)nullptr, 
                                                                            (const char *)nullptr,
                                                                            (const char *)nullptr,
                                                                            (char **)nullptr);

        SQLITEPP_METHOD_DETECTOR(void, step, (context *)nullptr, int{}, (value **)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, last, (context *)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, inverse, (context *)nullptr, int{}, (value **)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, current, (context *)nullptr);

    public:
        template<class T>
        static constexpr bool is_function = std::is_nothrow_invocable_r_v<void, T, context *, int, value **>;

        template<class T>
        static constexpr bool is_aggregate_function = has_noexcept_step<T> && has_noexcept_last<T>;

        template<class T>
        static constexpr bool is_aggregate_window_function = is_aggregate_function<T> && has_noexcept_inverse<T> && has_noexcept_current<T>;
        
        template<class R, class T, class... ArgTypes>
        static constexpr bool is_pointer_to_callback =  std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<R, std::remove_pointer_t<T>, ArgTypes...>);

        template<class R, class T, class... ArgTypes>
        static constexpr bool is_pointer_to_throwing_callback =  std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_invocable_r_v<R, std::remove_pointer_t<T>, ArgTypes...>);

        template<class T>
        static constexpr bool is_pointer_to_function = std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> &&
                (
                   is_function<std::remove_pointer_t<T>> ||
                   is_aggregate_function<std::remove_pointer_t<T>>
                )
            );

        template<class T>
        static constexpr bool is_pointer_to_window_function = std::is_null_pointer_v<T> ||
                (std::is_pointer_v<T> && is_aggregate_window_function<std::remove_pointer_t<T>>);
    };

    /** @endcond */

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Database Connection
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3.
     * 
     * `#include <thinsqlitepp/database.hpp>`
     * 
     */
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

        template<int Code, class ...Args>
        struct vtab_config_option
        {
            static void apply(database & db, Args && ...args)
            {
                int res = sqlite3_vtab_config(db.c_ptr(), Code, std::forward<Args>(args)...);
                if (res != SQLITE_OK)
                    throw exception(res, db);
            }
        };
        template<int Code> struct vtab_config_mapping;
        
    public:
        /**
         * Open a new database connection
         * 
         * Equivalent to ::sqlite3_open_v2
         */
        static std::unique_ptr<database> open(const string_param & db_filename, int flags, const char * vfs = nullptr);

        /// Equivalent to ::sqlite3_close_v2
        ~database() noexcept
            { sqlite3_close_v2(c_ptr()); }
        
        
        //MARK: -

        /**
         * Set a busy timeout
         * 
         * Equivalent to ::sqlite3_busy_timeout
         */
        void busy_timeout(int ms)
            { check_error(sqlite3_busy_timeout(c_ptr(), ms)); }
        
        //MARK: -

        /**
         * Count of the number of rows modified
         * 
         * Equivalent to ::sqlite3_changes
         */
        int64_t changes() const noexcept
        {
        #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 36, 1)
            return sqlite3_changes64(c_ptr()); 
        #else
            return sqlite3_changes(c_ptr()); 
        #endif
        }

        //MARK: - exec

        /** @{
         * @anchor database_exec
         * @name Executing queries
         */

        /**
         * Run multiple statements of SQL
         * 
         * Unlike other functions in this library this one **DOES NOT** delegate to
         * ::sqlite3_exec but instead implements equivalent functionality directly.
         * 
         * It runs zero or more UTF-8 encoded, semicolon-separate SQL statements passed
         * as the `sql` argument. If an error occurs while evaluating the SQL statements
         * then execution of the current statement stops and subsequent statements are skipped.
         * 
         * As usual the error will be reported via an @ref exception
         * 
         * @param sql Statements to execute
         */
        void exec(std::string_view sql);

    #if __cpp_char8_t >= 201811
        /// @overload
        template<class T>
        T exec(std::u8string_view sql)
            {  return exec(std::string_view((const char *)sql.data(), sql.size())); }
    #endif
        
        /**
         * Run multiple statements of SQL with a callback
         * 
         * Unlike other functions in this library this one **DOES NOT** delegate to
         * ::sqlite3_exec but instead implements equivalent functionality directly.
         * 
         * It runs zero or more UTF-8 encoded, semicolon-separate SQL statements passed
         * as the `sql` argument. The `callback` callable is passed by value and
         * is invoked for each result row coming out of the evaluated SQL statements. 
         * The callable can have one of the 4 possible variants:
         * ```cpp
         * 1. bool callback(int statement_idx, row current_row)
         * 2. void callback(int statement_idx, row current_row)
         * 3. bool callback(row current_row)
         * 4. void callback(row current_row)
         * ```
         * 
         * If more than one way of calling the callback is possible the way it will
         * be invoked is chosen in the order given above.
         * 
         * For variants 1 and 3 if an invocation of callback returns `false` then 
         * the execution of the current statement stops and subsequent statements are skipped.
         * 
         * For variants 1 and 2 the `statement_idx` is the index of the SQL statement 
         * being executed. If you only pass a single statement to `exec()` you 
         * generally don't need these variants.
         * 
         * The @p callback argument is returned back from the function which allows it to 
         * accumulate state.
         * 
         * If an error occurs while evaluating the SQL statements
         * then execution of the current statement stops and subsequent statements are skipped.
         * 
         * As usual the error will be reported via an @ref exception
         * 
         * @param sql Statements to execute
         * @param callback Callback to execute for each row of the results
         * @returns the `callback` argument
         */
        template<class T>
        SQLITEPP_ENABLE_IF((
            std::is_invocable_r_v<bool, T, int, row> ||
            std::is_invocable_r_v<void, T, int, row> ||
            std::is_invocable_r_v<bool, T, row> ||
            std::is_invocable_r_v<void, T, row>),
        T) exec(std::string_view sql, T callback);

    #if __cpp_char8_t >= 201811
        /// @overload
        template<class T>
        T exec(std::u8string_view sql, T callback)
            {  return exec(std::string_view((const char *)sql.data(), sql.size()), callback); }
    #endif

        /// @}

        /** @{
         * @anchor database_callbacks
         * @name Callbacks and notifications
         */

        //MARK: - busy_handler

        /**
         * Register a callback to handle #SQLITE_BUSY errors
         * 
         * Equivalent to ::sqlite3_busy_handler
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) busy_handler(int (* handler)(type_identity_t<T> data_ptr, int count_invoked) noexcept, T data_ptr)
            { check_error(sqlite3_busy_handler(this->c_ptr(), (int (*)(void*,int))handler, data_ptr)); }
        
        /**
         * Register a callback to handle #SQLITE_BUSY errors
         * 
         * Equivalent to ::sqlite3_busy_handler
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * int count_invoked = ...;
         * bool result = (*handler_ptr)(count_invoked);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<bool, T, int>),
        void) busy_handler(T handler_ptr);
        
        
        //MARK: - collation_needed

        /**
         * Register a callback to be called when undefined collation sequence is required
         * 
         * Equivalent to ::sqlite3_collation_needed
         * 
         * @param data_ptr A pointer to callback data or nullptr.
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) collation_needed(T data_ptr, void (* handler)(type_identity_t<T> data_ptr, database *, int encoding, const char * name) noexcept)
            { check_error(sqlite3_collation_needed(this->c_ptr(), data_ptr, (void(*)(void*,sqlite3*,int,const char*))handler)); }
        
        /**
         * Register a callback to be called when undefined collation sequence is required
         * 
         * Equivalent to ::sqlite3_collation_needed
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * database * db = ...;
         * int encoding = <one of SQLITE_UTF8, SQLITE_UTF16BE, or SQLITE_UTF16LE>
         * const char * name = ...;
         * (*handler_ptr)(db, encoding, name);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, database *, int, const char *>),
        void) collation_needed(T handler_ptr);
        
        //MARK: - commit_hook

        /**
         * Register a callback to be called on commit
         * 
         * Equivalent to ::sqlite3_commit_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) commit_hook(int (* handler)(type_identity_t<T> data_ptr) noexcept, T data_ptr) noexcept
            { sqlite3_commit_hook(this->c_ptr(), (int(*)(void*))handler, data_ptr); }
        
        /**
         * Register a callback to be called on commit
         * 
         * Equivalent to ::sqlite3_commit_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * bool result = (*handler_ptr)();
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<bool, T>),
        void) commit_hook(T handler_ptr) noexcept;
        
        
        //MARK: - rollback_hook

        /**
         * Register a callback to be called on rollback
         * 
         * Equivalent to ::sqlite3_rollback_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) rollback_hook(void (* handler)(type_identity_t<T> data_ptr) noexcept, T data_ptr) noexcept
            { sqlite3_rollback_hook(this->c_ptr(), (void(*)(void*))(handler), data_ptr); }
        
        /**
         * Register a callback to be called on rollback
         * 
         * Equivalent to ::sqlite3_rollback_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)();
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T>),
        void) rollback_hook(T handler_ptr) noexcept;


        //MARK: - update_hook

        /**
         * Register a callback to be called whenever a row is updated, inserted or deleted in a rowid table.
         * 
         * Equivalent to ::sqlite3_update_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) update_hook(void (* handler)(type_identity_t<T> data_ptr, int op, const char * db_name, const char * table, sqlite3_int64 rowid) noexcept, 
                          T data_ptr) noexcept
            { sqlite3_update_hook(this->c_ptr(), (void(*)(void*,int,char const *,char const *,sqlite3_int64))(handler), data_ptr); }


        /**
         * Register a callback to be called whenever a row is updated, inserted or deleted in a rowid table.
         * 
         * Equivalent to ::sqlite3_update_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)(int op, const char * db_name, const char * table, int64_t rowid);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, int, const char *, const char *, int64_t>),
        void) update_hook(T handler_ptr) noexcept;

        //MARK: - preupdate_hook

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 16, 0) && defined(SQLITE_ENABLE_PREUPDATE_HOOK)
        /**
         * Register a callback to be called prior to each INSERT, UPDATE, and DELETE operation on a database table.
         * 
         * Equivalent to ::sqlite3_preupdate_hook
         * 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         * 
         * @since SQLite 3.16
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) preupdate_hook(void (* handler)(type_identity_t<T> data_ptr, 
                                              database * db, 
                                              int op, 
                                              const char * db_name, 
                                              const char * table, 
                                              sqlite3_int64 rowid_old,
                                              sqlite3_int64 rowid_new) noexcept, 
                             T data_ptr) noexcept
        { 
            sqlite3_preupdate_hook(this->c_ptr(), (void(*)(void*,sqlite3 *,int,char const *,char const *,sqlite3_int64,sqlite3_int64))(handler), 
                                    data_ptr); 
        }

        /**
         * Register a callback to be called prior to each INSERT, UPDATE, and DELETE operation on a database table.
         * 
         * Equivalent to ::sqlite3_preupdate_hook
         * 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)(database * db, int op, const char * db_name, const char * table, int64_t rowid_old, int64_t rowid_new);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         * 
         * @since SQLite 3.16
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, database *, int, const char *, const char *, int64_t, int64_t>),
        void) preupdate_hook(T handler_ptr) noexcept;

    #endif

        /**
         * Register a callback to be called each time data is committed to a database in wal mode.
         * 
         * Equivalent to ::sqlite3_wal_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) wal_hook(int (* handler)(type_identity_t<T> data_ptr, database * db, const char * db_name, int num_pages) noexcept, 
                       T data_ptr) noexcept
            { sqlite3_wal_hook(this->c_ptr(), (int(*)(void *,sqlite3*,const char*,int))(handler), data_ptr); }


        /**
         * Register a callback to be called each time data is committed to a database in wal mode.
         * 
         * Equivalent to ::sqlite3_wal_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)(const char * db_name, int num_pages);
         * ```
         * This invocation can throw exceptions. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_throwing_callback<void, T, database *, const char *, int>),
        void) wal_hook(T handler_ptr) noexcept;

        /// @}

        //MARK: -

    #if defined(SQLITE_ENABLE_PREUPDATE_HOOK)
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 16, 0)

        /** @{
         * @name Preupdate hook helpers
         */

        /**
         * Returns value of a column of the table row before it is updated.
         * 
         * Equivalent to ::sqlite3_preupdate_old
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.16
         */
        value * preupdate_old(int column_idx);

        /**
         * Returns value of a column of the table row after it is updated.
         * 
         * Equivalent to ::sqlite3_preupdate_new
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.16
         */
        value * preupdate_new(int column_idx);

        /**
         * Returns the number of columns in the row that is being inserted, updated, or deleted.
         * 
         * Equivalent to ::sqlite3_preupdate_count
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.16
         */
        int preupdate_count() const noexcept
            { return sqlite3_preupdate_count(c_ptr()); }

        /**
         * Returns the "depth" of an update from the top level SQL
         * 
         * Equivalent to ::sqlite3_preupdate_depth
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @returns 0 if the preupdate callback was invoked as a result of a direct 
         * insert, update, or delete operation; or 1 for inserts, updates, or deletes 
         * invoked by top-level triggers; or 2 for changes resulting from triggers 
         * called by top-level triggers; and so forth.
         * 
         * @since SQLite 3.16
         */
        int preupdate_depth() const noexcept
            { return sqlite3_preupdate_depth(c_ptr()); }


    #endif
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 36, 0)
        /**
         * Returns the index of the column being written via ::sqlite3_blob_write.
         * 
         * Equivalent to ::sqlite3_preupdate_blobwrite
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.36
         */    
        int preupdate_blobwrite() const noexcept
            { return sqlite3_preupdate_blobwrite(c_ptr()); }


    #endif
    #endif
        

        /** @} */

        //MARK: -

        /** @{
         * @name WAL checkpoint control
         */

        /**
         * Checkpoint a database
         * 
         * Equivalent to ::sqlite3_wal_checkpoint_v2
         * 
         * @param db_name Name of attached database (or nullptr)
         * @param mode One of SQLITE_CHECKPOINT_ values
         * @returns A pair of {Size of WAL log in frames, Total number of frames checkpointed} or {-1, -1}
         * if the database is not in WAL mode
         */
        std::pair<int, int> checkpoint(const string_param & db_name, int mode = SQLITE_CHECKPOINT_PASSIVE);

        /**
         * Configure an auto-checkpoint
         * 
         * Equivalent to ::sqlite3_wal_autocheckpoint
         */
        void autocheckpoint(int num_frames)
            { check_error(sqlite3_wal_autocheckpoint(c_ptr(), num_frames)); }

        /* @} */


        
        //MARK: - create_collation

        /** @{
         * @anchor database_create_collation
         * @name Defining collating sequences
         */
        
        /**
         * Define a new collating sequence
         * 
         * Equivalent to ::sqlite3_create_collation_v2
         * 
         * @param name Collation name
         * @param encoding One of [SQLite text encodings](https://www.sqlite.org/c3ref/c_any.html)
         * @param collator_ptr A pointer to a collator or nullptr.
         * @param compare A collating function that matches the type of @p collator_ptr argument. Can be
         *  nullptr.
         * @param destructor A "destructor" function for the @p collator_ptr argument. Can be
         *  nullptr.
         * 
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) create_collation(const string_param & name, int encoding,
                               T collator_ptr,
                               int (*compare)(type_identity_t<T> collator, int lhs_len, const void * lhs_bytes, int rhs_len, const void * rhs_bytes) noexcept,
                               void (*destructor)(type_identity_t<T> collator) noexcept);
        
        /**
         * Define a new collating sequence
         * 
         * Equivalent to ::sqlite3_create_collation_v2
         * 
         * @param name Collation name
         * @param encoding One of [SQLite text encodings](https://www.sqlite.org/c3ref/c_any.html)
         * @param collator_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * span<const std::byte> lhs = ...;
         * span<const std::byte> rhs = ...;
         * int res = (*collator_ptr)(lhs, rhs);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the collator.
         * @param destructor A "destructor" function for the @p collator_ptr argument. Can be
         * nullptr. Unlike the ::sqlite3_create_collation_v2 the @p destructor is always called
         * even if this function throws an exception.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<int, T, span<const std::byte>, span<const std::byte>>),
        void) create_collation(const string_param & name, int encoding, T collator_ptr,
                              void (*destructor)(type_identity_t<T> obj) noexcept = nullptr);

        /// @}
        
        //MARK: - create_function

        /** @{
         * @anchor database_create_function
         * @name Creating or redefining SQL functions
         */
        
        /**
         * Create or redefine SQL function
         * 
         * Equivalent to ::sqlite3_create_function_v2
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL function takes. 
         * If this parameter is -1, then the SQL function may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param data_ptr A pointer to callback data or nullptr. The implementation of the function 
         * can gain access to this pointer using context::user_data().
         * @param func Function callback. See ::sqlite3_create_function_v2
         * @param step Step callback. See ::sqlite3_create_function_v2
         * @param last Last callback. See ::sqlite3_create_function_v2
         * @param destructor A "destructor" function for @p data_ptr. Can be nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) create_function(const char * name, int arg_count, int flags, T data_ptr,
                              void (*func)(context *, int, value **) noexcept,
                              void (*step)(context *, int, value **) noexcept,
                              void (*last)(context*) noexcept,
                              void (*destructor)(type_identity_t<T> data_ptr) noexcept);
        
        
        
        /**
         * Create or redefine SQL function
         * 
         * Equivalent to ::sqlite3_create_function_v2
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL function takes. 
         * If this parameter is -1, then the SQL function may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param impl_ptr A **pointer** to C++ object that implements the function or nullptr. 
         * The C++ object pointed to it needs to:
         * - For scalar SQL function **only** be callable as:
         *   ```
         *   (*impl_ptr)(context *, int, value **);
         *   ```
         * - For aggregate SQL function **only** be callable as:
         *   ```
         *   impl_ptr->step(context *, int, value **);
         *   impl_ptr->last(context *);
         *   ```
         * If impl_ptr is nullptr the function is removed.
         * @param destructor A "destructor" function for @p impl_ptr. Can be nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(database_detector::is_pointer_to_function<T>,
        void) create_function(const char * name, int arg_count, int flags, 
                              T impl_ptr, void (*destructor)(type_identity_t<T> obj) noexcept = nullptr);
        
        //MARK: - create_window_function

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 25, 0)

        /**
         * Create or redefine SQL [aggregate window function](https://www.sqlite.org/windowfunctions.html#aggwinfunc)
         * 
         * Equivalent to ::sqlite3_create_window_function
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL aggregate takes. 
         * If this parameter is -1, then the SQL aggregate may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param data_ptr A pointer to callback data or nullptr. The implementation of the function 
         * can gain access to this pointer using context::user_data().
         * @param step Step callback. See ::sqlite3_create_window_function
         * @param last Last callback. See ::sqlite3_create_window_function
         * @param current Current callback. See ::sqlite3_create_window_function
         * @param inverse Inverse callback. See ::sqlite3_create_window_function
         * @param destructor A "destructor" function for @p data_ptr. Can be nullptr.
         * 
         * @since SQLite 3.25
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) create_window_function(const char * name, int arg_count, int flags, T data_ptr,
                                     void (*step)(context *, int, value **) noexcept,
                                     void (*last)(context*) noexcept,
                                     void (*current)(context*) noexcept,
                                     void (*inverse)(context *, int, value **) noexcept,
                                     void (*destructor)(type_identity_t<T> data_ptr) noexcept);
        
        
        /**
         * Create or redefine SQL [aggregate window function](https://www.sqlite.org/windowfunctions.html#aggwinfunc)
         * 
         * Equivalent to ::sqlite3_create_window_function
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL function takes. 
         * If this parameter is -1, then the SQL function may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param impl_ptr A **pointer** to C++ object that implements the function or nullptr. 
         * The C++ object pointed to it needs to be callable as:
         *   ```
         *   impl_ptr->step(context *, int, value **);
         *   impl_ptr->last(context *);
         *   impl_ptr->current(context *);
         *   impl_ptr->inverse(context *, int, value **);
         *   ```
         * If impl_ptr is nullptr the function is removed.
         * @param destructor A "destructor" function for @p impl_ptr. Can be nullptr.
         * 
         * @since SQLite 3.25
         */
        template<class T>
        SQLITEPP_ENABLE_IF(database_detector::is_pointer_to_window_function<T>,
        void) create_window_function(const char * name, int arg_count, int flags, 
                                     T impl_ptr, void (*destructor)(type_identity_t<T> obj) noexcept = nullptr);
#endif

        ///@}
        
        //MARK: -
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) 
        /**
         * Flush caches to disk mid-transaction
         * 
         * Equivalent to ::sqlite3_db_cacheflush
         * 
         * @since SQLite 3.1
         */
        void cacheflush()
        {
            if (int res = sqlite3_db_cacheflush(c_ptr()); res != SQLITE_OK)
                throw exception(res); //sic! sqlite3_db_cacheflush doesn't set DB error
        }
#endif
        /**
         * Configure database connection
         * 
         * Wraps ::sqlite3_db_config
         * 
         * @tparam Code One of the SQLITE_DBCONFIG_ options. Needs to be explicitly specified
         * @tparam Args depend on the @p Code template parameter
         * 
         * The following table lists required argument types for each option.
         * Supplying wrong argument types will result in compile-time error.
         * 
         * @include{doc} db-options.md
         * 
         */
        template<int Code, class ...Args>
        auto config(Args && ...args) -> 
        #ifndef DOXYGEN
            //void but prevents instantiation with wrong types
            decltype(
              config_mapping<Code>::type::apply(*this, std::forward<decltype(args)>(args)...)
            )
        #else
            void
        #endif
            { config_mapping<Code>::type::apply(*this, std::forward<Args>(args)...); }
        

        //MARK: - create_module

        /** @{
         * @anchor modules
         * @name Virtual Table Modules
         */

        /**
         * Register a virtual table implementation
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * @param name name of the module
         * @param mod pointer to ::sqlite3_module "vtable"
         */
        void create_module(const string_param & name, const sqlite3_module * mod)
            { check_error(sqlite3_create_module_v2(c_ptr(), name.c_str(), mod, nullptr, nullptr)); }

        /**
         * Register a virtual table implementation
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * @param name name of the module
         * @param mod pointer to ::sqlite3_module "vtable"
         * @param data data to be passed to virtual table xCreate function.
         * @param destructor function to call when data is no longer needed. Can be omitted
         */
        template<typename T>
        void create_module(const string_param & name, const sqlite3_module * mod, 
                           T * data, void(*destructor)(T *) = nullptr)
            { check_error(sqlite3_create_module_v2(c_ptr(), name.c_str(), mod, (void*)data, (void (*)(void *))destructor)); }

        
        //MARK: -
        
        /**
         * Declare the schema of a virtual table
         * 
         * Equivalent to ::sqlite3_declare_vtab
         */
        void declare_vtab(const string_param & sdl)
            { check_error(sqlite3_declare_vtab(c_ptr(), sdl.c_str())); }


        /**
         * Configure virtual table
         * 
         * Wraps ::sqlite3_vtab_config
         * 
         * @tparam Code One of the SQLITE_VTAB_ options. Needs to be explicitly specified
         * @tparam Args depend on the @p Code template parameter
         * 
         * The following table lists required argument types for each option.
         * Supplying wrong argument types will result in compile-time error.
         * 
         * @include{doc} vtab-options.md
         * 
         */
        template<int Code, class ...Args>
        auto vtab_config(Args && ...args) -> 
        #ifndef DOXYGEN
            //void but prevents instantiation with wrong types
            decltype(
              vtab_config_mapping<Code>::type::apply(*this, std::forward<decltype(args)>(args)...)
            )
        #else
            void
        #endif
            { vtab_config_mapping<Code>::type::apply(*this, std::forward<Args>(args)...); }

        /**
         * Determine the virtual table conflict policy
         * 
         * Equivalent to ::sqlite3_vtab_on_conflict
         * 
         * @returns One of the [SQLITE_ROLLBACK, SQLITE_IGNORE, SQLITE_FAIL, 
         * SQLITE_ABORT, or SQLITE_REPLACE](https://www.sqlite.org/c3ref/c_fail.html)
         * conflict resolution modes
         */
        int vtab_on_conflict() const noexcept 
            { return sqlite3_vtab_on_conflict(c_ptr()); }

        //MARK: - drop_modules
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 30, 0)
        /**
         * Remove all virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules with nullptr second argument
         * 
         * @since SQLite 3.30
         */
        void drop_modules()
            { check_error(sqlite3_drop_modules(c_ptr(), nullptr)); }

        /**
         * Remove virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules
         * 
         * @since SQLite 3.30
         */                  
        void drop_modules_except(const char * const * keep)
            { check_error(sqlite3_drop_modules(c_ptr(), (const char **)keep)); }
        
        
        /**
         * Remove virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules
         * 
         * @since SQLite 3.30
         */      
        template<size_t N>
        SQLITEPP_ENABLE_IF(N > 0,
        void) drop_modules_except(const char * const (&keep)[N])
        {
            if (keep[N-1] != nullptr) throw exception(SQLITE_MISUSE);
            check_error(sqlite3_drop_modules(this->c_ptr(), (const char **)keep));
        }
        
        /**
         * Remove virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules
         * 
         * @param args Any combination of `const char *` and `std::string` arguments
         * that specify names of the modules to keep 
         * 
         * @since SQLite 3.30
         */  
        template<class ...Args>
        SQLITEPP_ENABLE_IF((std::conjunction_v<std::is_convertible<Args, string_param>...>),
        void) drop_modules_except(Args && ...args)
        {
            const char * buf[] = {string_param(std::forward<Args>(args)).c_str() ..., nullptr};
            check_error(sqlite3_drop_modules(this->c_ptr(), buf));
        }
#endif

        /// @}
        
        //MARK: -

        /**
         * Enable or disable extended result codes
         * 
         * Equivalent to ::sqlite3_extended_result_codes
         */
        void extended_result_codes(bool onoff)
            { check_error(sqlite3_extended_result_codes(c_ptr(), onoff)); }
        
        
        /**
         * Low-level control of database file
         * 
         * Equivalent to ::sqlite3_file_control
         */
        void file_control(const string_param & db_name, int op, void * data)
            { check_error(sqlite3_file_control(c_ptr(), db_name.c_str(), op, data)); }
        
        /**
         * Return the filename for the database connection
         * 
         * Equivalent to ::sqlite3_db_filename
         */
        const char * filename(const string_param & db_name) const noexcept
        {
            auto ret = sqlite3_db_filename(c_ptr(), db_name.c_str());
            return ret ? ret : "";
        }
        
        /**
         * Return the auto-commit mode
         * 
         * Equivalent to ::sqlite3_get_autocommit
         */
        bool get_autocommit() const noexcept
            { return sqlite3_get_autocommit(c_ptr()); }
        
        /**
         * Interrupt a long-running query
         * 
         * Equivalent to ::sqlite3_interrupt
         */
        void interrupt() noexcept
            { sqlite3_interrupt(c_ptr()); }

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 41, 0)
        /**
         * Returns whether or not an interrupt is currently in effect
         * 
         * Equivalent to ::sqlite3_is_interrupted
         * 
         * @since SQLite 3.41
         */
        bool is_interrupted() noexcept
            { return sqlite3_is_interrupted(c_ptr()) != 0; }
    #endif
        
        /**
         * Returns last insert rowid
         * 
         * Equivalent to ::sqlite3_last_insert_rowid
         */
        int64_t last_insert_rowid() const noexcept
            { return sqlite3_last_insert_rowid(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 18, 0)
        /** 
         * Set the last insert rowid value
         * 
         * Equivalent to ::sqlite3_set_last_insert_rowid 
         * 
         * @since SQLite 3.18
         */
        void set_last_insert_rowid(int64_t value) noexcept
            { sqlite3_set_last_insert_rowid(c_ptr(), value); }
#endif
        
        //returns -1 on bad limit or other issues
        /**
         * Set or retrieve run-time limits
         * 
         * Equivalent to ::sqlite3_limit
         * 
         * @param id one of the [limit categories](https://www.sqlite.org/c3ref/c_limit_attached.html)
         * @param new_val new value or -1 to query
         * @returns prior value of the limit or -1 on bad limit or other issues
         */
        int limit(int id, int new_val) noexcept
            { return sqlite3_limit(c_ptr(), id, new_val); }



        /** @{
         * @name Extension management
         */

        /**
         * Enable or disable extension loading
         * 
         * Equivalent to ::sqlite3_enable_load_extension
         */
        void enable_load_extension(bool val)
            { check_error(call_sqlite3_enable_load_extension(this->c_ptr(), val)); }
        
        
        /**
         * Load an extension
         * 
         * Equivalent to ::sqlite3_load_extension
         */
        void load_extension(const string_param & file, const string_param & proc = nullptr);

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 8, 7)

        /**
         * Automatically load statically linked extension
         * 
         * Equivalent to ::sqlite3_auto_extension
         * 
         * @since SQLite 3.8.7
         */
        void auto_extension(void(*entry_point)(database *, const char **, const struct sqlite3_api_routines *))
            { check_error(sqlite3_auto_extension((void(*)(void))entry_point)); }

    
        /**
         * Cancel automatic extension Loading
         * 
         * Equivalent to ::sqlite3_cancel_auto_extension
         * 
         * @since SQLite 3.8.7
         */
        void cancel_auto_extension(void(*entry_point)(database *, const char **, const struct sqlite3_api_routines *))
            { check_error(sqlite3_cancel_auto_extension((void(*)(void))entry_point)); }

    
        /**
         * Reset automatic extension loading
         * 
         * Equivalent to ::sqlite3_reset_auto_extension
         * 
         * @since SQLite 3.8.7
         */
        void reset_auto_extension() noexcept
            { sqlite3_reset_auto_extension(); }

    #endif

        /** @} */
        
        /**
         * Retrieve the mutex for the database connection
         * 
         * Equivalent to ::sqlite3_db_mutex
         */
        class mutex * mutex() const noexcept
            { return (class mutex *)sqlite3_db_mutex(c_ptr()); }
        
        //MARK: - next_statement
        
        /**
         * Find the next prepared statement
         * 
         * Equivalent to ::sqlite3_next_stmt
         */
        const class statement * next_statement(const class statement * prev) const noexcept
            { return (class statement *)sqlite3_next_stmt(c_ptr(), (sqlite3_stmt *)prev); }
        
        /// @overload
        class statement * next_statement(const class statement * prev) noexcept
            { return (class statement *)sqlite3_next_stmt(c_ptr(), (sqlite3_stmt *)prev); }
        
        //MARK: -
        
        /**
         * Overload a function for a virtual table
         * 
         * Equivalent to ::sqlite3_overload_function
         */
        void overload_function(const string_param & name, int arg_count) noexcept
            { check_error(sqlite3_overload_function(c_ptr(), name.c_str(), arg_count)); }
        
        //MARK: - progress_handler

        /**
         * Register a callback to be called on query progress
         * 
         * Equivalent to ::sqlite3_progress_handler
         * 
         * @param step_count An approximate number of 
         * [virtual machine instructions](https://www.sqlite.org/opcode.html)
         * that are evaluated between successive invocations of the callback.
         * If less than one then the progress handler is disabled.
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         * 
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) progress_handler(int step_count, int(*handler)(type_identity_t<T> data_ptr) noexcept, T data_ptr) const noexcept
            { sqlite3_progress_handler(this->c_ptr(), step_count, (int(*)(void*))handler, data_ptr); }
        
        /**
         * Register a callback to be called on query progress
         * 
         * Equivalent to ::sqlite3_progress_handler
         * 
         * @param step_count An approximate number of 
         * [virtual machine instructions](https://www.sqlite.org/opcode.html)
         * that are evaluated between successive invocations of the callback.
         * If less than one then the progress handler is disabled.
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)();
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<bool, std::remove_pointer_t<T>>)),
        void) progress_handler(int step_count, T handler_ptr) const noexcept;
        
        //MARK: -
        /**
         * Determine if a database is read-only
         * 
         * Equivalent to ::sqlite3_db_readonly
         * 
         * @param db_name database name
         * @return `true` if the database named @p db_name is readonly, `false` if it is
         * read/write or `std::nullopt` if @p db_name is not a name of a database on this
         * connection.
         */
        std::optional<bool> readonly(const string_param & db_name) const noexcept;
        
        /**
         * Free memory used by the database connection
         * 
         * Equivalent to ::sqlite3_db_release_memory
         */
        void release_memory() const
            { check_error(sqlite3_db_release_memory(c_ptr())); }
        
        //MARK: - status

        /// Return type for @ref status()
        struct status
        {
            int current;
            int high;
        };

        /**
         * Retrieve database connection status
         * 
         * Equivalent to ::sqlite3_db_status
         */
        struct status status(int op, bool reset = false) const;
        
        //MARK: - table_column_metadata
        
        /// Return type for table_column_metadata()
        struct column_metadata
        {
            const char * data_type;             ///< Declared data type
            const char * collation_sequence;    ///< Collation sequence name
            bool not_null;                      ///< Whether NOT NULL constraint exists
            bool primary_key;                   ///< Whether column part of PK
            bool auto_increment;                ///< Whether column is auto-increment
        };

        /**
         * Extract metadata about a column of a table
         * 
         * Equivalent to ::sqlite3_table_column_metadata
         */
        column_metadata table_column_metadata(const string_param & db_name, 
                                              const string_param & table_name, 
                                              const string_param & column_name) const;
        
        //MARK: -
        
        /**
         * Returns total number of rows modified
         * 
         * Equivalent to ::sqlite3_total_changes
         */
        int64_t total_changes() const noexcept
        { 
        #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 36, 1)
            return sqlite3_total_changes64(c_ptr()); 
        #else
            return sqlite3_total_changes(c_ptr()); 
        #endif
        }
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 34, 0)

        /**
         * Returns the transaction state of a database
         * 
         * Equivalent to ::sqlite3_txn_state
         * 
         * @since SQLite 3.34
         */
        int txn_state(const string_param & schema) const noexcept
            { return sqlite3_txn_state(c_ptr(), schema.c_str()); }
#endif

        /**
         * Open a blob
         * 
         * Equivalent to ::sqlite3_blob_open
         */
        std::unique_ptr<blob> open_blob(const string_param & dbname, 
                                        const string_param & table,
                                        const string_param & column,
                                        int64_t rowid,
                                        bool writable);

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 39, 0)

        /** @{
         * @name Serialization
         */

        /**
         * Serialize a database 
         * 
         * Equivalent to ::sqlite3_serialize with flags set 0
         * 
         * @since SQLite 3.39
         */
        std::pair<allocated_bytes, size_t> serialize(const string_param & schema_name);

        /**
         * Serialize a database 
         * 
         * Equivalent to ::sqlite3_serialize with flags set SQLITE_SERIALIZE_NOCOPY
         * 
         * @since SQLite 3.39
         */
        span<std::byte> serialize_reference(const string_param & schema_name) noexcept;


        /**
         * Deserialize a database
         * 
         * Equivalent to ::sqlite3_deserialize
         * 
         * @since SQLite 3.39
         */
        void deserialize(const string_param & schema_name, 
                         std::byte * buf, 
                         size_t size, 
                         size_t buf_size,
                         unsigned flags = 0)
            { check_error(sqlite3_deserialize(c_ptr(), schema_name.c_str(), (unsigned char *)buf, int64_size(size), int64_size(buf_size), flags)); }
        
        
        /**
         * Deserialize a database
         * 
         * A convenience overload for immutable data
         * 
         * Equivalent to ::sqlite3_deserialize with SQLITE_DESERIALIZE_READONLY flag always added
         * 
         * @since SQLite 3.39
         */
        void deserialize(const string_param & schema_name, 
                         const std::byte * buf, 
                         size_t size, 
                         size_t buf_size,
                         unsigned flags = 0)
            { deserialize(schema_name, (std::byte *)buf, size, buf_size, flags | SQLITE_DESERIALIZE_READONLY); }


        /**
         * Deserialize a database
         * 
         * A convenience overload that takes ownership over passed pointer
         * 
         * Equivalent to ::sqlite3_deserialize with SQLITE_DESERIALIZE_FREEONCLOSE flag always added
         * 
         * @since SQLite 3.39
         */
        void deserialize(const string_param & schema_name, 
                         allocated_bytes buf, 
                         size_t size, 
                         size_t buf_size,
                         unsigned flags = 0);
        


        /** @} */
#endif

        /** @{
         * @name Snapshots
         */

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) && THINSQLITEPP_ENABLE_EXPIREMENTAL
        /**
         * Record a database snapshot
         * 
         * Equivalent to ::sqlite3_snapshot_get
         * 
         * Requires THINSQLITEPP_ENABLE_EXPIREMENTAL macro defined to 1 as the underlying SQLite
         * feature is experimental.
         * 
         * @since SQLite 3.10
         */
        std::unique_ptr<snapshot> get_snapshot(const string_param & schema);

        /**
         * Start a read transaction on an historical snapshot
         * 
         * Equivalent to ::sqlite3_snapshot_open
         * 
         * Requires THINSQLITEPP_ENABLE_EXPIREMENTAL macro defined to 1 as the underlying SQLite
         * feature is experimental.
         * 
         * @since SQLite 3.10
         */
        void open_snapshot(const string_param & schema, const snapshot & snap)
            { check_error(sqlite3_snapshot_open(c_ptr(), schema.c_str(), snap.c_ptr())); }

        /**
         * Recover snapshots from a wal file
         * 
         * Equivalent to ::sqlite3_snapshot_recover
         * 
         * Requires THINSQLITEPP_ENABLE_EXPIREMENTAL macro defined to 1 as the underlying SQLite
         * feature is experimental.
         * 
         * @since SQLite 3.10
         */
        void recover_snapshot(const string_param & db)
            { check_error(sqlite3_snapshot_recover(c_ptr(), db.c_str())); }

#endif

        /** @} */
        
        
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 39, 0)
        /**
         * Return schema names
         *
         * Equivalent to ::sqlite3_db_name
         * 
         * @param idx Schema index. 0 means the main database file and 1 is 
         * the "temp" schema. Larger values correspond to various ATTACH-ed databases.
         * 
         * @since SQLite 3.39
         */
        const char * db_name(int idx) noexcept
            { return sqlite3_db_name(c_ptr(), idx); }

    #endif

        //MARK: - Private methods
        
    private:
        void check_error(int res) const
        {
            if (res != SQLITE_OK)
                throw exception(res, this);
        }

        template<class T>
        static int call_sqlite3_enable_load_extension(T * db, int onoff)
        {
            if constexpr (database_detector::has_enable_load_extension<T>)
                return sqlite3_enable_load_extension(db, onoff);
            else
                return SQLITE_ERROR;
        }

        template<class T>
        static int call_sqlite3_load_extension(T * db, const char * file, const char * proc, char ** err)
        {
            if constexpr (database_detector::has_load_extension<T>)
                return sqlite3_load_extension(db, file, proc, err);
            else
                return SQLITE_ERROR;
        }
    };

    /** @} */

    /** @cond PRIVATE */
    

    SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN

    #if SQLITEPP_USE_VARARG_POUND_POUND_TRICK

        #define SQLITEPP_DEFINE_DB_OPTION(code, ...) \
            template<> struct database::config_mapping<code> { using type = database::config_option<code, ##__VA_ARGS__>; };

        //Idiotic GCC in pedantic mode warns on MACRO(arg) for MARCO(x,...) in < C++20 mode
        //with no way to disable the warning(!!!). 
        #define SQLITEPP_DEFINE_VTAB_OPTION_0(code) \
            template<> struct database::vtab_config_mapping<code> { using type = database::vtab_config_option<code>; };
        #define SQLITEPP_DEFINE_VTAB_OPTION_N(code, ...) \
            template<> struct database::vtab_config_mapping<code> { using type = database::vtab_config_option<code, ##__VA_ARGS__>; };

    #else

        #define SQLITEPP_DEFINE_DB_OPTION(code, ...) \
            template<> struct database::config_mapping<code> { using type = database::config_option<code __VA_OPT__(,) __VA_ARGS__>; };

        #define SQLITEPP_DEFINE_VTAB_OPTION_N(code, ...) \
            template<> struct database::vtab_config_mapping<code> { using type = database::vtab_config_option<code __VA_OPT__(,) __VA_ARGS__>; };

        #define SQLITEPP_DEFINE_VTAB_OPTION_0(code) SQLITEPP_DEFINE_VTAB_OPTION_N(code)

    #endif


    //@ [DB Options]
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
    //@ [DB Options]

    //@ [VTab Options]

    SQLITEPP_DEFINE_VTAB_OPTION_N(SQLITE_VTAB_CONSTRAINT_SUPPORT,       int);
#ifdef SQLITE_VTAB_INNOCUOUS
    SQLITEPP_DEFINE_VTAB_OPTION_0(SQLITE_VTAB_INNOCUOUS                 );
#endif
#ifdef SQLITE_VTAB_DIRECTONLY
    SQLITEPP_DEFINE_VTAB_OPTION_0(SQLITE_VTAB_DIRECTONLY                );
#endif
#ifdef SQLITE_VTAB_USES_ALL_SCHEMAS
    SQLITEPP_DEFINE_VTAB_OPTION_0(SQLITE_VTAB_USES_ALL_SCHEMAS          );
#endif

    //@ [VTab Options]

    #undef SQLITEPP_DEFINE_DB_OPTION
    #undef SQLITEPP_DEFINE_VTAB_OPTION_0
    #undef SQLITEPP_DEFINE_VTAB_OPTION_N

    SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END

    /** @endcond */
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#endif

