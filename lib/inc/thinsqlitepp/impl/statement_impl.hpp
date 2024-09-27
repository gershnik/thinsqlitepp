/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_STATEMENT_IMPL_INCLUDED
#define HEADER_SQLITEPP_STATEMENT_IMPL_INCLUDED

#include "database_iface.hpp"
#include "value_iface.hpp"

namespace thinsqlitepp
{
    inline std::unique_ptr<statement> statement::create(const class database & db, const string_param & sql
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                        , unsigned int flags
#endif
                                                        )
    {
        const char * tail = nullptr;
        sqlite3_stmt * ret = nullptr;
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
        int res = sqlite3_prepare_v3(db.c_ptr(), sql.c_str(), -1, flags, &ret, &tail);
#else
        int res = sqlite3_prepare_v2(db.c_ptr(), sql.c_str(), -1, &ret, &tail);
#endif
        if (res != SQLITE_OK)
            throw exception(res, db);
        return std::unique_ptr<statement>(from(ret));
    }

    inline std::unique_ptr<statement> statement::create(const class database & db, std::string_view & sql
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                        , unsigned int flags
#endif
                                                        )
    {
        const char * start = sql.size() ? &sql[0] : "";
        const char * tail = nullptr;
        sqlite3_stmt * ret = nullptr;
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
        int res = sqlite3_prepare_v3(db.c_ptr(), start, int_size(sql.size()), flags, &ret, &tail);
#else
        int res = sqlite3_prepare_v2(db.c_ptr(), start, int_size(sql.size()), &ret, &tail);
#endif
        if (res != SQLITE_OK)
            throw exception(res, db);
        sql.remove_prefix(tail - start);
        return std::unique_ptr<statement>(from(ret));
    }


    inline bool statement::step()
    {
        int res = sqlite3_step(c_ptr());
        if (res == SQLITE_DONE)
            return false;
        if (res == SQLITE_ROW)
            return true;
        throw exception(res, database());
    }

    inline void statement::bind(int idx, const std::string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, data, int_size(value.size()), SQLITE_TRANSIENT));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, data, int_size(value.size()), SQLITE_STATIC));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::string_view & value, void (*unref)(const char *))
    {
        if (auto data = value.data())
        {
            check_error(sqlite3_bind_text(c_ptr(), idx, data, int_size(value.size()), (void (*)(void *))unref));
        }
        else
        {
            unref(data);
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
        }
    }

#if __cpp_char8_t >= 201811
    inline void statement::bind(int idx, const std::u8string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, (const char *)data, int_size(value.size()), SQLITE_TRANSIENT));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::u8string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, (const char *)data, int_size(value.size()), SQLITE_STATIC));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::u8string_view & value, void (*unref)(const char8_t *))
    {
        if (auto data = value.data())
        {
            check_error(sqlite3_bind_text(c_ptr(), idx, (const char *)data, int_size(value.size()), (void (*)(void *))unref));
        }
        else
        {
            unref(data);
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
        }
    }
#endif

    inline void statement::bind(int idx, const blob_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_blob(c_ptr(), idx, data, int_size(value.size()), SQLITE_TRANSIENT));
        else
            check_error(sqlite3_bind_zeroblob(c_ptr(), idx, 0));
    }

    inline void statement::bind_reference(int idx, const blob_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_blob(c_ptr(), idx, data, int_size(value.size()), SQLITE_STATIC));
        else
            check_error(sqlite3_bind_zeroblob(c_ptr(), idx, 0));
    }

    inline void statement::bind_reference(int idx, const blob_view & value, void (*unref)(const std::byte *))
    {
        if (auto data = value.data())
        {
            check_error(sqlite3_bind_blob(c_ptr(), idx, data, int_size(value.size()), (void (*)(void *))unref));
        }
        else
        {
            unref(data);
            check_error(sqlite3_bind_zeroblob(c_ptr(), idx, 0));
        }
    }
    
    inline void statement::bind(int idx, const value & val)
    {
        check_error(sqlite3_bind_value(c_ptr(), idx, val.c_ptr()));
    }

    

    template<>
    inline std::string_view statement::column_value<std::string_view>(int idx) const noexcept
    {
        auto first = (const char *)sqlite3_column_text(c_ptr(), idx);
        auto size = (size_t)sqlite3_column_bytes(c_ptr(), idx);
        return std::string_view(first, size);
    }

#if __cpp_char8_t >= 201811
    template<>
    inline std::u8string_view statement::column_value<std::u8string_view>(int idx) const noexcept
    {
        auto first = (const char8_t *)sqlite3_column_text(c_ptr(), idx);
        auto size = (size_t)sqlite3_column_bytes(c_ptr(), idx);
        return std::u8string_view(first, size);
    }
#endif
    
    template<>
    inline blob_view statement::column_value<blob_view>(int idx) const noexcept
    {
        auto first = (const std::byte *)sqlite3_column_blob(c_ptr(), idx);
        auto size = sqlite3_column_bytes(c_ptr(), idx);
        return blob_view(first, first + size);
    }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 14, 0)
    inline allocated_string statement::expanded_sql() const
    {
        auto ret = sqlite3_expanded_sql(c_ptr());
        if (!ret)
            throw exception(SQLITE_NOMEM);
        return allocated_string(ret);
    }
#endif

    inline void statement::check_error(int res) const
    {
        if (res != SQLITE_OK)
            throw exception(res, database());
    }

    inline std::unique_ptr<statement> statement_parser::next()
    {
        while (!_sql.empty())
        {
            auto stmt = statement::create(*_db, _sql);
            if (!stmt) //this happens for a comment or white-space
                continue;
            
            //trim whitespace after statement
            while (!_sql.empty() && isspace(_sql[0]))
                _sql.remove_prefix(1);
            
            return stmt;
        }
        return nullptr;
    }

}

#endif

