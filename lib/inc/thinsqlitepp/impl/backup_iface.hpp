/*
 Copyright 2024 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_VACKUP_IFACE_INCLUDED
#define HEADER_SQLITEPP_VACKUP_IFACE_INCLUDED

#include "handle.hpp"
#include "database_iface.hpp"

namespace thinsqlitepp
{
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Online backup object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_backup.
     * 
     * `#include <thinsqlitepp/backup.hpp>`
     * 
     */
    class backup final : public handle<sqlite3_backup, backup>
    {
    public:
        /**
         * Initialize the backup
         * 
         * Equivalent to ::sqlite3_backup_init
         */
        static std::unique_ptr<backup> init(database & dst, const string_param & dest_dbname, 
                                            database & src, const string_param & src_dbname)
        {
            std::unique_ptr<backup> ret(backup::from(
                sqlite3_backup_init(dst.c_ptr(), dest_dbname.c_str(), src.c_ptr(), src_dbname.c_str())));
            if (!ret)
                throw exception(sqlite3_errcode(dst.c_ptr()), dst);
            return ret;
        }

        /// Equivalent to ::sqlite3_backup_finish
        ~backup() noexcept
            { sqlite3_backup_finish(c_ptr()); }


        /// Result of a backup step
        enum step_result 
        {
            done,       ///< Backup finished (#SQLITE_DONE)
            success,    ///< Backup step succeeded (#SQLITE_OK)
            busy,       ///< Database is busy, retry later (#SQLITE_BUSY)
            locked      ///< Source database is being written, retry later (#SQLITE_LOCKED)
        };

        /**
         * Copy up to @p page_count pages between the source and destination databases
         * 
         * Equivalent to ::sqlite3_backup_step
         */
        step_result step(int page_count)
        {
            int ret = sqlite3_backup_step(c_ptr(), page_count);
            switch(ret)
            {
            case SQLITE_BUSY: return busy;
            case SQLITE_LOCKED: return locked;
            case SQLITE_DONE: return done;
            case SQLITE_OK: return success;
            }
            throw exception(ret);
        }

        /**
         * Returns the number of pages still to be backed up after last @ref step()
         * 
         * Equivalent to ::sqlite3_backup_remaining
         */
        int remaining() const noexcept
            { return sqlite3_backup_remaining(c_ptr()); }


        /**
         * Returns the total number of pages in the source database after last @ref step()
         * 
         * Equivalent to ::sqlite3_backup_pagecount
         */
        int pagecount() const noexcept
            { return sqlite3_backup_pagecount(c_ptr()); }
    };


    /** @} */
}


#endif
