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
#include "memory.hpp"

#include <utility>
#include <string>
#include <string_view>

namespace thinsqlitepp
{
    class database;
    class value;

    class statement final : public handle<sqlite3_stmt, statement>
    {
    public:
        static std::unique_ptr<statement> create(const database & db, const string_param & sql
#if SQLITE_VERSION_NUMBER >= 3020000
                                                 , unsigned int flags = 0
#endif
                                                 );
        static std::unique_ptr<statement> create(const database & db, std::string_view & sql
#if SQLITE_VERSION_NUMBER >= 3020000
                                                 , unsigned int flags = 0
#endif
                                                 );
        
        ~statement() noexcept
            { sqlite3_finalize(c_ptr()); }
        
        class database & database() const noexcept
            { return *(class database *)sqlite3_db_handle(c_ptr()); }
        
        bool step();
        void reset()
            { check_error(sqlite3_reset(c_ptr())); }
        
        bool busy() const noexcept
            { return sqlite3_stmt_busy(c_ptr()); }
        
        enum class explain_type : int
        {
            not_explain = 0,
            explain = 1,
            explain_query_plan = 2
        };
        
#if SQLITE_VERSION_NUMBER >= 3031001
        explain_type isexplain() const noexcept
            { return explain_type(sqlite3_stmt_isexplain(c_ptr())); }
#endif
        
        bool readonly() const noexcept
            { return sqlite3_stmt_readonly(c_ptr()); }
        
        void bind(int idx, std::nullptr_t)
            { check_error(sqlite3_bind_null(c_ptr(), idx)); }
        void bind(int idx, int value)
            { check_error(sqlite3_bind_int(c_ptr(), idx, value)); }
        void bind(int idx, int64_t value)
            { check_error(sqlite3_bind_int64(c_ptr(), idx, value)); }
        void bind(int idx, double value)
            { check_error(sqlite3_bind_double(c_ptr(), idx, value)); }
        void bind(int idx, const std::string_view & value, bool copy = false);
        void bind(int idx, const blob_view & value, bool copy = false);
        void bind(int idx, const zero_blob & value)
            { check_error(sqlite3_bind_zeroblob(c_ptr(), idx, int(value.size()))); }
        template<class T>
        void bind(int idx, T * ptr, const char * type, void(*destroy)(T*))
            { check_error(sqlite3_bind_pointer(this->c_ptr(), idx, ptr, type, (void(*)(void*))destroy)); }
        template<class T>
        void bind(int idx, std::unique_ptr<T> ptr)
            { this->bind(idx, ptr.release(), typeid(T).name(), [](T * p) { delete p; }); }
        void bind(int idx, const value & val);
        
        void clear_bindings()
            { check_error(sqlite3_clear_bindings(c_ptr())); }
        
        int bind_parameter_count() const noexcept
            { return sqlite3_bind_parameter_count(c_ptr()); }
        int bind_parameter_index(const string_param & name) const noexcept
            { return sqlite3_bind_parameter_index(c_ptr(), name.c_str()); }
        const char * bind_parameter_name(int idx) const noexcept
            { return sqlite3_bind_parameter_name(c_ptr(), idx); }
        
        template<class T>
        std::enable_if_t<
            std::is_same_v<T, int> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string_view> ||
            std::is_same_v<T, blob_view>,
        T> column_value(int idx) const noexcept;
        
        const value & raw_column_value(int idx) const noexcept
            { return *(const value *)sqlite3_column_value(c_ptr(), idx); }
        
        int column_count() const noexcept
            { return sqlite3_column_count(c_ptr()); }
        int data_count() const noexcept
            { return sqlite3_data_count(c_ptr()); }
        
        int column_type(int idx) const noexcept
            { return sqlite3_column_type(c_ptr(), idx); }
        const char * column_name(int idx) const noexcept
            { return sqlite3_column_name(c_ptr(), idx); }
        
#ifndef SQLITE_ENABLE_COLUMN_METADATA
        const char * column_database_name(int idx) const noexcept
            { return sqlite3_column_database_name(c_ptr(), idx); }
        const char * column_table_name(int idx) const noexcept
            { return sqlite3_column_table_name(c_ptr(), idx); }
        const char * column_origin_name(int idx) const noexcept
            { return sqlite3_column_origin_name(c_ptr(), idx); }
#endif

#ifndef SQLITE_OMIT_DECLTYPE
        const char * column_declared_type(int idx) const noexcept
            { return sqlite3_column_decltype(c_ptr(), idx); }
#endif
        
        
        const char * sql() const noexcept
            { return sqlite3_sql(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= 3014000
        allocated_string expanded_sql() const;
#endif
        
    private:
        void check_error(int res) const;
    };

    template<>
    inline int statement::column_value<int>(int idx) const noexcept
        { return sqlite3_column_int(c_ptr(), idx); }

    template<>
    inline int64_t statement::column_value<int64_t>(int idx) const noexcept
        { return sqlite3_column_int64(c_ptr(), idx); }

    template<>
    inline double statement::column_value<double>(int idx) const noexcept
        { return sqlite3_column_double(c_ptr(), idx); }


    class statement_parser
    {
    public:
        statement_parser(const database & db, std::string_view sql):
            _db(&db),
            _sql(sql)
        {}
        
        std::unique_ptr<statement> next();
    private:
        const database * _db;
        std::string_view _sql;
    };

}

#endif

