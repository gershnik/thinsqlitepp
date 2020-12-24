/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_MEMORY_INCLUDED
#define HEADER_SQLITEPP_MEMORY_INCLUDED

#include "config.hpp"

#include <memory>

namespace thinsqlitepp
{

    template<class T>
    class deleter
    {
    public:
        void operator()(T * mem) const noexcept
            { sqlite3_free(const_cast<std::remove_const_t<T> *>(mem)); }
    };

    using allocated_string = std::unique_ptr<char, deleter<char>>;

}

#endif



