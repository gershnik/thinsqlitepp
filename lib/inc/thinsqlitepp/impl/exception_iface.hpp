/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_EXCEPTION_IFACE_INCLUDED
#define HEADER_SQLITEPP_EXCEPTION_IFACE_INCLUDED

#include "config.hpp"

#include <exception>
#include <memory>

namespace thinsqlitepp
{
    class database;
    class exception;

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Carries information about SQLite error
     * 
     * The error class stores SQLite [error code](https://www.sqlite.org/rescode.html), 
     * possibly associated system `errno` error code (see ::sqlite3_system_errno)
     * and an error message, if available.
     * 
     * `#include <thinsqlitepp/exception.hpp>`
     */
    class error
    {
    friend class exception;
    private:
        class free_message
        {
        public:
            free_message() noexcept: _free(nullptr) {}
            free_message(void (*free)(void *)) noexcept: _free(free) {}
            
            void operator()(const char * message) const noexcept
                { if (_free) _free(const_cast<char *>(message)); }
            
            explicit operator bool() const noexcept
                { return _free != nullptr; }
        private:
            void (*_free)(void *);
        };
        
    public:
        /// An owning pointer to SQLite error message
        using message_ptr = std::unique_ptr<const char, free_message>;
        
    public:
        /**
         * Constructs an instance from database independent error code.
         * 
         * The error message is obtained via ::sqlite3_errstr
         */
        error(int error_code) noexcept;

        /** 
         * Constructs an instance from the last error reported from a database
         * 
         * The error message is obtained via ::sqlite3_errmsg. 
         * 
         * This constructor tries to discover the full extended error via
         * ::sqlite3_extended_errcode. The error_code argument is currently 
         * only used if it is SQLITE_MISUSE.
         */
        error(int error_code, const database * db) noexcept;
        
        /// @overload
        error(int error_code, const std::unique_ptr<database> & db) noexcept:
            error(error_code, db.get())
        {}
        /// @overload
        error(int error_code, const database & db) noexcept:
            error(error_code, &db)
        {}

        /**
         * Constructs an instance with a given error code and message.
         * 
         * No SQLite calls are performed for this constructor
         */
        error(int error_code, message_ptr && error_message) noexcept:
            _error_code(error_code),
            _message(std::move(error_message))
        {}
        
        error(const error & src) noexcept:
            _error_code(src._error_code),
            _message(src._message.get_deleter() ? copy_message(src._message.get()) : message_ptr(src._message.get()))
        {}
        error(error && src) noexcept:
            _error_code(src._error_code),
            _message(std::move(src._message))
        {}
        ~error() noexcept = default;
        
        error & operator=(error src) noexcept
        {
            swap(*this, src);
            return *this;
        }
        
        friend void swap(error & lhs, error & rhs) noexcept
        {
            using std::swap;
            swap(lhs._error_code, rhs._error_code);
            swap(lhs._message, rhs._message);
        }

        /// Returns full extended error code
        int extended() const noexcept
            { return _error_code; }
        /// Returns primary error code part
        int primary() const noexcept
            { return _error_code & 0x0FF; }
        /// Returns system `errno` error code, if available
        int system() const noexcept
            { return _system_error_code; }
        /// Returns error message or nullptr, if not available
        const char * message() const noexcept
            { return _message.get(); }
        
    private:
        static message_ptr copy_message(const char * src) noexcept;
    private:
        int _error_code = 0;
        int _system_error_code = 0;
        message_ptr _message = nullptr;
    };

    /**
     * Exception used to report any SQLite errors
     * 
     * The payload of this exception is an @ref error object.
     * 
     * `#include <thinsqlitepp/exception.hpp>`
     */
    class exception : public std::exception
    {
    public:
        /// Constructs an instance by moving an error in
        exception(class error && err) noexcept:
            _error(std::move(err))
        {}
        /// Constructs an instance by copying an error
        exception(const class error & err) noexcept:
            _error(err)
        {}
        /// Constructs an instance from database independent error code.
        /// See error::error(int) for details
        exception(int error_code) noexcept:
            _error(error_code)
        {}
        /// Constructs an instance from the last error reported from a database.
        /// See error::error(int, const database *) for details
        exception(int error_code, const database * db) noexcept:
            _error(error_code, db)
        {}
        /// @overload
        exception(int error_code, const std::unique_ptr<database> & db) noexcept:
            _error(error_code, db)
        {}
        /// @overload
        exception(int error_code, const database & db) noexcept:
            _error(error_code, db)
        {}
        /// Constructs an instance with a given error code and message.
        /// See error::error(int, error::message_ptr &&) for details
        exception(int error_code, error::message_ptr && message) noexcept:
            _error(error_code, std::move(message))
        {}

        /// Returns full extended error code of the stored error
        int extended_error_code() const noexcept
            { return _error.extended(); }
        /// Returns primary error code part of the stored error
        int primary_error_code() const noexcept
            { return _error.primary(); }
        /// Returns system `errno` error code of the stored error, if available
        int system_error_code() const noexcept
            { return _error.system(); }
        
        /// Returns the stored error
        const class error & error() noexcept
            { return _error; }

        /**
         * Returns error message
         * 
         * If no error message is available in the stored error object 
         * returns a fixed string such as "<no message available>"
         */
        const char * what() const noexcept override;
        
    private:
        class error _error;
    };

    /** @} */
}

#endif
