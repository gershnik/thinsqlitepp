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

    template<class T>
    class basic_string_param
    {
    public:
        basic_string_param(const T * str) noexcept : _str(str)
        {}
        basic_string_param(const std::basic_string<T> & str) noexcept : _str(str.c_str())
        {}

        const T * c_str() const noexcept
            { return _str; }
    private:
        const T * _str;
    };

    using string_param = basic_string_param<char>;

}

#endif



