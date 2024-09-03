/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_STRING_PARAM_IFACE_INCLUDED
#define HEADER_SQLITEPP_STRING_PARAM_IFACE_INCLUDED

#include "config.hpp"

#include <string>
#include <memory>

namespace thinsqlitepp
{

    /**
     * A reference to a null terminated string
     * 
     * This class allows passing either a `const T *` or `std::basic_string<T>`
     * to a function that internally needs a null terminated const T *
     * 
     * Note that this class has _reference_ semantics. The string it refers to must
     * be kept alive as long as the instance of this class is alive.
     * 
     * @tparam T character type
     */
    template<class T>
    class basic_string_param
    {
    public:
        /// Construct an instance from a raw pointer
        basic_string_param(const T * str) noexcept : _str(str)
        {}
        /// Construct an instance from std::basic_string<T>::c_str()
        basic_string_param(const std::basic_string<T> & str) noexcept : _str(str.c_str())
        {}

        /// Returns the stored pointer
        const T * c_str() const noexcept
            { return _str; }
    private:
        const T * _str;
    };

    /// Convenience typedef
    using string_param = basic_string_param<char>;

#if __cpp_char8_t >= 201811

    /// Convenience typedef. Only available if you compiler/library supports char8_t
    using u8string_param = basic_string_param<char8_t>;
    
#endif

}

#endif



