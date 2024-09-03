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
    /**
     * Base functionality for all [fake wrapper classes](https://github.com/gershnik/thinsqlitepp#fake-classes)
     * 
     * This is a [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) base class
     * that implements functionality common to all fake wrapper classes.
     * 
     * @tparam T the underlying SQLite type
     * @tparam Derived the derived class for CRTP casts
     */
    template<class T, class Derived>
    class handle
    {
    public:
        /// Deleting fake pointer does nothing
        void operator delete(void *) noexcept
        {}
        
        /// You cannot construct it
        handle() = delete;
        /// You cannot copy (or move) it
        handle(const handle &) = delete;
        /// You cannot assign it
        handle & operator=(const handle &) = delete;

        /// Create fake pointer from the underlying SQLite one
        static Derived * from(T * obj) noexcept
            { return (Derived *)obj; }

        /// Access the real underlying SQLite type
        T * c_ptr() const noexcept
            { return (T *)this; }
        
        /// Access the real underlying SQLite type
        friend T * c_ptr(const handle<T, Derived> & obj) noexcept
            { return (T *)&obj; }

        /// Access the real underlying SQLite type
        friend T * c_ptr(const handle<T, Derived> * obj) noexcept
            { return (T *)obj; }
        
    protected:
        ~handle() noexcept
        {}
    };

}

#endif

