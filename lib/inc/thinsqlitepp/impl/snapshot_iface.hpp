/*
 Copyright 2024 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_SNAPSHOT_IFACE_INCLUDED
#define HEADER_SQLITEPP_SNAPSHOT_IFACE_INCLUDED

#include "handle.hpp"
#include "meta.hpp"

namespace thinsqlitepp
{
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) && THINSQLITEPP_ENABLE_EXPIREMENTAL


    /**
     * A database snapshot
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_snapshot.
     * 
     * Use database::get_snapshot to create snapshot objects.
     * 
     * @since SQLite 3.10
     * 
     * Requires THINSQLITEPP_ENABLE_EXPIREMENTAL macro to be defined to 1 as the underlying SQLite
     * feature is experimental.
     * 
     * `#include <thinsqlitepp/snapshot.hpp>`
     * 
     */
    class snapshot final : public handle<sqlite3_snapshot, snapshot> 
    {
    public:
        /// Equivalent to ::sqlite3_snapshot_free
        ~snapshot() noexcept
            { sqlite3_snapshot_free(c_ptr()); }


        /**
         * Compare the ages of two snapshots
         * 
         * Equivalent to ::sqlite3_snapshot_cmp
         */
        friend int compare(const snapshot & lhs, const snapshot & rhs) noexcept
            { return sqlite3_snapshot_cmp(lhs.c_ptr(), rhs.c_ptr()); }

        friend bool operator==(const snapshot & lhs, const snapshot & rhs) noexcept
            { return compare(lhs, rhs) == 0; }
        friend bool operator!=(const snapshot & lhs, const snapshot & rhs) noexcept
            { return compare(lhs, rhs) != 0; }

    #if defined(DOXYGEN) || __cpp_impl_three_way_comparison >= 201907
        /// @{ 
        /// @name C++20 comparison operator. Not present in C++17
        friend std::strong_ordering operator<=>(const snapshot & lhs, const snapshot & rhs) noexcept
            { return strong_ordering_from_int(compare(lhs, rhs)); }
        /// @}
    #endif
    #if defined(DOXYGEN) || __cpp_impl_three_way_comparison < 201907
        /// @{ 
        /// @name C++17 comparison operators. Not present in C++20
        friend bool operator<(const snapshot & lhs, const snapshot & rhs) noexcept
            { return compare(lhs, rhs) < 0; }
        friend bool operator<=(const snapshot & lhs, const snapshot & rhs) noexcept
            { return compare(lhs, rhs) <= 0; }
        friend bool operator>(const snapshot & lhs, const snapshot & rhs) noexcept
            { return compare(lhs, rhs) > 0; }
        friend bool operator>=(const snapshot & lhs, const snapshot & rhs) noexcept
            { return compare(lhs, rhs) >= 0; }
        /// @} 
    #endif
    };

#endif

    /** @} */
}

#endif