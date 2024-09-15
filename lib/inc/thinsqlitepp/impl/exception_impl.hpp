/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_EXCEPTION_IMPL_INCLUDED
#define HEADER_SQLITEPP_EXCEPTION_IMPL_INCLUDED

#include "exception_iface.hpp"
#include "database_iface.hpp"

#include <memory.h>


namespace thinsqlitepp
{
    inline error::error(int error_code) noexcept:
        _error_code(error_code),
        _message(sqlite3_errstr(error_code))
    {}

    inline error::error(int error_code, const database * db) noexcept:
        error(error_code)
    {
        auto db_error_code = sqlite3_extended_errcode(c_ptr(db));
        if (error_code == SQLITE_MISUSE && (db_error_code & 0x0FF) != SQLITE_MISUSE)
            return;
        
        _error_code = db_error_code;
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 12, 0)
        _system_error_code = sqlite3_system_errno(c_ptr(db));
#endif
        auto db_message = sqlite3_errmsg(c_ptr(db));
        if (db_message != _message.get())
            _message = copy_message(sqlite3_errmsg(c_ptr(db)));
    }

    inline error::message_ptr error::copy_message(const char * src) noexcept
    {
        if (!src)
            return nullptr;
        const auto len = strlen(src);
        char * const ret = (char *)sqlite3_malloc(int(len + 1));
        if (!ret)
            return nullptr;
        memcpy(ret, src, len + 1);
        return message_ptr(ret, sqlite3_free);
    }

    inline const char * exception::what() const noexcept
    {
        auto message = _error.message();
        return message ? message : "<no message available>";
    }
}

#endif
