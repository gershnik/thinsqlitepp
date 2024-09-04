/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_MUTEX_IFACE_INCLUDED
#define HEADER_SQLITEPP_MUTEX_IFACE_INCLUDED

#include "handle.hpp"

namespace thinsqlitepp
{
    /**
     * SQLite Mutex
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_mutex.
     * 
     * `#include <thinsqlitepp/mutex.hpp>`
     * 
     */
    class mutex final : public handle<sqlite3_mutex, mutex>
    {
    public:
        void lock() noexcept
            { sqlite3_mutex_enter(c_ptr()); }
        bool try_lock() noexcept
            { return sqlite3_mutex_try(c_ptr()) == SQLITE_OK; }
        void unlock() noexcept
            { sqlite3_mutex_leave(c_ptr()); }
    };

    class lock_adapter
    {
    public:
        lock_adapter(mutex * mutex): _mutex(mutex)
        {}
        
        void lock() noexcept
        {
            if (_mutex)
                _mutex->lock();
        }
        bool try_lock() noexcept
        {
            return _mutex ? _mutex->try_lock() : true;
        }
        void unlock() noexcept
        {
            if (_mutex)
                _mutex->unlock();
        }
    private:
        mutex * _mutex;
    };
}

#endif

