/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_VALUE_IFACE_INCLUDED
#define HEADER_SQLITEPP_VALUE_IFACE_INCLUDED

#include "handle.hpp"
#include "span.hpp"
#include "exception_iface.hpp"

#include <memory>
#include <string_view>

namespace thinsqlitepp
{
    class value final : public handle<sqlite3_value, value>
    {
    public:
#if SQLITE_VERSION_NUMBER >= 3018011
        static std::unique_ptr<value> dup(const value * src)
        {
            auto ret = sqlite3_value_dup(src->c_ptr());
            if (!ret && src)
                throw exception(SQLITE_NOMEM);
            return std::unique_ptr<value>((value *)ret);
        }
        
        ~value() noexcept
            { sqlite3_value_free(c_ptr()); }
#endif
        
        template<class T>
        std::enable_if_t<
            std::is_same_v<T, int> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string_view> ||
            std::is_same_v<T, blob_view>,
        T> get() const noexcept;
        
        int type() const noexcept
            { return sqlite3_value_type(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= 3009000
        unsigned subtype()const noexcept
            { return sqlite3_value_subtype(c_ptr()); }
#endif
        
        int numeric_type() const noexcept
            { return sqlite3_value_numeric_type(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= 3022000
        bool nochange() const noexcept
            { return sqlite3_value_nochange(c_ptr()); }
#endif
        
#if SQLITE_VERSION_NUMBER >= 3028000
        bool frombind() const noexcept
            { return sqlite3_value_frombind(c_ptr()); }
#endif

    };

    template<>
    inline int value::get<int>() const noexcept
        { return sqlite3_value_int(c_ptr()); }

    template<>
    inline int64_t value::get<int64_t>() const noexcept
        { return sqlite3_value_int64(c_ptr()); }

    template<>
    inline std::string_view value::get<std::string_view>() const noexcept
    {
        auto first = (const char *)sqlite3_value_text(c_ptr());
        auto size = (size_t)sqlite3_value_bytes(c_ptr());
        return std::string_view(first, size);
    }

    template<>
    inline double value::get<double>() const noexcept
        { return sqlite3_value_double(c_ptr()); }

    template<>
    inline blob_view value::get<blob_view>() const noexcept
    {
        auto first = (const std::byte *)sqlite3_value_blob(c_ptr());
        auto size = sqlite3_value_bytes(c_ptr());
        return blob_view(first, first + size);
    }
}


#endif

