/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_CONTEXT_IFACE_INCLUDED
#define HEADER_SQLITEPP_CONTEXT_IFACE_INCLUDED

#include "handle.hpp"
#include "value_iface.hpp"
#include "meta.hpp"

#include <memory>

namespace thinsqlitepp
{
    class database;

    /**
     * SQL Function Context Object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_context.
     * 
     * `#include <thinsqlitepp/context.hpp>`
     * 
     */
    class context final : public handle<sqlite3_context, context>
    {
    public:
        /// Contexts are never destroyed by user code
        ~context() noexcept = delete;
        
        /**
         * Allocate memory for aggregate function context
         * 
         * Equivalent to ::sqlite3_aggregate_context
         */
        void * aggregate_context(int size) noexcept
            { return sqlite3_aggregate_context(c_ptr(), size); }
        
        /**
         * Retrieve database connection for the context
         * 
         * Equivalent to ::sqlite3_aggregate_context
         */
        class database & database() const noexcept
            { return *(class database *)sqlite3_context_db_handle(c_ptr()); }
        
        /**
         * Cause the implemented SQL function to throw an SQL exception
         * 
         * Equivalent to ::sqlite3_result_error
         */
        void error(const std::string_view & value) noexcept
            { sqlite3_result_error(c_ptr(), value.size() ? &value[0] : "", int(value.size())); }
    #if __cpp_char8_t >= 201811
        /// @overload
        void error(const std::u8string_view & value) noexcept
            { sqlite3_result_error(c_ptr(), value.size() ? (const char *)&value[0] : "", int(value.size())); }
    #endif

        /**
         * Changes the error code returned by function evaluation.
         * 
         * Equivalent to ::sqlite3_result_error_code
         * 
         * This call is useful to propagate SQLite error codes out of function
         * evaluation. Note that calling @ref error(const std::string_view &) 
         * after this call will reset error code to #SQLITE_ERROR
         */
        void error(int error_code) noexcept
            { sqlite3_result_error_code(c_ptr(), error_code); }
        
        /**
         * Causes the implemented SQL function to throw an SQL exception indicating that a memory allocation failed.
         * 
         * Equivalent to ::sqlite3_result_error_nomem
         */
        void error_nomem() noexcept
            { sqlite3_result_error_nomem(c_ptr()); }

        /**
         * Causes the implemented SQL function to throw an SQL exception indicating that a string or BLOB is too long to represent.
         * 
         * Equivalent to ::sqlite3_result_error_toobig
         */
        void error_toobig() noexcept
            { sqlite3_result_error_toobig(c_ptr()); }
        
        /**
         * Return NULL from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_null
         */
        void result(std::nullptr_t) noexcept
            { sqlite3_result_null(c_ptr()); }

        /**
         * Return an int from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_int
         */
        void result(int value) noexcept
            { sqlite3_result_int(c_ptr(), value); }

        /**
         * Return an int64_t from implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_int64
         */
        void result(int64_t value) noexcept
            { sqlite3_result_int64(c_ptr(), value); }

        /**
         * Return a double from implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_double
         */
        void result(double value) noexcept
            { sqlite3_result_double(c_ptr(), value); }

        /**
         * Return a string by value from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_text(..., SQLITE_TRANSIENT)
         * 
         * The string content is copied and does not need to persist.
         */
        void result(const std::string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), data, int(value.size()), SQLITE_TRANSIENT);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #if __cpp_char8_t >= 201811
        /// @overload
        void result(const std::u8string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), (const char *)data, int(value.size()), SQLITE_TRANSIENT);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #endif
        /**
         * Return a string by reference from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_text(..., SQLITE_STATIC)
         * 
         * The string content is used **by reference** and so must be present in memory
         * as long as SQLite library remain used.
         */
        void result_reference(const std::string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), data, int(value.size()), SQLITE_STATIC);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #if __cpp_char8_t >= 201811
        /// @overload
        void result_reference(const std::u8string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), (const char *)data, int(value.size()), SQLITE_STATIC);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #endif
        /**
         * Return a string by reference from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_text(..., destructor)
         * 
         * The string content is used **by reference**. 
         * 
         * @param value reference to string to return
         * @param unref called when the reference is no longer needed.
         * Its argument will be the pointer returned from `value.data()`
         */
        void result_reference(const std::string_view & value, void (*unref)(const char *)) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
            {
                sqlite3_result_text(c_ptr(), data, int(value.size()), (void (*)(void*))unref);
            }
            else 
            {
                unref(nullptr);
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
            }
        }

    #if __cpp_char8_t >= 201811
        /// @overload
        void result_reference(const std::u8string_view & value, void (*unref)(const char8_t *)) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
            {
                sqlite3_result_text(c_ptr(), (const char *)data, int(value.size()), (void (*)(void *))unref);
            }
            else 
            {
                unref(nullptr);
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
            }
        }
    #endif

        /**
         * Return a blob by value from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_blob(..., SQLITE_TRANSIENT)
         * 
         * The blob content is copied and does not need to persist.
         */
        void result(const blob_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_blob(c_ptr(), data, int(value.size()), SQLITE_TRANSIENT);
            else
                sqlite3_result_zeroblob(c_ptr(), 0);
        }

        /**
         * Return a blob by reference from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_blob(..., SQLITE_STATIC)
         * 
         * The blob content is used **by reference** and so must be present in memory
         * as long as SQLite library remain used.
         */
        void result_reference(const blob_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_blob(c_ptr(), data, int(value.size()), SQLITE_STATIC);
            else
                sqlite3_result_zeroblob(c_ptr(), 0);
        }

        /**
         * Return a blob by reference from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_blob(..., destructor)
         * 
         * The blob content is used **by reference**. 
         * 
         * @param value reference to blob to return
         * @param unref called when the reference is no longer needed. 
         * Its argument is the pointer returned from `value.data()`
         */
        void result_reference(const blob_view & value, void (*unref)(const std::byte *) noexcept) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
            {
                sqlite3_result_blob(c_ptr(), data, int(value.size()), (void (*)(void *))unref);
            }
            else
            {
                unref(nullptr);
                sqlite3_result_zeroblob(c_ptr(), 0);
            }
        }


        /**
         * Return a blob of zeroes from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_zeroblob()
         * 
         */
        void result(const zero_blob & value) noexcept
            { sqlite3_result_zeroblob(c_ptr(), int(value.size())); }
        
        template<class T>
        void result(T * ptr, const char * type, void(*destroy)(T*)) noexcept
            { sqlite3_result_pointer(this->c_ptr(), ptr, type, (void(*)(void*))destroy); }
        
        template<class T>
        void result(std::unique_ptr<T> ptr) noexcept
            { this->result(ptr.release(), typeid(T).name(), [](T * p) { delete p;}); }
        
        /**
         * Return a copy of the passed @ref value from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_value
         */
        void result(const value & val) noexcept
            { sqlite3_result_value(c_ptr(), val.c_ptr()); }
      
#if SQLITE_VERSION_NUMBER >= 3009000
        /**
         * Sets the subtype of the result of the implemented SQL function
         * 
         * Equivalent to ::sqlite3_result_subtype
         * 
         * @since SQLite 3.9
         */
        void result_subtype(unsigned value) noexcept
            { sqlite3_result_subtype(c_ptr(), value); }
#endif
        
        /**
         * Get auxiliary data associated with argument values.
         * 
         * Equivalent to ::sqlite3_get_auxdata
         * 
         * @tparam T type of the data
         */
        template<class T>
        T * get_auxdata(int arg) const noexcept
            { return (T *)sqlite3_get_auxdata(this->c_ptr(), arg); }
        
        /**
         * Associate auxiliary data with argument values.
         * 
         * Equivalent to ::sqlite3_set_auxdata
         * 
         * @tparam T type of the data
         */
        template<class T>
        void set_auxdata(int arg, T * data, void (*destroy)(T*)noexcept) noexcept
            { sqlite3_set_auxdata(this->c_ptr(), arg, data, (void(*)(void*))destroy); }

        /**
         * Return the function's user data.
         * 
         * Equivalent to ::sqlite3_user_data
         * 
         * User data can be associated with a function during creation.
         * 
         * @see database::create_function
         */
        template<class T>
        T * user_data() noexcept
            { return (T *)sqlite3_user_data(this->c_ptr()); }
    };
    
}


#endif
