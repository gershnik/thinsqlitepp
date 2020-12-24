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
        using message_ptr = std::unique_ptr<const char, free_message>;
        
    public:
        error(int error_code) noexcept;
        error(int error_code, const database * db) noexcept;
        
        error(int error_code, const std::unique_ptr<database> & db) noexcept:
            error(error_code, db.get())
        {}
        error(int error_code, const database & db) noexcept:
            error(error_code, &db)
        {}
        error(int error_code, message_ptr && message) noexcept:
            _error_code(error_code),
            _message(std::move(message))
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
        
        int extended() const noexcept
            { return _error_code; }
        int primary() const noexcept
            { return _error_code & 0x0FF; }
        int system() const noexcept
            { return _system_error_code; }
        const char * message() const noexcept
            { return _message.get(); }
        
    private:
        static message_ptr copy_message(const char * src) noexcept;
    private:
        int _error_code = 0;
        int _system_error_code = 0;
        message_ptr _message = nullptr;
    };

    class exception : public std::exception
    {
    public:
        exception(class error && err) noexcept:
            _error(std::move(err))
        {}
        exception(const class error & err) noexcept:
            _error(err)
        {}
        exception(int error_code) noexcept:
            _error(error_code)
        {}
        exception(int error_code, const database * db) noexcept:
            _error(error_code, db)
        {}
        exception(int error_code, const std::unique_ptr<database> & db) noexcept:
            _error(error_code, db)
        {}
        exception(int error_code, const database & db) noexcept:
            _error(error_code, db)
        {}
        exception(int error_code, error::message_ptr && message) noexcept:
            _error(error_code, std::move(message))
        {}
        
        [[noreturn]] void raise(int error_code, const database * db) noexcept(false);
        
        int extended_error_code() const noexcept
            { return _error.extended(); }
        int primary_error_code() const noexcept
            { return _error.primary(); }
        int system_error_code() const noexcept
            { return _error.system(); }
        
        const class error & error() noexcept
            { return _error; }
        const char * what() const noexcept override;
        
    private:
        class error _error;
    };
}

#endif
