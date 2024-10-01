/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_MEMORY_IFACE_INCLUDED
#define HEADER_SQLITEPP_MEMORY_IFACE_INCLUDED

#include "config.hpp"

#include <memory>
#include <limits>

namespace thinsqlitepp
{
    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /**
     * Memory deleter that uses ::sqlite3_free
     * 
     * `#include <thinsqlitepp/memory.hpp>`
     */
    template<class T>
    class sqlite_deleter
    {
    public:
        void operator()(T * mem) const noexcept
            { sqlite3_free(const_cast<std::remove_const_t<T> *>(mem)); }
    };

    /// @cond DEPRECATED
    template<class T>
    using deleter [[deprecated]] = sqlite_deleter<T>;
    /// @endcond

    /**
     * A string allocated by SQLite
     * 
     * `#include <thinsqlitepp/memory.hpp>`
     */
    using allocated_string = std::unique_ptr<char, sqlite_deleter<char>>;

    /**
     * A byte buffer allocated by SQLite
     * 
     * `#include <thinsqlitepp/memory.hpp>`
     */
    using allocated_bytes = std::unique_ptr<std::byte, sqlite_deleter<std::byte>>;


    /// @cond PRIVATE
    inline void * sqlite_allocate_nothrow(std::size_t size) noexcept
    {
        if (size == 0)
            return sqlite3_malloc(1);

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 8, 7) 
        if constexpr (sizeof(size_t) > sizeof(sqlite3_uint64))
        {
            if (size > size_t(std::numeric_limits<sqlite3_uint64>::max()))
                return nullptr;
        }
        return sqlite3_malloc64(size);
    #else
        if (size > std::numeric_limits<int>::max())
            return nullptr;
        return sqlite3_malloc(int(size));
    #endif
    }

    inline void * sqlite_allocate(std::size_t size)
    {
        if (auto ret = sqlite_allocate_nothrow(size))
            return ret;
        throw std::bad_alloc();
    }



    /// @endcond

    /**
     * Base class that makes derived classes be allocated using SQLite
     * 
     * Allocation is performed via ::sqlite3_malloc or ::sqlite3_malloc64
     * Deallocation - via ::sqlite3_free
     * 
     * @note Unlike ::sqlite3_malloc, size 0 is legal for `operator new` overloads. 
     * It allocates a unique object just like standard @ref std::malloc does.
     * 
     * `#include <thinsqlitepp/memory.hpp>`
     */
    struct sqlite_allocated
    {
        void * operator new(std::size_t size, const std::nothrow_t &) noexcept
            { return sqlite_allocate_nothrow(size); }

        void* operator new[](std::size_t size, const std::nothrow_t &) noexcept
            { return sqlite_allocate_nothrow(size); }

        void * operator new(size_t size)
            { return sqlite_allocate(size); }

        void * operator new[](size_t size)
            { return sqlite_allocate(size); }


        void operator delete (void * ptr) noexcept
            { sqlite3_free(ptr); }
        void operator delete[](void * ptr) noexcept
            { sqlite3_free(ptr); }
    };


    /**
     * A C++ [Allocator](https://en.cppreference.com/w/cpp/named_req/Allocator)
     * that uses SQLite memory allocation functions
     */
    template<class T>
    struct sqlite_allocator 
    {
        using value_type = T;

        sqlite_allocator() = default;
 
        template<class U>
        constexpr sqlite_allocator(const sqlite_allocator <U>&) noexcept {}

        T * allocate(std::size_t n)
        {
            if (std::numeric_limits<std::size_t>::max() / sizeof(T) < n)
                throw std::bad_array_new_length();
            return sqlite_allocate(n * sizeof(T));
        }

        void deallocate(T * ptr, std::size_t /*n*/) noexcept
            { sqlite3_free(ptr); }

        template<class U>
        friend bool operator==(const sqlite_allocator<T> &, const sqlite_allocator<U> &) noexcept { return true; }
        
        template<class U>
        friend bool operator!=(const sqlite_allocator<T> &, const sqlite_allocator<U> &) noexcept { return false; }
    };

    /** @} */
}

#endif



