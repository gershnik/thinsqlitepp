/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_DATABASE_IMPL_INCLUDED
#define HEADER_SQLITEPP_DATABASE_IMPL_INCLUDED

#include "database_iface.hpp"
#include "context_iface.hpp"
#include "statement_iface.hpp"
#include "row_iterator.hpp"

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
#endif


namespace thinsqlitepp
{
    inline
    std::unique_ptr<database> database::open(const string_param & filename, int flags, const char * vfs)
    {
        sqlite3 * db = nullptr;
        int res = sqlite3_open_v2(filename.c_str(), &db, flags, vfs);
        std::unique_ptr<database> ret(from(db));
        if (res != SQLITE_OK)
            throw exception(res, ret);
        return ret;
    }

    template<class T>
    SQLITEPP_ENABLE_IF((
        std::is_invocable_r_v<bool, T, int, row> ||
        std::is_invocable_r_v<void, T, int, row> ||
        std::is_invocable_r_v<bool, T, row> ||
        std::is_invocable_r_v<void, T, row>),
    T) database::exec(std::string_view sql, T callback)
    {
        int statement_count = 0;
        statement_parser parser(*this, sql);
        for (auto stmt = parser.next(); stmt; stmt = parser.next(), ++statement_count)
        {
            while(stmt->step())
            {
                if constexpr (std::is_invocable_r_v<bool, T, int, row>)
                {
                    if (!callback(statement_count, row(stmt)))
                        break;
                } 
                else if constexpr (std::is_invocable_r_v<void, T, int, row>)
                {
                    callback(statement_count, row(stmt));
                }
                else if constexpr (std::is_invocable_r_v<bool, T, row>)
                {
                    if (!callback(row(stmt)))
                        break;
                }
                else
                {
                    callback(row(stmt));
                }
            }
        }
        return callback;
    }

    inline void database::exec(std::string_view sql)
    {
        exec(sql, [] (int, row) {
            return true;
        });
    }

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<bool, T, int>),
    void) database::busy_handler(T handler_ptr)
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (handler_ptr)
                this->busy_handler([] (T data, int count_invoked) noexcept -> int { return (*data)(count_invoked); }, handler_ptr);
            else
                this->busy_handler(nullptr, nullptr);
        }
        else
        {
            this->busy_handler(nullptr, nullptr);
        }
    }

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, database *, int, const char *>),
    void) database::collation_needed(T handler_ptr)
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            this->collation_needed(handler_ptr, handler_ptr ? [] (T data, sqlite3 * db, int encoding, const char * name) noexcept ->void {
                
                (*data)((database*)db, encoding, name);
                
            } : nullptr);
        }
        else
        {
            this->collation_needed(nullptr, nullptr);
        }
    }

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<bool, T>),
    void) database::commit_hook(T handler_ptr) noexcept
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (handler_ptr)
                this->commit_hook([] (T data) noexcept -> int { return (*data)(); }, handler_ptr);
            else
                this->commit_hook(nullptr, nullptr);
        }
        else
        {
            this->commit_hook(nullptr, nullptr);
        }
    }

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T>),
    void) database::rollback_hook(T handler_ptr) noexcept
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (handler_ptr)
                this->rollback_hook([] (T data) noexcept -> void { (*data)(); }, handler_ptr);
            else
                this->rollback_hook(nullptr, nullptr);
        }
        else
        {
            this->rollback_hook(nullptr, nullptr);
        }
    }

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, int, const char * , const char * , int64_t>),
    void) database::update_hook(T handler_ptr) noexcept
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (handler_ptr)
                this->update_hook([] (T data, int op,
                                      const char * db_name, const char * table, 
                                      sqlite3_int64 rowid) noexcept -> void { 
                    (*data)(op, db_name, table, rowid); 
                }, handler_ptr);
            else
                this->update_hook(nullptr, nullptr);
        }
        else
        {
            this->update_hook(nullptr, nullptr);
        }
    }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 16, 0) && defined(SQLITE_ENABLE_PREUPDATE_HOOK)
    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, database *, int, const char *, const char *, int64_t, int64_t>),
    void) database::preupdate_hook(T handler_ptr) noexcept
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (handler_ptr)
                this->preupdate_hook([] (T data, database * db, int op,
                                         const char * db_name, const char * table, 
                                         sqlite3_int64 rowid_old, sqlite3_int64 rowid_new) noexcept -> void { 
                    (*data)(db, op, db_name, table, rowid_old, rowid_new);
                }, handler_ptr);
            else
                this->preupdate_hook(nullptr, nullptr);
        }
        else
        {
            this->preupdate_hook(nullptr, nullptr);
        }
    }
#endif

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_throwing_callback<void, T, database *, const char *, int>),
    void) database::wal_hook(T handler_ptr) noexcept
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (handler_ptr)
                this->wal_hook([] (T data, database * db, const char * db_name, int num_pages) noexcept -> int { 
                    try 
                    {
                        (*data)(db, db_name, num_pages);
                        return SQLITE_OK;
                    }
                    catch(exception & ex) 
                    {
                        return ex.extended_error_code();
                    }
                    catch(std::exception &)
                    {
                        return SQLITE_ERROR;
                    }
                }, handler_ptr);
            else
                this->wal_hook(nullptr, nullptr);
        }
        else
        {
            this->wal_hook(nullptr, nullptr);
        }
    }

    template<class T>
    SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
    void) database::create_collation(const string_param & name, int encoding,
                                     T collator,
                                     int (* compare)(type_identity_t<T> collator, int lhs_len, const void * lhs_bytes, int rhs_len, const void * rhs_bytes) noexcept,
                                     void (*deleter)(type_identity_t<T> obj) noexcept)
    {
        int res = sqlite3_create_collation_v2(this->c_ptr(), name.c_str(), encoding, collator,
                                              (int(*)(void*,int,const void*,int,const void*))compare,
                                              (void (*)(void *))deleter);
        if (res != SQLITE_OK)
        {
            if (deleter)
                deleter(collator);
            throw exception(res, this);
        }
    }

    template<class T>
    SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<int, T, span<const std::byte>, span<const std::byte>>),
    void) database::create_collation(const string_param & name, int encoding, T collator,
                                     void (*deleter)(type_identity_t<T> obj) noexcept)
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (collator)
                this->create_collation(name, encoding, collator,
                                       [] (T data, int lhs_len, const void * lhs_bytes, int rhs_len, const void * rhs_bytes) noexcept -> int {
                        return (*data)(span<const std::byte>((std::byte *)lhs_bytes, lhs_len), span<const std::byte>((std::byte *)rhs_bytes, rhs_len));
                },
                                       deleter);
            else
                this->create_collation(name, encoding, nullptr, nullptr, nullptr);
        }
        else
        {
            this->create_collation(name, encoding, nullptr, nullptr, nullptr);
        }
    }


    template<class T>
    SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
    void) database::create_function(const char * name, int arg_count, int flags, T data,
                                    void (*func)(context *, int, value **) noexcept,
                                    void (*step)(context *, int, value **) noexcept,
                                    void (*last)(context*) noexcept,
                                    void (*deleter)(type_identity_t<T> obj) noexcept)
    {
        check_error(sqlite3_create_function_v2(this->c_ptr(), name, arg_count, flags, (void *)data,
                                               (void (*)(sqlite3_context *, int, sqlite3_value **))func,
                                               (void (*)(sqlite3_context *, int, sqlite3_value **))step,
                                               (void (*)(sqlite3_context *))last,
                                               (void (*)(void *))deleter));
    }

    template<class T>
    SQLITEPP_ENABLE_IF(database_detector::is_pointer_to_function<T>,
    void) database::create_function(const char * name, int arg_count, int flags, T impl,
                                    void (*deleter)(type_identity_t<T> obj) noexcept)
    {
        void (*func)(context *, int, value **) noexcept = nullptr;
        void (*step)(context *, int, value **) noexcept = nullptr;
        void (*last)(context*) noexcept = nullptr;
        void (*destroy)(T) noexcept = nullptr;
        
        if constexpr (!std::is_null_pointer_v<T>)
        {
            using handler_t = std::remove_pointer_t<T>;
            if (impl)
            {
                destroy = deleter;
                
                if constexpr (!database_detector::is_aggregate_function<std::remove_pointer_t<T>>)
                {
                    func = [] (context * ctxt, int count, value ** values) noexcept {
                        auto & impl = *ctxt->user_data<handler_t>();
                        impl(ctxt, count, values);
                    };
                }
                else
                {
                    step = [] (context * ctxt, int count, value ** values) noexcept {
                        auto & impl = *ctxt->user_data<handler_t>();
                        impl.step(ctxt, count, values);
                    };
                    last = [] (context * ctxt) noexcept {
                        auto & impl = *ctxt->user_data<handler_t>();
                        impl.last(ctxt);
                    };
                }
            }
        }
        this->create_function(name, arg_count, flags, impl, func, step, last, destroy);
    }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 25, 0)
    template<class T>
    SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
    void) database::create_window_function(const char * name, int arg_count, int flags, T data,
                                           void (*step)(context *, int, value **) noexcept,
                                           void (*last)(context*) noexcept,
                                           void (*current)(context*) noexcept,
                                           void (*inverse)(context *, int, value **) noexcept,
                                           void (*deleter)(type_identity_t<T> obj) noexcept)
    {
        check_error(sqlite3_create_window_function(this->c_ptr(), name, arg_count, flags, data,
                                                   (void (*)(sqlite3_context *, int, sqlite3_value **))step,
                                                   (void (*)(sqlite3_context *))last,
                                                   (void (*)(sqlite3_context *))current,
                                                   (void (*)(sqlite3_context *, int, sqlite3_value **))inverse,
                                                   (void (*)(void *))deleter));
    }

    template<class T>
    SQLITEPP_ENABLE_IF(database_detector::is_pointer_to_window_function<T>,
    void) database::create_window_function(const char * name, int arg_count, int flags, T impl, void (*deleter)(type_identity_t<T> obj) noexcept)
    {
        void (*step)(context *, int, value **) noexcept = nullptr;
        void (*last)(context*) noexcept = nullptr;
        void (*current)(context*) noexcept = nullptr;
        void (*inverse)(context *, int, value **) noexcept = nullptr;
        void (*destroy)(T) noexcept = nullptr;
        
        if constexpr (!std::is_null_pointer_v<T>)
        {
            using handler_t = std::remove_pointer_t<T>;
            if (impl)
            {
                destroy = deleter;
                
                step = [] (context * ctxt, int count, value ** values) noexcept {
                    auto & impl = *ctxt->user_data<handler_t>();
                    impl.step(ctxt, count, values);
                };
                last = [] (context * ctxt) noexcept {
                    auto & impl = *ctxt->user_data<handler_t>();
                    impl.last(ctxt);
                };
                current = [] (context * ctxt) noexcept {
                    auto & impl = *ctxt->user_data<handler_t>();
                    impl.current(ctxt);
                };
                inverse = [] (context * ctxt, int count, value ** values) noexcept {
                    auto & impl = *ctxt->user_data<handler_t>();
                    impl.inverse(ctxt, count, values);
                };
                
            }
        }
        this->create_window_function(name, arg_count, flags, impl, step, last, current, inverse, destroy);
    }
#endif

    inline std::optional<bool> database::readonly(const string_param & db_name) const noexcept
    {
        int ret = sqlite3_db_readonly(c_ptr(), db_name.c_str());
        if (ret == -1)
            return std::nullopt;
        return bool(ret);
    }

    inline struct database::status database::status(int op, bool reset) const
    {
        struct status ret;
        check_error(sqlite3_db_status(c_ptr(), op, &ret.current, &ret.high, reset));
        return ret;
    }
        
    inline void database::load_extension(const string_param & file, const string_param & proc)
    {
        char * errmessage = nullptr;
        int res = call_sqlite3_load_extension(c_ptr(), file.c_str(), proc.c_str(), &errmessage);
        error::message_ptr errmessage_ptr(errmessage, sqlite3_free);
        if (res != SQLITE_OK)
            throw exception(res, std::move(errmessage_ptr));
    }

    template<class T>
    SQLITEPP_ENABLE_IF((std::is_null_pointer_v<T> ||
        (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<bool, std::remove_pointer_t<T>>)),
    void) database::progress_handler(int step_count, T func) const noexcept
    {
        if constexpr (!std::is_null_pointer_v<T>)
        {
            if (func)
                this->progress_handler(step_count, [] (T data) noexcept -> int { return (*data)(); }, func);
            else
                this->progress_handler(step_count, nullptr, nullptr);
        }
        else
        {
            this->progress_handler(step_count, nullptr, nullptr);
        }
    }

    inline database::column_metadata database::table_column_metadata(const string_param & db_name,
                                                                     const string_param & table_name,
                                                                     const string_param & column_name) const
    {
        column_metadata ret;
        int not_null, primary_key, auto_increment;
        check_error(sqlite3_table_column_metadata(c_ptr(), db_name.c_str(), table_name.c_str(), column_name.c_str(),
                                                  &ret.data_type, &ret.collation_sequence, &not_null, &primary_key, &auto_increment));
        ret.not_null = not_null;
        ret.primary_key = primary_key;
        ret.auto_increment = auto_increment;
        return ret;
    }

    inline std::unique_ptr<blob> database::open_blob(const string_param & dbname, 
                                                     const string_param & table,
                                                     const string_param & column,
                                                     int64_t rowid,
                                                     bool writable)
    {
        sqlite3_blob * blob_ptr = nullptr;
        int res = sqlite3_blob_open(c_ptr(), dbname.c_str(), table.c_str(), column.c_str(), rowid, writable, &blob_ptr);
        std::unique_ptr<blob> ret(blob::from(blob_ptr));
        if (res != SQLITE_OK)
            throw exception(res, this);
        return ret;
    }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) && THINSQLITEPP_ENABLE_EXPIREMENTAL

    inline std::unique_ptr<snapshot> database::get_snapshot(const string_param & schema)
    {
        sqlite3_snapshot * snapshot_ptr = nullptr;
        int res = sqlite3_snapshot_get(c_ptr(), schema.c_str(), &snapshot_ptr);
        std::unique_ptr<snapshot> ret(snapshot::from(snapshot_ptr));
        if (res != SQLITE_OK)
            throw exception(res, this);
        return ret;
    }
#endif


#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 39, 0)
    inline std::pair<allocated_bytes, size_t>  database::serialize(const string_param & schema_name)
    {
        sqlite3_int64 size;
        auto ptr = (std::byte *)sqlite3_serialize(c_ptr(), schema_name.c_str(), &size, 0);
        if (!ptr)
            throw exception(SQLITE_NOMEM);
        return {std::unique_ptr<std::byte, sqlite_deleter<std::byte>>{ptr}, size_t(size)};
    }


    inline span<std::byte> database::serialize_reference(const string_param & schema_name) noexcept
    {
        sqlite3_int64 size;
        auto ptr = (std::byte *)sqlite3_serialize(c_ptr(), schema_name.c_str(), &size, SQLITE_SERIALIZE_NOCOPY);
        if (!ptr)
            size = 0;
        return {ptr, size_t(size)};
    }

    inline void database::deserialize(const string_param & schema_name, 
                                      allocated_bytes buf, 
                                      size_t size, 
                                      size_t buf_size,
                                      unsigned flags)
    {
        int res = sqlite3_deserialize(c_ptr(), 
                                      schema_name.c_str(), 
                                      (unsigned char *)buf.get(), 
                                      int64_size(size), 
                                      int64_size(buf_size), 
                                      flags | SQLITE_DESERIALIZE_FREEONCLOSE);
        buf.release();
        check_error(res);
    }

#endif

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 16, 0) && defined(SQLITE_ENABLE_PREUPDATE_HOOK)
    inline value * database::preupdate_old(int column_idx)
    {
        sqlite3_value * ptr;
        check_error(sqlite3_preupdate_old(c_ptr(), column_idx, &ptr));
        return value::from(ptr);
    }

    inline value * database::preupdate_new(int column_idx)
    {
        sqlite3_value * ptr;
        check_error(sqlite3_preupdate_new(c_ptr(), column_idx, &ptr));
        return value::from(ptr);
    }
#endif

    inline std::pair<int, int> database::checkpoint(const string_param & db_name, int mode)
    {
        std::pair<int, int> ret;
        check_error(sqlite3_wal_checkpoint_v2(c_ptr(), db_name.c_str(), mode, &ret.first, &ret.second));
        return ret;
    }
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#endif

