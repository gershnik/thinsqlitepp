/*
 Copyright 2024 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_VERSION_IFACE_INCLUDED
#define HEADER_SQLITEPP_VERSION_IFACE_INCLUDED

#include "config.hpp"

#include <tuple>
#include <limits>
#include <stdexcept>

namespace thinsqlitepp
{

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Representation of SQLite version
     * 
     * This class wraps usage of SQLite version numbers which are
     * encoded as integers with the value `major*1000000 + minor*1000 + release`.
     * 
     * It provides wrappers for SQLite functions that obtain runtime SQLite version
     * information and constexpr wrapper for compile-time version info.
     */
    class sqlite_version
    {
    public:
        /// Construct an instance from an `int` encoded in SQLite format
        explicit constexpr sqlite_version(int val): _value(val) {}

        /// Construct an instance from major, minor, release parts
        static constexpr sqlite_version from_parts(unsigned major, unsigned minor, unsigned release)
        { 
            if (release > 1000)
                throw std::runtime_error("invalid release value");
            int val = release;
            if (minor > 1000000)
                throw std::runtime_error("invalid minor value");
            val += minor*1000;
            if (major > unsigned((std::numeric_limits<int>::max() - val) / 1000000))
                throw std::runtime_error("invalid major value");
            val += major*1000000;
            return sqlite_version(val); 
        }

        /**
         * Break an instance into constituent parts
         * 
         * The intended usage is
         * ```cpp
         * auto [major, minor, release] = ver.parts();
         * ```
         */
        constexpr std::tuple<unsigned, unsigned, unsigned> parts() const noexcept
        { 
            unsigned val = _value;
            unsigned major = val / 1000000;
            val -= (major * 1000000);
            unsigned minor = val / 1000;
            val -= (minor * 1000);
            unsigned release = val;
            return {major, minor, release};
        }

        /// Get the stored SQLite version value
        constexpr int value() const noexcept
            { return _value; }

        /**
         * Returns the compile time SQLite version
         * 
         * Equivalent to #SQLITE_VERSION_NUMBER
         */
        static constexpr sqlite_version compile_time() noexcept
            { return sqlite_version(SQLITE_VERSION_NUMBER); }

        /**
         * Returns the runtime SQLite version
         * 
         * Equivalent to ::sqlite3_libversion_number
         */
        static sqlite_version runtime() noexcept
            { return sqlite_version(sqlite3_libversion_number()); }

        /**
         * Returns the compile time SQLite version as a string
         * 
         * Equivalent to #SQLITE_VERSION
         */
        static constexpr const char * compile_time_str() noexcept
            { return SQLITE_VERSION; }
        /**
         * Returns the runtime SQLite version as a string
         * 
         * Equivalent to ::sqlite3_libversion
         */
        static const char * runtime_str() noexcept
            { return sqlite3_libversion(); }


        /**
         * Returns the compile time SQLite source identifier
         * 
         * Equivalent to #SQLITE_SOURCE_ID
         */
        static constexpr const char * compile_time_sourceid() noexcept
            { return SQLITE_SOURCE_ID; }
        
        /**
         * Returns the runtime SQLite source identifier
         * 
         * Equivalent to ::sqlite3_sourceid
         */
        static const char * runtime_sourceid() noexcept
            { return sqlite3_sourceid(); }

        friend constexpr bool operator==(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value == rhs._value; }
        friend constexpr bool operator!=(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value != rhs._value; }

    #if defined(DOXYGEN) || __cpp_impl_three_way_comparison >= 201907
        friend constexpr std::strong_ordering operator<=>(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value <=> rhs._value; }
    #endif
        friend constexpr bool operator<(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value < rhs._value; }
        friend constexpr bool operator<=(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value <= rhs._value; }
        friend constexpr bool operator>(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value > rhs._value; }
        friend constexpr bool operator>=(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value >= rhs._value; }
        
    private:
        int _value;
    };

    /** @} */
}

#endif
