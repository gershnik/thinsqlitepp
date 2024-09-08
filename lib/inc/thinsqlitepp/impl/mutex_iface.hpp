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
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * SQLite Mutex
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_mutex.
     * 
     * Unlike other wrappers in this library the interface of this class does not match the 
     * names of SQLite C interface. Instead is made to conform to 
     * [Lockable](https://en.cppreference.com/w/cpp/named_req/Lockable)
     * C++ standard library concept. This allows you to use it in standard lock related 
     * machinery like `std::unique_lock` or `std::lock_guard`
     * 
     * In many cases SQLite can return nullptr mutexes due to compile-time or runtime disabling
     * of synchronization.
     * While calling @ref lock(), @ref try_lock() and @ref unlock() on a nullptr `this` pointer should
     * work fine on all compilers (underlying SQLite functions all support null pointers) it is
     * technically an undefined behavior in C++ (sigh). To avoid this and deal with null mutex pointers
     * safely see @ref lock_adapter class
     * 
     * `#include <thinsqlitepp/mutex.hpp>`
     * 
     * @see lock_adapter
     */
    class mutex final : public handle<sqlite3_mutex, mutex>
    {
    public:
        /// Type of mutex to allocate for @ref alloc()
        enum type { 
            fast = SQLITE_MUTEX_FAST,
            recursive = SQLITE_MUTEX_RECURSIVE
        };

        /** 
         * Allocate a new mutex
         * 
         * Equivalent to ::sqlite3_mutex_alloc
         * 
         * Note that the interface to this function deliberately disallows
         * access to internal static SQLite mutexes. Accroding to 
         * [SQLite docs](https://www.sqlite.org/c3ref/mutex_alloc.html)
         * "[s]tatic mutexes are for internal use by SQLite only". 
         * 
         * If you do require access to one of the static mutexes you can use
         * `from(sqlite3_mutex_alloc(SQLITE_MUTEX_XXX))` manually. 
         * 
         * @param t type of the mutex to allocate
         * @returns Newly allocated mutex of `nullptr` if the SQLite implementation
         * is unable to allocate a mutex (e.g. if it does not support mutexes).
         */
        static std::unique_ptr<mutex> alloc(type t) 
            { return std::unique_ptr<mutex>(from(sqlite3_mutex_alloc(int(t)))); }

        /**
         * Lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_enter
         */
        void lock() noexcept
            { sqlite3_mutex_enter(c_ptr()); }
        /** 
         * Try to lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_try
         */
        bool try_lock() noexcept
            { return sqlite3_mutex_try(c_ptr()) == SQLITE_OK; }

        /**
         * Unlock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_leave
         */
        void unlock() noexcept
            { sqlite3_mutex_leave(c_ptr()); }
    };

    /** @} */

    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /**
     * A mutex adapter for [Lockable](https://en.cppreference.com/w/cpp/named_req/Lockable) concept
     * that works with null and non-null mutexes
     * 
     * In many cases SQLite can return nullptr mutexes due to compile-time or runtime disabling of
     * synchronization. This adapter allows you to treat null and non-null mutexes uniformly.
     * 
     * Note that this class stores the passed mutex **by reference**. If non-null it must remain
     * alive while this class is in use.
     */
    class lock_adapter
    {
    public:
        /// Adapt a @ref mutex pointer
        lock_adapter(mutex * mutex = nullptr): _mutex(mutex)
        {}
        /// Adapt a std::unique_ptr<mutex>
        lock_adapter(const std::unique_ptr<mutex> & mutex): _mutex(mutex.get())
        {}
        

        /**
         * Lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_enter
         */
        void lock() noexcept
            { sqlite3_mutex_enter(c_ptr(_mutex)); }

        /** 
         * Try to lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_try
         */
        bool try_lock() noexcept
            { return sqlite3_mutex_try(c_ptr(_mutex)) == SQLITE_OK; }
        
        /**
         * Unlock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_leave
         */
        void unlock() noexcept
            { sqlite3_mutex_leave(c_ptr(_mutex)); }
    private:
        mutex * _mutex;
    };

    /** @} */
}

#endif

