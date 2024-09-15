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
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Dynamically Typed Value Object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_value. 
     * 
     * `#include <thinsqlitepp/value.hpp>`
     */
    class value final : public handle<sqlite3_value, value>
    {
    public:
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 18, 11)
        /**
         * Creates a new value by copying an original one
         * 
         * Equivalent to ::sqlite3_value_dup
         * 
         * @since SQLite 3.18
         * 
         * @param src Original @ref value. Can be nullptr
         * @returns A new @ref value object which is a copy of the original
         * or nullptr if the original is nullptr
         */
        static std::unique_ptr<value> dup(const value * src)
        {
            auto ret = sqlite3_value_dup(src->c_ptr());
            if (!ret && src)
                throw exception(SQLITE_NOMEM);
            return std::unique_ptr<value>((value *)ret);
        }

        /// @overload
        static std::unique_ptr<value> dup(const std::unique_ptr<value> & src) 
            { return dup(src.get()); }
        
        /**  
         * Equivalent to ::sqlite3_value_free
         * 
         * @since SQLite 3.18
         */
        ~value() noexcept
            { sqlite3_value_free(c_ptr()); }
#endif

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
        
        /**
         * Obtain value's content 
         * 
         * Wraps @ref sqlite3_value_ function family. Unlike the C API you specify the
         * desired type via T template parameter
         * 
         * @tparam T Desired output type. Must be one of:
         * - int
         * - int64_t
         * - double
         * - std::string_view
         * - std::u8string_view (if `char8_t` is supported by your compiler/library)
         * - blob_view
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_column_type<T>,
        T) get() const noexcept;
        
        /**
         * Default datatype of the value
         * 
         * Equivalent to ::sqlite3_value_type
         * 
         * @returns One of the SQLite [datatype constants](https://www.sqlite.org/c3ref/c_blob.html)
         */
        int type() const noexcept
            { return sqlite3_value_type(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 9, 0)
        /**
         * Subtype of the value
         * 
         * Equivalent to ::sqlite3_value_subtype
         * 
         * @since SQLite 3.9
         */
        unsigned subtype()const noexcept
            { return sqlite3_value_subtype(c_ptr()); }
#endif
        
        /**
         * Best numeric datatype of the value
         * 
         * Equivalent to ::sqlite3_value_numeric_type
         * 
         * @returns One of the SQLite [datatype constants](https://www.sqlite.org/c3ref/c_blob.html)
         */
        int numeric_type() const noexcept
            { return sqlite3_value_numeric_type(c_ptr()); }
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 22, 0)
        /**
         * Whether the column is unchanged in an UPDATE against a virtual table.
         * 
         * Equivalent to ::sqlite3_value_nochange
         * 
         * @since SQLite 3.22
         */
        bool nochange() const noexcept
            { return sqlite3_value_nochange(c_ptr()); }
#endif
        
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 28, 0)
        /**
         * Whether if value originated from a [bound parameter](https://www.sqlite.org/lang_expr.html#varparam)
         * 
         * Equivalent to ::sqlite3_value_frombind
         * 
         * @since SQLite 3.28
         */
        bool frombind() const noexcept
            { return sqlite3_value_frombind(c_ptr()); }
#endif

    };

    /** @} */

    /// @cond PRIVATE

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

#if __cpp_char8_t >= 201811
    template<>
    inline std::u8string_view value::get<std::u8string_view>() const noexcept
    {
        auto first = (const char8_t *)sqlite3_value_text(c_ptr());
        auto size = (size_t)sqlite3_value_bytes(c_ptr());
        return std::u8string_view(first, size);
    }
#endif

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

    /// @endcond
}


#endif

