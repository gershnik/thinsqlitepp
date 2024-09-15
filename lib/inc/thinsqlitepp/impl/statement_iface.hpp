/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_STATEMENT_IFACE_INCLUDED
#define HEADER_SQLITEPP_STATEMENT_IFACE_INCLUDED

#include "handle.hpp"
#include "string_param.hpp"
#include "span.hpp"
#include "memory_iface.hpp"

#include <utility>
#include <string>
#include <string_view>

namespace thinsqlitepp
{
    class database;
    class value;

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Prepared Statement Object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_stmt.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     * 
     */
    class statement final : public handle<sqlite3_stmt, statement>
    {
    public:
        /**
         * Compile an SQL statement
         * 
         * This is a wrapper over ::sqlite3_prepare_v3 or ::sqlite3_prepare_v2, if the former is
         * not available.
         * 
         * @param db The database to create statement for
         * @param sql The statement to be compiled. Must be in UTF-8.
         * @param flags Zero or more SQLITE_PREPARE_ flags. Only available for SQLite 3.2 or greater
         */
        static std::unique_ptr<statement> create(const database & db, const string_param & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 );

        /**
         * Compile an SQL statement
         * 
         * This is a wrapper over ::sqlite3_prepare_v3 or ::sqlite3_prepare_v2, if the former is
         * not available.
         * 
         * @param db The database to create statement for
         * @param sql The statement to be compiled. Must be in UTF-8. This is an input-output parameter.
         *            On output the string_view is adjusted to contain any text past the end of the first SQL statement.
         *            See `pzTail` argument description for ::sqlite3_prepare_v3
         * @param flags Zero or more SQLITE_PREPARE_ flags. Only available for SQLite 3.2 or greater
         */
        static std::unique_ptr<statement> create(const database & db, std::string_view & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 );

#if __cpp_char8_t >= 201811
        /**
         * Compile an SQL statement
         * 
         * char8_t overload for create(const database &, const string_param &, unsigned int)
         */ 
        static std::unique_ptr<statement> create(const database & db, const u8string_param & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 )
        {
            return create(db, (const char *)sql.c_str()
                        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                          , flags
                        #endif
                   );
        }

        /**
         * Compile an SQL statement
         * 
         * char8_t overload for create(const database &, std::string_view &, unsigned int)
         */
        static std::unique_ptr<statement> create(const database & db, std::u8string_view & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 )
        {
            return create(db, *reinterpret_cast<std::string_view *>(&sql)
                        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                          , flags
                        #endif
                   );
        }
#endif
        
        /// Equivalent to ::sqlite3_finalize
        ~statement() noexcept
            { sqlite3_finalize(c_ptr()); }

        /**
         * Returns the database to which this statement belongs
         * 
         * Equivalent to ::sqlite3_db_handle
         */
        class database & database() const noexcept
            { return *(class database *)sqlite3_db_handle(c_ptr()); }
        
        /**
         * Evaluate the statement
         * 
         * Equivalent to ::sqlite3_step.
         * 
         * Returns true if a row was retrieved (#SQLITE_ROW) or false if the 
         * statement has finished executing successfully (#SQLITE_DONE).
         * 
         * All other ::sqlite3_step return codes result in @ref exception being thrown
         */
        bool step();

        /**
         * Reset the statement
         * 
         * Equivalent to ::sqlite3_reset
         */
        void reset() noexcept
            { sqlite3_reset(c_ptr()); }
        
        /**
         * Determine if the statement has been reset
         * 
         * Equivalent to ::sqlite3_stmt_busy
         */
        bool busy() const noexcept
            { return sqlite3_stmt_busy(c_ptr()); }

        /** 
         * Return type for isexplain()
         */ 
        enum class explain_type : int
        {
            not_explain = 0,        ///< The statement is an ordinary statement
            explain = 1,            ///< The statement is an EXPLAIN statement
            explain_query_plan = 2  ///< The statement is an EXPLAIN QUERY PLAN
        };
        
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 31, 1)
        /**
         * Query the EXPLAIN Setting for the statement
         * 
         * Equivalent to ::sqlite3_stmt_isexplain. 
         * 
         * @since SQLite 3.31
         */
        explain_type isexplain() const noexcept
            { return explain_type(sqlite3_stmt_isexplain(c_ptr())); }
#endif
        
        /**
         * Determine if the statement writes to the database
         * 
         * Equivalent to ::sqlite3_stmt_readonly
         */
        bool readonly() const noexcept
            { return sqlite3_stmt_readonly(c_ptr()); }

        /** @{
         * @anchor statement_bind
         * @name Binding values to parameters
         * 
         * This set of overloaded functions wraps @ref sqlite3_bind_ function
         * group. 
         */

        /**
         * Bind a NULL value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_null
         */
        void bind(int idx, std::nullptr_t)
            { check_error(sqlite3_bind_null(c_ptr(), idx)); }
        /**
         * Bind an int value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_int
         */
        void bind(int idx, int val)
            { check_error(sqlite3_bind_int(c_ptr(), idx, val)); }
        /**
         * Bind an int64_t value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_int64
         */
        void bind(int idx, int64_t val)
            { check_error(sqlite3_bind_int64(c_ptr(), idx, val)); }
        /**
         * Bind a double value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_double
         */
        void bind(int idx, double val)
            { check_error(sqlite3_bind_double(c_ptr(), idx, val)); }

        
        /**
         * Bind a string value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_text with #SQLITE_TRANSIENT.
         * 
         * The string content is used **by value** and copied into the statement.
         * Thus the lifetime of the string referred to by `value` parameter is 
         * independent of the statement's
         */
        void bind(int idx, const std::string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_text with #SQLITE_STATIC.
         * 
         * The string content is used **by reference**.
         * Thus the string referred to by `value` parameter must
         * remain valid during this statement's lifetime.
         */
        void bind_reference(int idx, const std::string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_text(..., unref)
         * 
         * The string content is used **by reference**.
         * 
         * @param idx index of the SQL parameter to be bound 
         * @param val reference to string to bind to the parameter
         * @param unref called when the reference is no longer needed. 
         * Its argument is the pointer returned from `value.data()`
         */
        void bind_reference(int idx, const std::string_view & val, void (*unref)(const char *));

    #if __cpp_char8_t >= 201811
        /**
         * Bind a string value to a parameter of the statement
         * 
         * char8_t overload for bind(int, const std::string_view &)
         */
        void bind(int idx, const std::u8string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * char8_t overload for bind_reference(int, const std::string_view &)
         */
        void bind_reference(int idx, const std::u8string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * char8_t overload for bind_reference(int, const std::string_view &, void (*)(const char *))
         */
        void bind_reference(int idx, const std::u8string_view & val, void (*unref)(const char8_t *));
    #endif

        /**
         * Bind a blob value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_blob with #SQLITE_TRANSIENT.
         * 
         * The blob content is used **by value** and copied into the statement.
         * Thus the lifetime of the blob referred to by `value` parameter is 
         * independent of the statement's
         */
        void bind(int idx, const blob_view & val);

        /**
         * Bind a blob reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_blob with #SQLITE_STATIC.
         * 
         * The blob content is used **by reference**.
         * Thus the string referred to by `value` parameter must
         * remain valid during this statement's lifetime.
         */
        void bind_reference(int idx, const blob_view & val);

        /**
         * Bind a blob reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_blob (..., unref)
         * 
         * The blob content is used **by reference**.
         * 
         * @param idx index of the SQL parameter to be bound 
         * @param val reference to blob to bind to the parameter
         * @param unref called when the reference is no longer needed. 
         * Its argument is the pointer returned from `value.data()`
         */
        void bind_reference(int idx, const blob_view & val, void (*unref)(const std::byte *));

        /**
         * Bind a blob of zeroes to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_zeroblob.
         */
        void bind(int idx, const zero_blob & val)
            { check_error(sqlite3_bind_zeroblob(c_ptr(), idx, int(val.size()))); }

        /**
         * Bind a custom pointer to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_pointer. 
         * 
         * The `type` parameter should be a static string, preferably a string literal.
         */
        template<class T>
        void bind(int idx, T * ptr, const char * type, void(*destroy)(T*))
            { check_error(sqlite3_bind_pointer(this->c_ptr(), idx, ptr, type, (void(*)(void*))destroy)); }

        /**
         * Bind a custom pointer to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_pointer. 
         * 
         * This is a safer overload of bind(int, T *, const char *, void(*)(T*))
         * that takes a pointer via std::unique_ptr ownership transfer. The inferred
         * "type" for ::sqlite3_bind_pointer is `typeid(T).name()`.
         */
        template<class T>
        void bind(int idx, std::unique_ptr<T> ptr)
            { this->bind(idx, ptr.release(), typeid(T).name(), [](T * p) { delete p; }); }

        /**
         * Bind a dynamically typed value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_value. 
         */
        void bind(int idx, const value & val);

        ///@}
        
        /** @{
         * @anchor statement_managing_binding
         * @name Managing parameter bindings
         */

        /**
         * Reset all bindings on the statement
         * 
         * Equivalent to ::sqlite3_clear_bindings. 
         */
        void clear_bindings() noexcept
            { sqlite3_clear_bindings(c_ptr()); }
        
        /**
         * Returns the number of SQL parameters
         * 
         * Equivalent to ::sqlite3_bind_parameter_count
         */
        int bind_parameter_count() const noexcept
            { return sqlite3_bind_parameter_count(c_ptr()); }

        /**
         * Returns the index of a parameter with a given name
         * 
         * Equivalent to ::sqlite3_bind_parameter_index
         */
        int bind_parameter_index(const string_param & name) const noexcept
            { return sqlite3_bind_parameter_index(c_ptr(), name.c_str()); }

        /**
         * Returns the name of a parameter with a given index
         * 
         * Equivalent to ::sqlite3_bind_parameter_name
         */
        const char * bind_parameter_name(int idx) const noexcept
            { return sqlite3_bind_parameter_name(c_ptr(), idx); }

        ///@}


        /**
         * Number of columns in a result set
         * 
         * Equivalent to ::sqlite3_column_count
         * 
         * Note that ::sqlite3_column_count represented by this function and 
         * ::sqlite3_data_count represented by data_count() are subtly and confusingly 
         * different. See their respective documentation for details.
         * 
         * @see data_count
         */
        int column_count() const noexcept
            { return sqlite3_column_count(c_ptr()); }

        /**
         * Number of columns in a result set
         * 
         * Equivalent to ::sqlite3_data_count
         * 
         * Note that ::sqlite3_data_count represented by this function and 
         * ::sqlite3_column_count represented by column_count() are subtly and confusingly 
         * different. See their respective documentation for details.
         * 
         * @see column_count
         */
        int data_count() const noexcept
            { return sqlite3_data_count(c_ptr()); }

    private:
        template<typename T>
        static constexpr bool supported_column_type = 
            std::is_same_v<T, int> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string_view> ||
        #if __cpp_char8_t >= 201811
            std::is_same_v<T, std::u8string_view> ||
        #endif
            std::is_same_v<T, blob_view>;

    public:

        /** @{
         * @anchor statement_column_info
         * @name Obtaining query results information by column
         */
        
        /**
         * Get result value from a query 
         * 
         * Wraps @ref sqlite3_column_ function family. Unlike the C API you specify the
         * desired type via T template parameter
         * 
         * @tparam T Desired output type. Must be one of:
         * - int
         * - int64_t
         * - double
         * - std::string_view
         * - std::u8string_view (if `char8_t` is supported by your compiler/library)
         * - blob_view
         * @param idx Column index
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_column_type<T>,
        T) column_value(int idx) const noexcept;
        
        /**
         * Get result values from a query as a raw @ref value object
         * 
         * Equivalent to ::sqlite3_column_value
         */
        const value & raw_column_value(int idx) const noexcept
            { return *(const value *)sqlite3_column_value(c_ptr(), idx); }

        /**
         * Default datatype of the result column
         * 
         * Equivalent to ::sqlite3_column_type
         * 
         * @returns One of the [SQLite fundamental datatypes](https://www.sqlite.org/c3ref/c_blob.html)
         */
        int column_type(int idx) const noexcept
            { return sqlite3_column_type(c_ptr(), idx); }

        /**
         * Name of the result column
         * 
         * Equivalent to ::sqlite3_column_name
         * 
         * The returned string pointer is valid until either the 
         * statement is destroyed or until the statement is automatically 
         * re-prepared by the first call to step() for a particular run or 
         * until the next call to column_name() on the same column.
         */
        const char * column_name(int idx) const noexcept
            { return sqlite3_column_name(c_ptr(), idx); }
        
        /**
         * Database that is the origin of a result column
         * 
         * Equivalent to ::sqlite3_column_database_name
         */
        const char * column_database_name(int idx) const noexcept
            { return sqlite3_column_database_name(c_ptr(), idx); }

        /**
         * Table that is the origin of a result column
         * 
         * Equivalent to ::sqlite3_column_table_name
         */
        const char * column_table_name(int idx) const noexcept
            { return sqlite3_column_table_name(c_ptr(), idx); }

        /**
         * Table column that is the origin of a result column
         * 
         * Equivalent to ::sqlite3_column_origin_name
         */
        const char * column_origin_name(int idx) const noexcept
            { return sqlite3_column_origin_name(c_ptr(), idx); }

        /**
         * Declared datatype of a result column
         * 
         * Equivalent to ::sqlite3_column_decltype
         */
        const char * column_declared_type(int idx) const noexcept
            { return sqlite3_column_decltype(c_ptr(), idx); }

        ///@}
        
        /**
         * Returns a pointer to a copy of the SQL text used to create the statement
         * 
         * Equivalent to ::sqlite3_sql
         */
        const char * sql() const noexcept
            { return sqlite3_sql(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 14, 0)
        /**
         * Returns SQL text of the statement with bound parameters expanded
         * 
         * Equivalent to ::sqlite3_expanded_sql
         * 
         * @since SQLite 3.14
         */
        allocated_string expanded_sql() const;
#endif
        
    private:
        void check_error(int res) const;
    };

    /// @cond PRIVATE

    template<>
    inline int statement::column_value<int>(int idx) const noexcept
        { return sqlite3_column_int(c_ptr(), idx); }

    template<>
    inline int64_t statement::column_value<int64_t>(int idx) const noexcept
        { return sqlite3_column_int64(c_ptr(), idx); }

    template<>
    inline double statement::column_value<double>(int idx) const noexcept
        { return sqlite3_column_double(c_ptr(), idx); }

    /// @endcond

    /** @} */

    /**
     * @addtogroup Utility Utilities
     * @{
     */


    /**
     * Parses text containing multiple SQL statements
     * 
     * This helper class allows you to iterate over text containing multiple SQL
     * statements and generate @ref statement instances from them 
     */
    class statement_parser
    {
    public:
        /// Create a parser for the given database and SQL text
        statement_parser(const database & db, std::string_view sql):
            _db(&db),
            _sql(sql)
        {}
        
        /**
         * Return the next statement if any
         * 
         * @returns Next statement or nullptr when done
         */
        std::unique_ptr<statement> next();
    private:
        const database * _db;
        std::string_view _sql;
    };

    /**
     * Bitwise mask of resets to perform for thinsqlitepp::auto_reset
     * 
     * This enum supports all the normal bitwise operations: `&`, `|`, `^` and `~`
     */
    enum class auto_reset_flags: unsigned
    {
        none = 0,               ///< Reset nothing
        reset = 1,              ///< Reset the statement (does not affect the bindings)
        clear_bindings = 2,     ///< Reset the bindings 
        all = 3                 ///< Reset everything
    };
    constexpr auto_reset_flags operator|(auto_reset_flags lhs, auto_reset_flags rhs)
        { return auto_reset_flags(unsigned(lhs) | unsigned(rhs)); }
    constexpr auto_reset_flags operator&(auto_reset_flags lhs, auto_reset_flags rhs)
        { return auto_reset_flags(unsigned(lhs) & unsigned(rhs)); }
    constexpr auto_reset_flags operator^(auto_reset_flags lhs, auto_reset_flags rhs)
        { return auto_reset_flags(unsigned(lhs) ^ unsigned(rhs)); }
    constexpr auto_reset_flags operator~(auto_reset_flags arg)
        { return auto_reset_flags(~unsigned(arg)); }

    /**
     * RAII wrapper that resets @ref statement on destruction
     * 
     * This class allows you to restore the state of a @ref statement after using it.
     * This allows you to reuse the statement cleanly without having to worry about resetting
     * it properly on different code paths.
     * 
     * @tparam Flags @ref auto_reset_flags specifying what kind of reset to perform on destruction
     */
    template<auto_reset_flags Flags>
    class auto_reset
    {
    public:
        /// Constructs an empty instance with no statement
        auto_reset():
            _st(nullptr)
        {}
        /**
         * Constructs an instance referring to a given statement
         * 
         * The statement is being help by reference and must exist as long as
         * this object is existing.
         */
        auto_reset(const std::unique_ptr<statement> & st) noexcept:
            _st(st.get())
        {}

        /// @overload
        auto_reset(statement * st) noexcept:
            _st(st)
        {}

        auto_reset(const auto_reset &) = delete;
        auto_reset & operator=(const auto_reset &) = delete;
        auto_reset(auto_reset && src) noexcept:
            _st(src._st)
        {
            src._st = nullptr;
        }
        auto_reset & operator=(auto_reset && src) noexcept
        {
            destroy();
            _st = src._st;
            src._st = nullptr;
            return *this;
        }

        /// Resets the statement if present
        ~auto_reset() noexcept
            { destroy(); }

        /// Access the stored @ref statement 
        statement * operator->() const noexcept
            { return _st; }

    private:
        void destroy() noexcept
        {
            if (_st)
            {
                if constexpr ((Flags & auto_reset_flags::reset) != auto_reset_flags::none)
                    _st->reset();
                if constexpr ((Flags & auto_reset_flags::clear_bindings) != auto_reset_flags::none)
                    _st->clear_bindings();
            }
        }
    private:
        statement * _st;
    };

    /** @} */

}

#endif

