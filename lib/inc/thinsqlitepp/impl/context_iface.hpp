/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_CONTEXT_IFACE_INCLUDED
#define HEADER_SQLITEPP_CONTEXT_IFACE_INCLUDED

#include "handle.hpp"
#include "value_iface.hpp"

#include <memory>

namespace thinsqlitepp
{
    class database;

    class context final : public handle<sqlite3_context, context>
    {
    public:
        ~context() noexcept = delete;
        
        void * aggregate_context(int size) noexcept
            { return sqlite3_aggregate_context(c_ptr(), size); }
        
        class database & database() const noexcept
            { return *(class database *)sqlite3_context_db_handle(c_ptr()); }
        
        void error(const std::string_view & value) noexcept
            { sqlite3_result_error(c_ptr(), value.size() ? &value[0] : nullptr, int(value.size())); }
        
        void error(int error_code) noexcept
            { sqlite3_result_error_code(c_ptr(), error_code); }
        
        void error_nomem() noexcept
            { sqlite3_result_error_nomem(c_ptr()); }
        
        template<class T>
        T * get_auxdata(int arg) const noexcept
            { return (T *)sqlite3_get_auxdata(this->c_ptr(), arg); }
        
        template<class T>
        void set_auxdata(int arg, T * data, void (*destroy)(T*)noexcept) noexcept
            { sqlite3_set_auxdata(this->c_ptr(), arg, data, (void(*)(void*))destroy); };
                
        
        void result(std::nullptr_t) noexcept
            { sqlite3_result_null(c_ptr()); }
        void result(int value) noexcept
            { sqlite3_result_int(c_ptr(), value); }
        void result(int64_t value) noexcept
            { sqlite3_result_int64(c_ptr(), value); }
        void result(const std::string_view & value, bool copy = false) noexcept
        {
            sqlite3_result_text(c_ptr(),
                                value.size() ? &value[0] : nullptr,
                                int(value.size()),
                                copy ? SQLITE_TRANSIENT : SQLITE_STATIC);
        }
        void result(double value) noexcept
            { sqlite3_result_double(c_ptr(), value); }
        void result(const blob_view & value, bool copy = false) noexcept
        {
            sqlite3_result_blob(c_ptr(),
                                value.size() ? &value[0] : nullptr,
                                int(value.size()),
                                copy ? SQLITE_TRANSIENT : SQLITE_STATIC);
        }
        void result(const zero_blob & value) noexcept
            { sqlite3_result_zeroblob(c_ptr(), int(value.size())); }
        
        template<class T>
        void result(T * ptr, const char * type, void(*destroy)(T*)) noexcept
            { sqlite3_result_pointer(this->c_ptr(), ptr, type, (void(*)(void*))destroy); }
        
        template<class T>
        void result(std::unique_ptr<T> ptr) noexcept
            { this->result(ptr.release(), typeid(T).name(), [](T * p) { delete p;}); }
        
        void result(const value & val) noexcept
            { sqlite3_result_value(c_ptr(), val.c_ptr()); }
      
#if SQLITE_VERSION_NUMBER >= 3009000
        void result_subtype(unsigned value) noexcept
            { sqlite3_result_subtype(c_ptr(), value); }
#endif
        
        template<class T>
        T * user_data() noexcept
            { return (T *)sqlite3_user_data(this->c_ptr()); }
    };
    
}


#endif
