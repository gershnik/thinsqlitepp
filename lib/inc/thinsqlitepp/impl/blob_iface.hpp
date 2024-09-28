/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_BLOB_IFACE_INCLUDED
#define HEADER_SQLITEPP_BLOB_IFACE_INCLUDED

#include "handle.hpp"
#include "database_iface.hpp"
#include "string_param.hpp"
#include "span.hpp"

namespace thinsqlitepp
{
    class database;

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Access blob as a byte stream
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_blob.
     * 
     * `#include <thinsqlitepp/blob.hpp>`
     * 
     */
    class blob final : public handle<sqlite3_blob, blob>
    {
    public:
        /**
         * Open a blob
         * 
         * Equivalent to ::sqlite3_blob_open
         */
        static std::unique_ptr<blob> open(const database & db, 
                                          const string_param & dbname, 
                                          const string_param & table,
                                          const string_param & column,
                                          int64_t rowid,
                                          bool writable)
        {
            sqlite3_blob * blob_ptr = nullptr;
            int res = sqlite3_blob_open(db.c_ptr(), dbname.c_str(), table.c_str(), column.c_str(), rowid, writable, &blob_ptr);
            std::unique_ptr<blob> ret(from(blob_ptr));
            if (res != SQLITE_OK)
                throw exception(res, db);
            return ret;
        }

        /// Equivalent to ::sqlite3_blob_close
        ~blob() noexcept
            { sqlite3_blob_close(c_ptr()); }


        /**
         * Move the object to a new row
         * 
         * Equivalent to ::sqlite3_blob_reopen
         */
        void reopen(int64_t rowid)
        {
            int res = sqlite3_blob_reopen(c_ptr(), rowid);
            if (res != SQLITE_OK)
                throw exception(res); //we do not know the db here, unfortunately
        }

        /**
         * Returns the size of the blob
         * 
         * Equivalent to ::sqlite3_blob_bytes
         */
        size_t bytes() const noexcept
        { 
            int ret = sqlite3_blob_bytes(c_ptr());
            return ret >= 0 ? ret : 0;
        } 

        /**
         * Read data from the blob
         * 
         * Equivalent to ::sqlite3_blob_read
         */
        void read(size_t offset, span<std::byte> dest) const
        {
            int res = sqlite3_blob_read(c_ptr(), dest.data(), int_size(dest.size()), int_size(offset));
            if (res != SQLITE_OK)
                throw exception(res); //we do not know the db here, unfortunately
        }

    #if __cpp_lib_ranges >= 201911L

        /**
         * Read data from the blob
         * 
         * This overload is only available in C++20 and allows you 
         * to read into any compatible range.
         * 
         * Equivalent to ::sqlite3_blob_read
         */
        template<std::ranges::contiguous_range R>
        requires(std::is_trivially_copyable_v<std::ranges::range_value_t<R>> &&
                 !std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<R>>>)
        void read(size_t offset, R & range) const
        {
            using value_type = std::remove_reference_t<std::ranges::range_reference_t<R>>;
            auto data = std::data(range);
            auto size = std::size(range);
            if (size > std::numeric_limits<int>::max() / sizeof(value_type))
                throw exception(SQLITE_TOOBIG);
            int res = sqlite3_blob_read(c_ptr(), data, size * sizeof(value_type), int_size(offset));
            if (res != SQLITE_OK)
                throw exception(res); //we do not know the db here, unfortunately
        }

    #endif

        /**
         * Write data to the blob
         * 
         * This function may only modify the contents of the blob; it is not possible to increase 
         * the size of a blob using this API
         * 
         * Equivalent to ::sqlite3_blob_write
         */
        void write(size_t offset, span<const std::byte> src)
        {
            int res = sqlite3_blob_write(c_ptr(), src.data(), int_size(src.size()), int_size(offset));
            if (res != SQLITE_OK)
                throw exception(res); //we do not know the db here, unfortunately
        }

        /// @overload
        void write(size_t offset, span<std::byte> src)
            { write(offset, span<const std::byte>(src)); }

    #if __cpp_lib_ranges >= 201911L

        /**
         * Write data to the blob
         * 
         * This function may only modify the contents of the blob; it is not possible to increase 
         * the size of a blob using this API
         * 
         * This overload is only available in C++20 and allows you 
         * to write from any compatible range.
         * 
         * Equivalent to ::sqlite3_blob_write
         */
        template<std::ranges::contiguous_range R>
        requires(std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
        void write(size_t offset, R range) const
        {
            using value_type = std::remove_reference_t<std::ranges::range_reference_t<R>>;
            auto data = std::data(range);
            auto size = std::size(range);
            if (size > std::numeric_limits<int>::max() / sizeof(value_type))
                throw exception(SQLITE_TOOBIG);
            int res = sqlite3_blob_write(c_ptr(), data(), size * sizeof(value_type), int_size(offset));
            if (res != SQLITE_OK)
                throw exception(res); //we do not know the db here, unfortunately
        }

    #endif
    };


    /** @} */
}


#endif
