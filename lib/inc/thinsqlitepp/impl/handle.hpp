/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_HANDLE_INCLUDED
#define HEADER_SQLITEPP_HANDLE_INCLUDED

#include "config.hpp"

namespace thinsqlitepp
{
    template<class T, class Derived>
    class handle
    {
    public:
        void operator delete(void *) noexcept
        {}
        
        handle() = delete;
        handle(const handle &) = delete;
        handle & operator=(const handle &) = delete;
        
        T * c_ptr() const noexcept
            { return (T *)this; }
        
        static Derived * from(T * obj) noexcept
            { return (Derived *)obj; }
        
        
        static T * c_ptr(const handle<T, Derived> * obj) noexcept
            { return (T *)obj; }

        friend T * c_ptr(const handle<T, Derived> & obj) noexcept
            { return (T *)&obj; }
        
    protected:
        ~handle() noexcept
        {}
    };

    template<class T, class Derived>
    T * c_ptr(const handle<T, Derived> * obj) noexcept
        { return handle<T, Derived>::c_ptr(obj); }
    
}

#endif

