/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/master/LICENSE
*/
module;

#ifndef SQLITE_VERSION
    #if THINSQLITEPP_BUILDING_EXTENSION
        #include <sqlite3ext.h>
        SQLITE_EXTENSION_INIT3
    #else
        #include <sqlite3.h>
    #endif
#endif
#include <exception>
#include <memory>
#include <limits>
#include <string>
#include <iterator>
#include <cassert>
#include <span>
#include <type_traits>
#include <compare>
#include <bit>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#if __has_include(<sys/uio.h>)
    #include <sys/uio.h>
#endif
#include <memory.h>
#include <cstdint>
#include <tuple>
#include <stdexcept>
#include <string.h>


export module thinsqlitepp;

#define SQLITEPP_BUILDING_MODULE 1 

#ifndef SQLITE_VERSION
    #if THINSQLITEPP_BUILDING_EXTENSION
        SQLITE_EXTENSION_INIT3
    #else
    #endif
#endif

#define SQLITEPP_SQLITE_VERSION(x, y, z) ((x) * 1000000 + (y) * 1000 + (z))

#if SQLITE_VERSION_NUMBER < SQLITEPP_SQLITE_VERSION(3, 7, 15)

    #error This library requires SQLite 3.7.15 or greater

#endif

#ifdef __clang__
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END _Pragma("GCC diagnostic pop")
#else
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN
    #define SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END
#endif

        #define SQLITEPP_ENABLE_IF(cond, t) requires(cond) t
        #define SQLITEPP_ENABLE_IFP(prefix, cond, t) requires(cond) prefix t

#if SQLITEPP_BUILDING_MODULE 
    #define SQLITEPP_EXPORTED export
#else
    #define SQLITEPP_EXPORTED
#endif

namespace thinsqlitepp
{
    /**
     * Base functionality for all [fake wrapper classes](https://github.com/gershnik/thinsqlitepp#fake-classes)
     * 
     * This is a [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) base class
     * that implements functionality common to all fake wrapper classes.
     * 
     * @tparam T the underlying SQLite type
     * @tparam Derived the derived class for CRTP casts
     */
    template<class T, class Derived>
    class handle
    {
    public:
        /// Operator delete for a fake pointer is no-op
        void operator delete(void *) noexcept
        {}

        /// You cannot construct it
        handle() = delete;
        /// You cannot copy (or move) it
        handle(const handle &) = delete;
        /// You cannot assign it
        handle & operator=(const handle &) = delete;

        /// Create fake pointer from the underlying SQLite one
        static Derived * from(T * obj) noexcept
            { return (Derived *)obj; }

        /// Access the real underlying SQLite type
        T * c_ptr() const noexcept
            { return (T *)this; }

        /// Access the real underlying SQLite type
        friend T * c_ptr(const handle<T, Derived> & obj) noexcept
            { return (T *)&obj; }

        /// Access the real underlying SQLite type
        friend T * c_ptr(const handle<T, Derived> * obj) noexcept
            { return (T *)obj; }

    protected:
        ~handle() noexcept
        {}
    };

}

namespace thinsqlitepp
{
    SQLITEPP_EXPORTED class database;
    SQLITEPP_EXPORTED class exception;

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
    SQLITEPP_EXPORTED
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
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 38, 0)
        /**
         * Returns the offset within SQL statement that produced the error.
         * 
         * Equivalent to ::sqlite3_error_offset
         */
        int offset() const noexcept
            { return _offset;}
    #endif

        /**
         * Move the message out of this object
         * 
         * This allows returning messages to SQLite from callbacks
         * without allocating a copy
         */
        message_ptr extract_message() & noexcept
            { return std::move(_message); }
        /**
         * Move the message out of this object
         * 
         * This allows returning messages to SQLite from callbacks
         * without allocating a copy
         */
        message_ptr extract_message() && noexcept
            { return std::move(_message); }
    private:
        static message_ptr copy_message(const char * src) noexcept;
    private:
        int _error_code = 0;
        int _system_error_code = 0;
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 38, 0)
        int _offset = -1;
    #endif
        message_ptr _message = nullptr;
    };

    /**
     * Exception used to report any SQLite errors
     * 
     * The payload of this exception is an @ref error object.
     * 
     * `#include <thinsqlitepp/exception.hpp>`
     */
    SQLITEPP_EXPORTED
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
        const class error & error() const & noexcept
            { return _error; }

        /// Returns the stored error
        class error & error() & noexcept
            { return _error; }

        /// Returns the stored error
        class error && error() && noexcept
            { return std::move(_error); }

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

    /// @cond PRIVATE

    inline int int_size(size_t size)
    {
        if (size > std::numeric_limits<int>::max())
            throw exception(SQLITE_TOOBIG);
        return int(size);
    }

    inline sqlite3_int64 int64_size(size_t size) noexcept(sizeof(size_t) < sizeof(sqlite3_int64))
    {
        if constexpr (sizeof(size_t) >= sizeof(sqlite3_int64)) {
            if (size > size_t(std::numeric_limits<sqlite3_int64>::max()))
                throw exception(SQLITE_TOOBIG);
        }
        return sqlite3_int64(size);
    }

    /// @endcond

    /** @} */
}

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
    SQLITEPP_EXPORTED
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
         * access to internal static SQLite mutexes. According to 
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
        static std::unique_ptr<mutex> alloc([[maybe_unused]] type t) noexcept
        {
        #if !SQLITEPP_NO_MUTEX
            return std::unique_ptr<mutex>(from(sqlite3_mutex_alloc(int(t))));
        #else
            return {};
        #endif
        }

        /**
         * Lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_enter
         */
        void lock() noexcept
        { 
        #if !SQLITEPP_NO_MUTEX
            sqlite3_mutex_enter(c_ptr());
        #endif
        }
        /** 
         * Try to lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_try
         */
        bool try_lock() noexcept
        { 
        #if !SQLITEPP_NO_MUTEX
            return sqlite3_mutex_try(c_ptr()) == SQLITE_OK;
        #else
            return true;
        #endif
        }

        /**
         * Unlock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_leave
         */
        void unlock() noexcept
        { 
        #if !SQLITEPP_NO_MUTEX
            sqlite3_mutex_leave(c_ptr());
        #endif
        }
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
        lock_adapter(mutex * mutex = nullptr) noexcept: _mutex(mutex)
        {}
        /// Adapt a std::unique_ptr<mutex>
        lock_adapter(const std::unique_ptr<mutex> & mutex) noexcept: _mutex(mutex.get())
        {}

        /**
         * Lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_enter
         */
        void lock() noexcept
        { 
        #if !SQLITEPP_NO_MUTEX
            sqlite3_mutex_enter(c_ptr(_mutex));
        #endif
        }

        /** 
         * Try to lock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_try
         */
        bool try_lock() noexcept
        { 
        #if !SQLITEPP_NO_MUTEX
            return sqlite3_mutex_try(c_ptr(_mutex)) == SQLITE_OK;
        #else
            return true;
        #endif
        }

        /**
         * Unlock the mutex
         * 
         * Equivalent to ::sqlite3_mutex_leave
         */
        void unlock() noexcept
        { 
        #if !SQLITEPP_NO_MUTEX
            sqlite3_mutex_leave(c_ptr(_mutex));
        #endif
        }
    private:
        mutex * _mutex;
    };

    /** @} */
}

namespace thinsqlitepp
{
    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /**
     * A reference to a null terminated string
     * 
     * This class allows passing either a `const T *` or `std::basic_string<T>`
     * to a function that internally needs a null terminated const T *
     * 
     * Note that this class has _reference_ semantics. The string it refers to must
     * be kept alive as long as the instance of this class is alive.
     * 
     * @tparam T character type
     */
    SQLITEPP_EXPORTED
    template<class T>
    class basic_string_param
    {
    public:
        /// Construct an instance from a raw pointer
        basic_string_param(const T * str) noexcept : _str(str)
        {}
        /// Construct an instance from std::basic_string<T>::c_str()
        basic_string_param(const std::basic_string<T> & str) noexcept : _str(str.c_str())
        {}
        /// Construct an instance from a nullptr
        basic_string_param(std::nullptr_t) noexcept : _str(nullptr)
        {}

        /// Returns the stored pointer
        const T * c_str() const noexcept
            { return _str; }
    private:
        const T * _str;
    };

    /// Convenience typedef
    SQLITEPP_EXPORTED using string_param = basic_string_param<char>;

#if __cpp_char8_t >= 201811

    /// Convenience typedef. Only available if you compiler/library supports char8_t
    SQLITEPP_EXPORTED using u8string_param = basic_string_param<char8_t>;

#endif

    /** @} */
}

    namespace thinsqlitepp
    {
        /**
         * @addtogroup Utility Utilities
         * @{
         */

        /**
         * Alias or reimplementation of std::span
         * 
         * If std::span is available, %thinsqlitepp::span is a typedef to it.
         * Otherwise it is an equivalent class defined in this library
         */
        SQLITEPP_EXPORTED
        template<class T>
        using span = std::span<T>;

        /** @} */
    }

namespace thinsqlitepp
{
    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /// A blob_view is a span of bytes
    SQLITEPP_EXPORTED using blob_view = span<const std::byte>;

    /**
     * An efficient blob of zeroes of a given size
     * 
     * This class is an STL random-access container that returns 0 
     * for all its elements. It simply stores blob size and 
     * doesn't allocate any memory.
     * 
     * SQLite contains optimized methods that operate on "blobs of zeroes" of
     * a given size (e.g. ::sqlite3_bind_zeroblob). This class is used to pass
     * "blobs of zeroes" to overloaded C++ methods (e.g. statement::bind(int, const zero_blob &))
     * to achieve the same effect in this library.
     * 
     */
    SQLITEPP_EXPORTED
    class zero_blob
    {
    public:
        using element_type           = const std::byte;
        using value_type             = std::byte;
        using index_type             = size_t;
        using difference_type        = ptrdiff_t;
        using pointer                = const std::byte *;
        using const_pointer          = const std::byte *;
        using reference              = const std::byte &;
        using const_reference        = const std::byte &;

        class const_iterator
        {
        friend class zero_blob;
        public:
            using iterator_category      = std::random_access_iterator_tag;
            using value_type             = zero_blob::value_type;
            using difference_type        = zero_blob::difference_type;
            using pointer                = zero_blob::pointer;
            using reference              = zero_blob::reference;
        public:
            constexpr const_iterator() noexcept = default;

            constexpr const_reference operator*() const noexcept { return s_value; }

            constexpr const_iterator & operator++() noexcept { ++_idx; return *this; }
            constexpr const_iterator operator++(int) noexcept { return _idx++; }
            constexpr const_iterator & operator+=(difference_type diff) noexcept { _idx += size_t(diff); return *this; }

            constexpr const_iterator & operator--() noexcept { --_idx; return *this; }
            constexpr const_iterator operator--(int) noexcept { return _idx--; }
            constexpr const_iterator & operator-=(difference_type diff) noexcept { _idx -= size_t(diff); return *this; }

            friend constexpr difference_type operator-(const_iterator lhs, const_iterator rhs) noexcept { return difference_type(lhs._idx - rhs._idx); }

            friend constexpr bool operator==(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx == rhs._idx; };
            friend constexpr bool operator!=(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx != rhs._idx; };
            friend constexpr bool operator<(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx < rhs._idx; };
            friend constexpr bool operator<=(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx <= rhs._idx; };
            friend constexpr bool operator>(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx > rhs._idx; };
            friend constexpr bool operator>=(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx >= rhs._idx; };
        private:
            constexpr const_iterator(size_t idx) noexcept : _idx(idx) {};
        private:
            size_t _idx = size_t(-1);
        };

        using iterator               = const_iterator;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        constexpr zero_blob(size_t size = 0) noexcept : _size{size} {}

        constexpr zero_blob           (const zero_blob&) noexcept = default;
        constexpr zero_blob& operator=(const zero_blob&) noexcept = default;

        constexpr zero_blob first(index_type count) const noexcept
        {
            assert(count <= size());
            return {count};
        }

        constexpr zero_blob last (index_type count) const noexcept
        {
            assert(count <= size());
            return {count};
        }

        constexpr zero_blob
        subspan(index_type offset, index_type count = size_t(-1)) const noexcept
        {
            assert(offset <= size());
            assert(count  <= size() || count == size_t(-1));
            if (count == size_t(-1))
                return {size() - offset};
            assert(offset <= size() - count);
            return {count};
        }

        constexpr index_type size()       const noexcept { return _size; }
        constexpr index_type size_bytes() const noexcept { return _size * sizeof(element_type); }
        constexpr bool empty()            const noexcept { return _size == 0; }

        constexpr reference operator[](index_type idx) const noexcept
        {
            assert(idx < size());
            (void)idx;
            return s_value;
        }

        constexpr reference front() const noexcept
        {
            assert(!empty());
            return s_value;
        }

        constexpr reference back() const noexcept
        {
            assert(!empty());
            return s_value;
        }

        constexpr iterator                 begin() const noexcept { return iterator(0); }
        constexpr iterator                   end() const noexcept { return iterator(size()); }
        constexpr const_iterator          cbegin() const noexcept { return const_iterator(0); }
        constexpr const_iterator            cend() const noexcept { return const_iterator(size()); }
        constexpr reverse_iterator        rbegin() const noexcept { return reverse_iterator(end()); }
        constexpr reverse_iterator          rend() const noexcept { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        constexpr const_reverse_iterator   crend() const noexcept { return const_reverse_iterator(cbegin()); }
    private:
        size_t _size;

        static inline const std::byte s_value{0};
    };

    /** @} */
}

namespace thinsqlitepp
{

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
     * Use database::open_blob to create blob objects.
     * 
     * `#include <thinsqlitepp/blob.hpp>`
     * 
     */
    SQLITEPP_EXPORTED
    class blob final : public handle<sqlite3_blob, blob>
    {
    public:
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
            return ret >= 0 ? size_t(ret) : 0;
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
            int res = sqlite3_blob_read(c_ptr(), data, int(size * sizeof(value_type)), int_size(offset));
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
        void write(size_t offset, const R & range)
        {
            using value_type = std::remove_reference_t<std::ranges::range_reference_t<R>>;
            auto data = std::data(range);
            auto size = std::size(range);
            if (size > std::numeric_limits<int>::max() / sizeof(value_type))
                throw exception(SQLITE_TOOBIG);
            int res = sqlite3_blob_write(c_ptr(), data, int(size * sizeof(value_type)), int_size(offset));
            if (res != SQLITE_OK)
                throw exception(res); //we do not know the db here, unfortunately
        }

    #endif
    };

    /** @} */
}

#define SQLITEPP_CALL_DETECTOR_0(name, rettype, call) \
    private: \
        template<class T> static constexpr bool has_##name##_impl(decltype(call()) *) \
            { return std::is_same_v<decltype(call()), rettype>; } \
        template<class T> static constexpr bool has_##name##_impl(...) \
            { return false; } \
    public: \
        template<class T> static constexpr bool has_##name = has_##name##_impl<T>(nullptr); \
        template<class T> static constexpr bool has_noexcept_##name = []() constexpr { \
            if constexpr (has_##name<T>) \
                return noexcept(call()); \
            else \
                return false; \
        }()

#define SQLITEPP_CALL_DETECTOR(name, rettype, call, ...) \
    private: \
        template<class T> static constexpr bool has_##name##_impl(decltype(call(__VA_ARGS__)) *) \
            { return std::is_same_v<decltype(call(__VA_ARGS__)), rettype>; } \
        template<class T> static constexpr bool has_##name##_impl(...) \
            { return false; } \
    public: \
        template<class T> static constexpr bool has_##name = has_##name##_impl<T>(nullptr); \
        template<class T> static constexpr bool has_noexcept_##name = []() constexpr { \
            if constexpr (has_##name<T>) \
                return noexcept(call(__VA_ARGS__)); \
            else \
                return false; \
        }()

#define SQLITEPP_STATIC_METHOD_DETECTOR_0(rettype, name) SQLITEPP_CALL_DETECTOR_0(name, rettype, T::name)
#define SQLITEPP_STATIC_METHOD_DETECTOR(rettype, name, ...) SQLITEPP_CALL_DETECTOR(name, rettype, T::name, __VA_ARGS__)
#define SQLITEPP_METHOD_DETECTOR_0(rettype, name) SQLITEPP_CALL_DETECTOR_0(name, rettype, std::declval<T>().name)
#define SQLITEPP_METHOD_DETECTOR(rettype, name, ...) SQLITEPP_CALL_DETECTOR(name, rettype, std::declval<T>().name, __VA_ARGS__)

namespace thinsqlitepp
{

    template< class T > using type_identity = std::type_identity<T>;

    template< class T > using type_identity_t = std::type_identity_t<T>;

    /** @cond PRIVATE */

    template<class T, bool B>
    constexpr bool dependent_bool = B;

    template<class T>
    constexpr bool dependent_false = dependent_bool<T, false>;

    template<class T>
    constexpr bool dependent_true = dependent_bool<T, true>;

    //MARK: - strong_ordering_from_int

    template<class T>
    using size_equivalent = 
        std::conditional_t<sizeof(T) == sizeof(signed char),  signed char,      
        std::conditional_t<sizeof(T) == sizeof(short), short,
        std::conditional_t<sizeof(T) == sizeof(int), int,
        std::conditional_t<sizeof(T) == sizeof(long), long,
        std::conditional_t<sizeof(T) == sizeof(long long), long long,
        void
    >>>>>;

        inline std::strong_ordering strong_ordering_from_int(int val)
        {
            using equivalent = size_equivalent<std::strong_ordering>;
            if constexpr (!std::is_void_v<equivalent>)
            {
                equivalent eq_val = equivalent((val > 0) - (val < 0));
                    return std::bit_cast<std::strong_ordering>(eq_val);
            }
            else
            {
                return val < 0 ? std::strong_ordering::less : (
                       val == 0 ? std::strong_ordering::equal : (
                       std::strong_ordering::greater 
                       ));
            }
        }

    /** @endcond */

}

namespace thinsqlitepp
{
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) && (THINSQLITEPP_ENABLE_EXPERIMENTAL || THINSQLITEPP_ENABLE_EXPIREMENTAL)

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
     * Requires THINSQLITEPP_ENABLE_EXPERIMENTAL macro to be defined to 1 as the underlying SQLite
     * feature is experimental.
     * 
     * `#include <thinsqlitepp/snapshot.hpp>`
     * 
     */
    SQLITEPP_EXPORTED
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

        /// @{ 
        /// @name C++20 comparison operator. Not present in C++17
        friend std::strong_ordering operator<=>(const snapshot & lhs, const snapshot & rhs) noexcept
            { return strong_ordering_from_int(compare(lhs, rhs)); }
        /// @}
    };

#endif

    /** @} */
}

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
    SQLITEPP_EXPORTED
    template<class T>
    class sqlite_deleter
    {
    public:
        void operator()(T * mem) const noexcept
            { sqlite3_free(const_cast<std::remove_const_t<T> *>(mem)); }
    };

    /// @cond DEPRECATED
    SQLITEPP_EXPORTED
    template<class T>
    using deleter [[deprecated]] = sqlite_deleter<T>;
    /// @endcond

    /**
     * A string allocated by SQLite
     * 
     * `#include <thinsqlitepp/memory.hpp>`
     */
    SQLITEPP_EXPORTED
    using allocated_string = std::unique_ptr<char, sqlite_deleter<char>>;

    /**
     * A byte buffer allocated by SQLite
     * 
     * `#include <thinsqlitepp/memory.hpp>`
     */
    SQLITEPP_EXPORTED
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

    SQLITEPP_EXPORTED
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
    SQLITEPP_EXPORTED
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
    SQLITEPP_EXPORTED
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
            return (T *)sqlite_allocate(n * sizeof(T));
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

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"

    #if defined(__APPLE__) && defined(__clang__)
        #pragma GCC diagnostic ignored "-Wunguarded-availability-new"
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif
#endif

namespace thinsqlitepp
{
    SQLITEPP_EXPORTED class context;
    SQLITEPP_EXPORTED class row;
    SQLITEPP_EXPORTED class value;

    /** @cond PRIVATE */

    struct database_detector
    {
        SQLITEPP_CALL_DETECTOR(enable_load_extension, int, sqlite3_enable_load_extension, (T *)nullptr, int{});
        SQLITEPP_CALL_DETECTOR(load_extension, int, sqlite3_load_extension, (T *)nullptr, 
                                                                            (const char *)nullptr,
                                                                            (const char *)nullptr,
                                                                            (char **)nullptr);

        SQLITEPP_METHOD_DETECTOR(void, step, (context *)nullptr, int{}, (value **)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, last, (context *)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, inverse, (context *)nullptr, int{}, (value **)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, current, (context *)nullptr);

    public:
        template<class T>
        static constexpr bool is_function = std::is_nothrow_invocable_r_v<void, T, context *, int, value **>;

        template<class T>
        static constexpr bool is_aggregate_function = has_noexcept_step<T> && has_noexcept_last<T>;

        template<class T>
        static constexpr bool is_aggregate_window_function = is_aggregate_function<T> && has_noexcept_inverse<T> && has_noexcept_current<T>;

        template<class R, class T, class... ArgTypes>
        static constexpr bool is_pointer_to_callback =  std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<R, std::remove_pointer_t<T>, ArgTypes...>);

        template<class R, class T, class... ArgTypes>
        static constexpr bool is_pointer_to_throwing_callback =  std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_invocable_r_v<R, std::remove_pointer_t<T>, ArgTypes...>);

        template<class T>
        static constexpr bool is_pointer_to_function = std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> &&
                (
                   is_function<std::remove_pointer_t<T>> ||
                   is_aggregate_function<std::remove_pointer_t<T>>
                )
            );

        template<class T>
        static constexpr bool is_pointer_to_window_function = std::is_null_pointer_v<T> ||
                (std::is_pointer_v<T> && is_aggregate_window_function<std::remove_pointer_t<T>>);
    };

    /** @endcond */

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Database Connection
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3.
     * 
     * `#include <thinsqlitepp/database.hpp>`
     * 
     */
    SQLITEPP_EXPORTED
    class database final : public handle<sqlite3, database>
    {
    private:
        template<int Code, class ...Args>
        struct config_option
        {
            static void apply(database & db, Args && ...args)
            {
                int res = sqlite3_db_config(db.c_ptr(), Code, std::forward<Args>(args)...);
                if (res != SQLITE_OK)
                    throw exception(res, db);
            }
        };
        template<int Code> struct config_mapping;

        template<int Code, class ...Args>
        struct vtab_config_option
        {
            static void apply(database & db, Args && ...args)
            {
                int res = sqlite3_vtab_config(db.c_ptr(), Code, std::forward<Args>(args)...);
                if (res != SQLITE_OK)
                    throw exception(res, db);
            }
        };
        template<int Code> struct vtab_config_mapping;

    public:
        /**
         * Open a new database connection
         * 
         * Equivalent to ::sqlite3_open_v2
         */
        static std::unique_ptr<database> open(const string_param & db_filename, int flags, const char * vfs = nullptr);

        /// Equivalent to ::sqlite3_close_v2
        ~database() noexcept
            { sqlite3_close_v2(c_ptr()); }

        //MARK: -

        /**
         * Set a busy timeout
         * 
         * Equivalent to ::sqlite3_busy_timeout
         */
        void busy_timeout(int ms)
            { check_error(sqlite3_busy_timeout(c_ptr(), ms)); }

        //MARK: -

    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 50, 0)

        /**
         * Set a setlk timeout
         * 
         * Equivalent to ::sqlite3_setlk_timeout
         */
        void setlk_timeout(int ms, int flags = 0)
            { check_error(sqlite3_setlk_timeout(c_ptr(), ms, flags)); }

    #endif

        //MARK: Client data

    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)

        /**
         * Set arbitrary client data to this database connection
         * 
         * Equivalent to ::sqlite3_set_clientdata
         */
        void set_clientdata(const string_param & name, void * ptr, void(*destroy)(void*) = nullptr)
            { check_error(sqlite3_set_clientdata(c_ptr(), name.c_str(), ptr, destroy)); }

        /**
         * Set arbitrary client data to this database connection
         * 
         * Equivalent to ::sqlite3_set_clientdata
         * 
         * This is a safer overload of @ref set_clientdata(const string_param &, void *, void(*)(void*))
         * that takes a pointer via std::unique_ptr ownership transfer.
         */
        template<class T>
        void set_clientdata(const string_param & name, std::unique_ptr<T> && data)
        { 
            set_clientdata(name, data.release(), [](void * d) {
                delete static_cast<T *>(d);
            });
        }

        /**
         * Get arbitrary client data of this database connection
         * 
         * Equivalent to ::sqlite3_get_clientdata
         * 
         * Note that no type checks are performed to ensure that the data
         * is, indeed, of the type T. It is  **your** responsibility to match T
         * to the type passed to @ref set_clientdata
         */
        template<class T>
        T * get_clientdata(const string_param & name) noexcept
            { return static_cast<T *>(sqlite3_get_clientdata(c_ptr(), name.c_str())); }

    #endif

        //MARK: -

        /**
         * Count of the number of rows modified
         * 
         * Equivalent to ::sqlite3_changes
         */
        int64_t changes() const noexcept
        {
        #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 36, 1)
            return sqlite3_changes64(c_ptr()); 
        #else
            return sqlite3_changes(c_ptr()); 
        #endif
        }

        //MARK: -

    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 51, 1)
        /**
         * Set error code and message
         * 
         * Equivalent to ::sqlite3_set_errmsg
         */
        void set_errmsg(int errcode, const string_param & message)
            { check_error(sqlite3_set_errmsg(c_ptr(), errcode, message.c_str())); }
    #endif

        //MARK: - exec

        /** @{
         * @anchor database_exec
         * @name Executing queries
         */

        /**
         * Run multiple statements of SQL
         * 
         * Unlike other functions in this library this one **DOES NOT** delegate to
         * ::sqlite3_exec but instead implements equivalent functionality directly.
         * 
         * It runs zero or more UTF-8 encoded, semicolon-separate SQL statements passed
         * as the `sql` argument. If an error occurs while evaluating the SQL statements
         * then execution of the current statement stops and subsequent statements are skipped.
         * 
         * As usual the error will be reported via an @ref exception
         * 
         * @param sql Statements to execute
         */
        void exec(std::string_view sql);

    #if __cpp_char8_t >= 201811
        /// @overload
        template<class T>
        T exec(std::u8string_view sql)
            {  return exec(std::string_view((const char *)sql.data(), sql.size())); }
    #endif

        /**
         * Run multiple statements of SQL with a callback
         * 
         * Unlike other functions in this library this one **DOES NOT** delegate to
         * ::sqlite3_exec but instead implements equivalent functionality directly.
         * 
         * It runs zero or more UTF-8 encoded, semicolon-separate SQL statements passed
         * as the `sql` argument. The `callback` callable is passed by value and
         * is invoked for each result row coming out of the evaluated SQL statements. 
         * The callable can have one of the 4 possible variants:
         * ```cpp
         * 1. bool callback(int statement_idx, row current_row)
         * 2. void callback(int statement_idx, row current_row)
         * 3. bool callback(row current_row)
         * 4. void callback(row current_row)
         * ```
         * 
         * If more than one way of calling the callback is possible the way it will
         * be invoked is chosen in the order given above.
         * 
         * For variants 1 and 3 if an invocation of callback returns `false` then 
         * the execution of the current statement stops and subsequent statements are skipped.
         * 
         * For variants 1 and 2 the `statement_idx` is the index of the SQL statement 
         * being executed. If you only pass a single statement to `exec()` you 
         * generally don't need these variants.
         * 
         * The @p callback argument is returned back from the function which allows it to 
         * accumulate state.
         * 
         * If an error occurs while evaluating the SQL statements
         * then execution of the current statement stops and subsequent statements are skipped.
         * 
         * As usual the error will be reported via an @ref exception
         * 
         * @param sql Statements to execute
         * @param callback Callback to execute for each row of the results
         * @returns the `callback` argument
         */
        template<class T>
        SQLITEPP_ENABLE_IF((
            std::is_invocable_r_v<bool, T, int, row> ||
            std::is_invocable_r_v<void, T, int, row> ||
            std::is_invocable_r_v<bool, T, row> ||
            std::is_invocable_r_v<void, T, row>),
        T) exec(std::string_view sql, T callback);

    #if __cpp_char8_t >= 201811
        /// @overload
        template<class T>
        T exec(std::u8string_view sql, T callback)
            {  return exec(std::string_view((const char *)sql.data(), sql.size()), callback); }
    #endif

        /// @}

        /** @{
         * @anchor database_callbacks
         * @name Callbacks and notifications
         */

        //MARK: - busy_handler

        /**
         * Register a callback to handle #SQLITE_BUSY errors
         * 
         * Equivalent to ::sqlite3_busy_handler
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) busy_handler(int (* handler)(type_identity_t<T> data_ptr, int count_invoked) noexcept, T data_ptr)
            { check_error(sqlite3_busy_handler(this->c_ptr(), (int (*)(void*,int))handler, data_ptr)); }

        /**
         * Register a callback to handle #SQLITE_BUSY errors
         * 
         * Equivalent to ::sqlite3_busy_handler
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * int count_invoked = ...;
         * bool result = (*handler_ptr)(count_invoked);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<bool, T, int>),
        void) busy_handler(T handler_ptr);

        //MARK: - collation_needed

        /**
         * Register a callback to be called when undefined collation sequence is required
         * 
         * Equivalent to ::sqlite3_collation_needed
         * 
         * @param data_ptr A pointer to callback data or nullptr.
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) collation_needed(T data_ptr, void (* handler)(type_identity_t<T> data_ptr, database *, int encoding, const char * name) noexcept)
            { check_error(sqlite3_collation_needed(this->c_ptr(), data_ptr, (void(*)(void*,sqlite3*,int,const char*))handler)); }

        /**
         * Register a callback to be called when undefined collation sequence is required
         * 
         * Equivalent to ::sqlite3_collation_needed
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * database * db = ...;
         * int encoding = <one of SQLITE_UTF8, SQLITE_UTF16BE, or SQLITE_UTF16LE>
         * const char * name = ...;
         * (*handler_ptr)(db, encoding, name);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, database *, int, const char *>),
        void) collation_needed(T handler_ptr);

        //MARK: - commit_hook

        /**
         * Register a callback to be called on commit
         * 
         * Equivalent to ::sqlite3_commit_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) commit_hook(int (* handler)(type_identity_t<T> data_ptr) noexcept, T data_ptr) noexcept
            { sqlite3_commit_hook(this->c_ptr(), (int(*)(void*))handler, data_ptr); }

        /**
         * Register a callback to be called on commit
         * 
         * Equivalent to ::sqlite3_commit_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * bool result = (*handler_ptr)();
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<bool, T>),
        void) commit_hook(T handler_ptr) noexcept;

        //MARK: - rollback_hook

        /**
         * Register a callback to be called on rollback
         * 
         * Equivalent to ::sqlite3_rollback_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) rollback_hook(void (* handler)(type_identity_t<T> data_ptr) noexcept, T data_ptr) noexcept
            { sqlite3_rollback_hook(this->c_ptr(), (void(*)(void*))(handler), data_ptr); }

        /**
         * Register a callback to be called on rollback
         * 
         * Equivalent to ::sqlite3_rollback_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)();
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T>),
        void) rollback_hook(T handler_ptr) noexcept;

        //MARK: - update_hook

        /**
         * Register a callback to be called whenever a row is updated, inserted or deleted in a rowid table.
         * 
         * Equivalent to ::sqlite3_update_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) update_hook(void (* handler)(type_identity_t<T> data_ptr, int op, const char * db_name, const char * table, sqlite3_int64 rowid) noexcept, 
                          T data_ptr) noexcept
            { sqlite3_update_hook(this->c_ptr(), (void(*)(void*,int,char const *,char const *,sqlite3_int64))(handler), data_ptr); }

        /**
         * Register a callback to be called whenever a row is updated, inserted or deleted in a rowid table.
         * 
         * Equivalent to ::sqlite3_update_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)(int op, const char * db_name, const char * table, int64_t rowid);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, int, const char *, const char *, int64_t>),
        void) update_hook(T handler_ptr) noexcept;

        //MARK: - preupdate_hook

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 16, 0) && defined(SQLITE_ENABLE_PREUPDATE_HOOK)
        /**
         * Register a callback to be called prior to each INSERT, UPDATE, and DELETE operation on a database table.
         * 
         * Equivalent to ::sqlite3_preupdate_hook
         * 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         * 
         * @since SQLite 3.16
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) preupdate_hook(void (* handler)(type_identity_t<T> data_ptr, 
                                              database * db, 
                                              int op, 
                                              const char * db_name, 
                                              const char * table, 
                                              sqlite3_int64 rowid_old,
                                              sqlite3_int64 rowid_new) noexcept, 
                             T data_ptr) noexcept
        { 
            sqlite3_preupdate_hook(this->c_ptr(), (void(*)(void*,sqlite3 *,int,char const *,char const *,sqlite3_int64,sqlite3_int64))(handler), 
                                    data_ptr); 
        }

        /**
         * Register a callback to be called prior to each INSERT, UPDATE, and DELETE operation on a database table.
         * 
         * Equivalent to ::sqlite3_preupdate_hook
         * 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)(database * db, int op, const char * db_name, const char * table, int64_t rowid_old, int64_t rowid_new);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         * 
         * @since SQLite 3.16
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<void, T, database *, int, const char *, const char *, int64_t, int64_t>),
        void) preupdate_hook(T handler_ptr) noexcept;

    #endif

        /**
         * Register a callback to be called each time data is committed to a database in wal mode.
         * 
         * Equivalent to ::sqlite3_wal_hook
         * 
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) wal_hook(int (* handler)(type_identity_t<T> data_ptr, database * db, const char * db_name, int num_pages) noexcept, 
                       T data_ptr) noexcept
            { sqlite3_wal_hook(this->c_ptr(), (int(*)(void *,sqlite3*,const char*,int))(handler), data_ptr); }

        /**
         * Register a callback to be called each time data is committed to a database in wal mode.
         * 
         * Equivalent to ::sqlite3_wal_hook
         * 
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)(const char * db_name, int num_pages);
         * ```
         * This invocation can throw exceptions. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_throwing_callback<void, T, database *, const char *, int>),
        void) wal_hook(T handler_ptr) noexcept;

        /// @}

        //MARK: -

    #if defined(SQLITE_ENABLE_PREUPDATE_HOOK)
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 16, 0)

        /** @{
         * @name Preupdate hook helpers
         */

        /**
         * Returns value of a column of the table row before it is updated.
         * 
         * Equivalent to ::sqlite3_preupdate_old
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.16
         */
        value * preupdate_old(int column_idx);

        /**
         * Returns value of a column of the table row after it is updated.
         * 
         * Equivalent to ::sqlite3_preupdate_new
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.16
         */
        value * preupdate_new(int column_idx);

        /**
         * Returns the number of columns in the row that is being inserted, updated, or deleted.
         * 
         * Equivalent to ::sqlite3_preupdate_count
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.16
         */
        int preupdate_count() const noexcept
            { return sqlite3_preupdate_count(c_ptr()); }

        /**
         * Returns the "depth" of an update from the top level SQL
         * 
         * Equivalent to ::sqlite3_preupdate_depth
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @returns 0 if the preupdate callback was invoked as a result of a direct 
         * insert, update, or delete operation; or 1 for inserts, updates, or deletes 
         * invoked by top-level triggers; or 2 for changes resulting from triggers 
         * called by top-level triggers; and so forth.
         * 
         * @since SQLite 3.16
         */
        int preupdate_depth() const noexcept
            { return sqlite3_preupdate_depth(c_ptr()); }

    #endif
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 36, 0)
        /**
         * Returns the index of the column being written via ::sqlite3_blob_write.
         * 
         * Equivalent to ::sqlite3_preupdate_blobwrite
         * 
         * This can only be called from a pre-update hook. 
         * Available only if #SQLITE_ENABLE_PREUPDATE_HOOK is defined during compilation
         * 
         * @since SQLite 3.36
         */    
        int preupdate_blobwrite() const noexcept
            { return sqlite3_preupdate_blobwrite(c_ptr()); }

    #endif
    #endif

        /** @} */

        //MARK: -

        /** @{
         * @name WAL checkpoint control
         */

        /**
         * Checkpoint a database
         * 
         * Equivalent to ::sqlite3_wal_checkpoint_v2
         * 
         * @param db_name Name of attached database (or nullptr)
         * @param mode One of SQLITE_CHECKPOINT_ values
         * @returns A pair of {Size of WAL log in frames, Total number of frames checkpointed} or {-1, -1}
         * if the database is not in WAL mode
         */
        std::pair<int, int> checkpoint(const string_param & db_name, int mode = SQLITE_CHECKPOINT_PASSIVE);

        /**
         * Configure an auto-checkpoint
         * 
         * Equivalent to ::sqlite3_wal_autocheckpoint
         */
        void autocheckpoint(int num_frames)
            { check_error(sqlite3_wal_autocheckpoint(c_ptr(), num_frames)); }

        /* @} */

        //MARK: - create_collation

        /** @{
         * @anchor database_create_collation
         * @name Defining collating sequences
         */

        /**
         * Define a new collating sequence
         * 
         * Equivalent to ::sqlite3_create_collation_v2
         * 
         * @param name Collation name
         * @param encoding One of [SQLite text encodings](https://www.sqlite.org/c3ref/c_any.html)
         * @param collator_ptr A pointer to a collator or nullptr.
         * @param compare A collating function that matches the type of @p collator_ptr argument. Can be
         *  nullptr.
         * @param destructor A "destructor" function for the @p collator_ptr argument. Can be
         *  nullptr.
         * 
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) create_collation(const string_param & name, int encoding,
                               T collator_ptr,
                               int (*compare)(type_identity_t<T> collator, int lhs_len, const void * lhs_bytes, int rhs_len, const void * rhs_bytes) noexcept,
                               void (*destructor)(type_identity_t<T> collator) noexcept);

        /**
         * Define a new collating sequence
         * 
         * Equivalent to ::sqlite3_create_collation_v2
         * 
         * @param name Collation name
         * @param encoding One of [SQLite text encodings](https://www.sqlite.org/c3ref/c_any.html)
         * @param collator_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * span<const std::byte> lhs = ...;
         * span<const std::byte> rhs = ...;
         * int res = (*collator_ptr)(lhs, rhs);
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the collator.
         * @param destructor A "destructor" function for the @p collator_ptr argument. Can be
         * nullptr. Unlike the ::sqlite3_create_collation_v2 the @p destructor is always called
         * even if this function throws an exception.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((database_detector::is_pointer_to_callback<int, T, span<const std::byte>, span<const std::byte>>),
        void) create_collation(const string_param & name, int encoding, T collator_ptr,
                              void (*destructor)(type_identity_t<T> obj) noexcept = nullptr);

        /// @}

        //MARK: - create_function

        /** @{
         * @anchor database_create_function
         * @name Creating or redefining SQL functions
         */

        /**
         * Create or redefine SQL function
         * 
         * Equivalent to ::sqlite3_create_function_v2
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL function takes. 
         * If this parameter is -1, then the SQL function may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param data_ptr A pointer to callback data or nullptr. The implementation of the function 
         * can gain access to this pointer using context::user_data().
         * @param func Function callback. See ::sqlite3_create_function_v2
         * @param step Step callback. See ::sqlite3_create_function_v2
         * @param last Last callback. See ::sqlite3_create_function_v2
         * @param destructor A "destructor" function for @p data_ptr. Can be nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) create_function(const char * name, int arg_count, int flags, T data_ptr,
                              void (*func)(context *, int, value **) noexcept,
                              void (*step)(context *, int, value **) noexcept,
                              void (*last)(context*) noexcept,
                              void (*destructor)(type_identity_t<T> data_ptr) noexcept);

        /**
         * Create or redefine SQL function
         * 
         * Equivalent to ::sqlite3_create_function_v2
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL function takes. 
         * If this parameter is -1, then the SQL function may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param impl_ptr A **pointer** to C++ object that implements the function or nullptr. 
         * The C++ object pointed to it needs to:
         * - For scalar SQL function **only** be callable as:
         *   ```
         *   (*impl_ptr)(context *, int, value **);
         *   ```
         * - For aggregate SQL function **only** be callable as:
         *   ```
         *   impl_ptr->step(context *, int, value **);
         *   impl_ptr->last(context *);
         *   ```
         * If impl_ptr is nullptr the function is removed.
         * @param destructor A "destructor" function for @p impl_ptr. Can be nullptr.
         */
        template<class T>
        SQLITEPP_ENABLE_IF(database_detector::is_pointer_to_function<T>,
        void) create_function(const char * name, int arg_count, int flags, 
                              T impl_ptr, void (*destructor)(type_identity_t<T> obj) noexcept = nullptr);

        //MARK: - create_window_function

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 25, 0)

        /**
         * Create or redefine SQL [aggregate window function](https://www.sqlite.org/windowfunctions.html#aggwinfunc)
         * 
         * Equivalent to ::sqlite3_create_window_function
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL aggregate takes. 
         * If this parameter is -1, then the SQL aggregate may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param data_ptr A pointer to callback data or nullptr. The implementation of the function 
         * can gain access to this pointer using context::user_data().
         * @param step Step callback. See ::sqlite3_create_window_function
         * @param last Last callback. See ::sqlite3_create_window_function
         * @param current Current callback. See ::sqlite3_create_window_function
         * @param inverse Inverse callback. See ::sqlite3_create_window_function
         * @param destructor A "destructor" function for @p data_ptr. Can be nullptr.
         * 
         * @since SQLite 3.25
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) create_window_function(const char * name, int arg_count, int flags, T data_ptr,
                                     void (*step)(context *, int, value **) noexcept,
                                     void (*last)(context*) noexcept,
                                     void (*current)(context*) noexcept,
                                     void (*inverse)(context *, int, value **) noexcept,
                                     void (*destructor)(type_identity_t<T> data_ptr) noexcept);

        /**
         * Create or redefine SQL [aggregate window function](https://www.sqlite.org/windowfunctions.html#aggwinfunc)
         * 
         * Equivalent to ::sqlite3_create_window_function
         * 
         * @param name Name of the SQL function to be created or redefined
         * @param arg_count The number of arguments that the SQL function takes. 
         * If this parameter is -1, then the SQL function may take any number of arguments.
         * @param flags Combination of
         * - [Text encoding flags](https://www.sqlite.org/c3ref/c_any.html) that specify
         * what encoding this SQL function prefers for its parameters
         * - [Function flags](https://www.sqlite.org/c3ref/c_deterministic.html)
         * @param impl_ptr A **pointer** to C++ object that implements the function or nullptr. 
         * The C++ object pointed to it needs to be callable as:
         *   ```
         *   impl_ptr->step(context *, int, value **);
         *   impl_ptr->last(context *);
         *   impl_ptr->current(context *);
         *   impl_ptr->inverse(context *, int, value **);
         *   ```
         * If impl_ptr is nullptr the function is removed.
         * @param destructor A "destructor" function for @p impl_ptr. Can be nullptr.
         * 
         * @since SQLite 3.25
         */
        template<class T>
        SQLITEPP_ENABLE_IF(database_detector::is_pointer_to_window_function<T>,
        void) create_window_function(const char * name, int arg_count, int flags, 
                                     T impl_ptr, void (*destructor)(type_identity_t<T> obj) noexcept = nullptr);
#endif

        ///@}

        //MARK: -
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) 
        /**
         * Flush caches to disk mid-transaction
         * 
         * Equivalent to ::sqlite3_db_cacheflush
         * 
         * @since SQLite 3.10
         */
        void cacheflush()
        {
            if (int res = sqlite3_db_cacheflush(c_ptr()); res != SQLITE_OK)
                throw exception(res); //sic! sqlite3_db_cacheflush doesn't set DB error
        }
#endif
        /**
         * Configure database connection
         * 
         * Wraps ::sqlite3_db_config
         * 
         * @tparam Code One of the SQLITE_DBCONFIG_ options. Needs to be explicitly specified
         * @tparam Args depend on the @p Code template parameter
         * 
         * The following table lists required argument types for each option.
         * Supplying wrong argument types will result in compile-time error.
         * 
         * @include{doc} db-options.md
         * 
         */
        template<int Code, class ...Args>
        auto config(Args && ...args) -> 
            //void but prevents instantiation with wrong types
            decltype(
              config_mapping<Code>::type::apply(*this, std::forward<decltype(args)>(args)...)
            )
            { config_mapping<Code>::type::apply(*this, std::forward<Args>(args)...); }

        //MARK: - create_module

        /** @{
         * @anchor modules
         * @name Virtual Table Modules
         */

        /**
         * Register a virtual table implementation
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * @param name name of the module
         * @param mod pointer to ::sqlite3_module "vtable"
         */
        void create_module(const string_param & name, const sqlite3_module * mod)
            { check_error(sqlite3_create_module_v2(c_ptr(), name.c_str(), mod, nullptr, nullptr)); }

        /**
         * Register a virtual table implementation
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * @param name name of the module
         * @param mod pointer to ::sqlite3_module "vtable"
         * @param data data to be passed to virtual table xCreate function.
         * @param destructor `void(*)(T *) noexcept` function (or anything convertible to such a pointer)
         *   to call when data is no longer needed. Can be omitted
         */
        template<typename T, typename D = void(*)(T *) noexcept>
        SQLITEPP_ENABLE_IF((std::is_convertible_v<D, void(*)(T *) noexcept>),
        void) create_module(const string_param & name, const sqlite3_module * mod, 
                           T * data, D destructor = nullptr)
            { check_error(sqlite3_create_module_v2(c_ptr(), name.c_str(), mod, (void*)data, (void (*)(void *))(void(*)(T *) noexcept)destructor)); }

        //MARK: -

        /**
         * Declare the schema of a virtual table
         * 
         * Equivalent to ::sqlite3_declare_vtab
         */
        void declare_vtab(const string_param & sdl)
            { check_error(sqlite3_declare_vtab(c_ptr(), sdl.c_str())); }

        /**
         * Configure virtual table
         * 
         * Wraps ::sqlite3_vtab_config
         * 
         * @tparam Code One of the SQLITE_VTAB_ options. Needs to be explicitly specified
         * @tparam Args depend on the @p Code template parameter
         * 
         * The following table lists required argument types for each option.
         * Supplying wrong argument types will result in compile-time error.
         * 
         * @include{doc} vtab-options.md
         * 
         */
        template<int Code, class ...Args>
        auto vtab_config(Args && ...args) -> 
            //void but prevents instantiation with wrong types
            decltype(
              vtab_config_mapping<Code>::type::apply(*this, std::forward<decltype(args)>(args)...)
            )
            { vtab_config_mapping<Code>::type::apply(*this, std::forward<Args>(args)...); }

        /**
         * Determine the virtual table conflict policy
         * 
         * Equivalent to ::sqlite3_vtab_on_conflict
         * 
         * @returns One of the [SQLITE_ROLLBACK, SQLITE_IGNORE, SQLITE_FAIL, 
         * SQLITE_ABORT, or SQLITE_REPLACE](https://www.sqlite.org/c3ref/c_fail.html)
         * conflict resolution modes
         */
        int vtab_on_conflict() const noexcept 
            { return sqlite3_vtab_on_conflict(c_ptr()); }

        //MARK: - drop_modules
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 30, 0)
        /**
         * Remove all virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules with nullptr second argument
         * 
         * @since SQLite 3.30
         */
        void drop_modules()
            { check_error(sqlite3_drop_modules(c_ptr(), nullptr)); }

        /**
         * Remove virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules
         * 
         * @since SQLite 3.30
         */                  
        void drop_modules_except(const char * const * keep)
            { check_error(sqlite3_drop_modules(c_ptr(), (const char **)keep)); }

        /**
         * Remove virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules
         * 
         * @since SQLite 3.30
         */      
        template<size_t N>
        SQLITEPP_ENABLE_IF(N > 0,
        void) drop_modules_except(const char * const (&keep)[N])
        {
            if (keep[N-1] != nullptr) throw exception(SQLITE_MISUSE);
            check_error(sqlite3_drop_modules(this->c_ptr(), (const char **)keep));
        }

        /**
         * Remove virtual table modules from database connection
         * 
         * Equivalent to ::sqlite3_drop_modules
         * 
         * @param args Any combination of `const char *` and `std::string` arguments
         * that specify names of the modules to keep 
         * 
         * @since SQLite 3.30
         */  
        template<class ...Args>
        SQLITEPP_ENABLE_IF((std::conjunction_v<std::is_convertible<Args, string_param>...>),
        void) drop_modules_except(Args && ...args)
        {
            const char * buf[] = {string_param(std::forward<Args>(args)).c_str() ..., nullptr};
            check_error(sqlite3_drop_modules(this->c_ptr(), buf));
        }
#endif

        /// @}

        //MARK: -

        /**
         * Enable or disable extended result codes
         * 
         * Equivalent to ::sqlite3_extended_result_codes
         */
        void extended_result_codes(bool onoff)
            { check_error(sqlite3_extended_result_codes(c_ptr(), onoff)); }

        /**
         * Low-level control of database file
         * 
         * Equivalent to ::sqlite3_file_control
         */
        void file_control(const string_param & db_name, int op, void * data)
            { check_error(sqlite3_file_control(c_ptr(), db_name.c_str(), op, data)); }

        /**
         * Return the filename for the database connection
         * 
         * Equivalent to ::sqlite3_db_filename
         */
        const char * filename(const string_param & db_name) const noexcept
        {
            auto ret = sqlite3_db_filename(c_ptr(), db_name.c_str());
            return ret ? ret : "";
        }

        /**
         * Return the auto-commit mode
         * 
         * Equivalent to ::sqlite3_get_autocommit
         */
        bool get_autocommit() const noexcept
            { return sqlite3_get_autocommit(c_ptr()); }

        /**
         * Interrupt a long-running query
         * 
         * Equivalent to ::sqlite3_interrupt
         */
        void interrupt() noexcept
            { sqlite3_interrupt(c_ptr()); }

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 41, 0)
        /**
         * Returns whether or not an interrupt is currently in effect
         * 
         * Equivalent to ::sqlite3_is_interrupted
         * 
         * @since SQLite 3.41
         */
        bool is_interrupted() noexcept
            { return sqlite3_is_interrupted(c_ptr()) != 0; }
    #endif

        /**
         * Returns last insert rowid
         * 
         * Equivalent to ::sqlite3_last_insert_rowid
         */
        int64_t last_insert_rowid() const noexcept
            { return sqlite3_last_insert_rowid(c_ptr()); }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 18, 0)
        /** 
         * Set the last insert rowid value
         * 
         * Equivalent to ::sqlite3_set_last_insert_rowid 
         * 
         * @since SQLite 3.18
         */
        void set_last_insert_rowid(int64_t value) noexcept
            { sqlite3_set_last_insert_rowid(c_ptr(), value); }
#endif

        //returns -1 on bad limit or other issues
        /**
         * Set or retrieve run-time limits
         * 
         * Equivalent to ::sqlite3_limit
         * 
         * @param id one of the [limit categories](https://www.sqlite.org/c3ref/c_limit_attached.html)
         * @param new_val new value or -1 to query
         * @returns prior value of the limit or -1 on bad limit or other issues
         */
        int limit(int id, int new_val) noexcept
            { return sqlite3_limit(c_ptr(), id, new_val); }

        /** @{
         * @name Extension management
         */

        /**
         * Enable or disable extension loading
         * 
         * Equivalent to ::sqlite3_enable_load_extension
         */
        void enable_load_extension(bool val)
            { check_error(call_sqlite3_enable_load_extension(this->c_ptr(), val)); }

        /**
         * Load an extension
         * 
         * Equivalent to ::sqlite3_load_extension
         */
        void load_extension(const string_param & file, const string_param & proc = nullptr);

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 8, 7)

        /**
         * Automatically load statically linked extension
         * 
         * Equivalent to ::sqlite3_auto_extension
         * 
         * @since SQLite 3.8.7
         */
        void auto_extension(void(*entry_point)(database *, const char **, const struct sqlite3_api_routines *))
            { check_error(sqlite3_auto_extension((void(*)(void))entry_point)); }

        /**
         * Cancel automatic extension Loading
         * 
         * Equivalent to ::sqlite3_cancel_auto_extension
         * 
         * @since SQLite 3.8.7
         */
        void cancel_auto_extension(void(*entry_point)(database *, const char **, const struct sqlite3_api_routines *))
            { check_error(sqlite3_cancel_auto_extension((void(*)(void))entry_point)); }

        /**
         * Reset automatic extension loading
         * 
         * Equivalent to ::sqlite3_reset_auto_extension
         * 
         * @since SQLite 3.8.7
         */
        void reset_auto_extension() noexcept
            { sqlite3_reset_auto_extension(); }

    #endif

        /** @} */

        /**
         * Retrieve the mutex for the database connection
         * 
         * Equivalent to ::sqlite3_db_mutex
         */
        class mutex * mutex() const noexcept
            { return (class mutex *)sqlite3_db_mutex(c_ptr()); }

        //MARK: - next_statement

        /**
         * Find the next prepared statement
         * 
         * Equivalent to ::sqlite3_next_stmt
         */
        const class statement * next_statement(const class statement * prev) const noexcept
            { return (class statement *)sqlite3_next_stmt(c_ptr(), (sqlite3_stmt *)prev); }

        /// @overload
        class statement * next_statement(const class statement * prev) noexcept
            { return (class statement *)sqlite3_next_stmt(c_ptr(), (sqlite3_stmt *)prev); }

        //MARK: -

        /**
         * Overload a function for a virtual table
         * 
         * Equivalent to ::sqlite3_overload_function
         */
        void overload_function(const string_param & name, int arg_count)
            { check_error(sqlite3_overload_function(c_ptr(), name.c_str(), arg_count)); }

        //MARK: - progress_handler

        /**
         * Register a callback to be called on query progress
         * 
         * Equivalent to ::sqlite3_progress_handler
         * 
         * @param step_count An approximate number of 
         * [virtual machine instructions](https://www.sqlite.org/opcode.html)
         * that are evaluated between successive invocations of the callback.
         * If less than one then the progress handler is disabled.
         * @param handler A callback function that matches the type of @p data_ptr argument. Can be
         *  nullptr.
         * @param data_ptr A pointer to callback data or nullptr.
         * 
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T> || std::is_null_pointer_v<T>,
        void) progress_handler(int step_count, int(*handler)(type_identity_t<T> data_ptr) noexcept, T data_ptr) const noexcept
            { sqlite3_progress_handler(this->c_ptr(), step_count, (int(*)(void*))handler, data_ptr); }

        /**
         * Register a callback to be called on query progress
         * 
         * Equivalent to ::sqlite3_progress_handler
         * 
         * @param step_count An approximate number of 
         * [virtual machine instructions](https://www.sqlite.org/opcode.html)
         * that are evaluated between successive invocations of the callback.
         * If less than one then the progress handler is disabled.
         * @param handler_ptr A **pointer** to any C++ callable that can be invoked as
         * ```
         * (*handler_ptr)();
         * ```
         * This invocation must be `noexcept`. 
         * This parameter can also be nullptr to reset the handler.
         * The handler object must exist as long as it is set.
         */
        template<class T>
        SQLITEPP_ENABLE_IF((std::is_null_pointer_v<T> ||
            (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<bool, std::remove_pointer_t<T>>)),
        void) progress_handler(int step_count, T handler_ptr) const noexcept;

        //MARK: -
        /**
         * Determine if a database is read-only
         * 
         * Equivalent to ::sqlite3_db_readonly
         * 
         * @param db_name database name
         * @return `true` if the database named @p db_name is readonly, `false` if it is
         * read/write or `std::nullopt` if @p db_name is not a name of a database on this
         * connection.
         */
        std::optional<bool> readonly(const string_param & db_name) const noexcept;

        /**
         * Free memory used by the database connection
         * 
         * Equivalent to ::sqlite3_db_release_memory
         */
        void release_memory() const
            { check_error(sqlite3_db_release_memory(c_ptr())); }

        //MARK: - status

        /// Return type for @ref status()
        struct status
        {
            int current;
            int high;
        };

    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 51, 0)
        [[deprecated("use status64")]]
    #endif
        /**
         * Retrieve database connection status
         * 
         * Equivalent to ::sqlite3_db_status
         */
        struct status status(int op, bool reset = false) const;

    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 51, 1)

        /// Return type for @ref status64()
        struct status64
        {
            sqlite3_int64 current;
            sqlite3_int64 high;
        };

        /**
         * Retrieve database connection status
         * 
         * Equivalent to ::sqlite3_db_status64
         */
        struct status64 status64(int op, bool reset = false) const;

    #endif

        //MARK: - table_column_metadata

        /// Return type for table_column_metadata()
        struct column_metadata
        {
            const char * data_type;             ///< Declared data type
            const char * collation_sequence;    ///< Collation sequence name
            bool not_null;                      ///< Whether NOT NULL constraint exists
            bool primary_key;                   ///< Whether column part of PK
            bool auto_increment;                ///< Whether column is auto-increment
        };

        /**
         * Extract metadata about a column of a table
         * 
         * Equivalent to ::sqlite3_table_column_metadata
         */
        column_metadata table_column_metadata(const string_param & db_name, 
                                              const string_param & table_name, 
                                              const string_param & column_name) const;

        //MARK: -

        /**
         * Returns total number of rows modified
         * 
         * Equivalent to ::sqlite3_total_changes
         */
        int64_t total_changes() const noexcept
        { 
        #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 36, 1)
            return sqlite3_total_changes64(c_ptr()); 
        #else
            return sqlite3_total_changes(c_ptr()); 
        #endif
        }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 34, 0)

        /**
         * Returns the transaction state of a database
         * 
         * Equivalent to ::sqlite3_txn_state
         * 
         * @since SQLite 3.34
         */
        int txn_state(const string_param & schema) const noexcept
            { return sqlite3_txn_state(c_ptr(), schema.c_str()); }
#endif

        /**
         * Open a blob
         * 
         * Equivalent to ::sqlite3_blob_open
         */
        std::unique_ptr<blob> open_blob(const string_param & dbname, 
                                        const string_param & table,
                                        const string_param & column,
                                        int64_t rowid,
                                        bool writable);

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 39, 0)

        /** @{
         * @name Serialization
         */

        /**
         * Serialize a database 
         * 
         * Equivalent to ::sqlite3_serialize with flags set 0
         * 
         * @since SQLite 3.39
         */
        std::pair<allocated_bytes, size_t> serialize(const string_param & schema_name);

        /**
         * Serialize a database 
         * 
         * Equivalent to ::sqlite3_serialize with flags set SQLITE_SERIALIZE_NOCOPY
         * 
         * @since SQLite 3.39
         */
        span<std::byte> serialize_reference(const string_param & schema_name) noexcept;

        /**
         * Deserialize a database
         * 
         * Equivalent to ::sqlite3_deserialize
         * 
         * @since SQLite 3.39
         */
        void deserialize(const string_param & schema_name, 
                         std::byte * buf, 
                         size_t size, 
                         size_t buf_size,
                         unsigned flags = 0)
            { check_error(sqlite3_deserialize(c_ptr(), schema_name.c_str(), (unsigned char *)buf, int64_size(size), int64_size(buf_size), flags)); }

        /**
         * Deserialize a database
         * 
         * A convenience overload for immutable data
         * 
         * Equivalent to ::sqlite3_deserialize with SQLITE_DESERIALIZE_READONLY flag always added
         * 
         * @since SQLite 3.39
         */
        void deserialize(const string_param & schema_name, 
                         const std::byte * buf, 
                         size_t size, 
                         size_t buf_size,
                         unsigned flags = 0)
            { deserialize(schema_name, (std::byte *)buf, size, buf_size, flags | SQLITE_DESERIALIZE_READONLY); }

        /**
         * Deserialize a database
         * 
         * A convenience overload that takes ownership over passed pointer
         * 
         * Equivalent to ::sqlite3_deserialize with SQLITE_DESERIALIZE_FREEONCLOSE flag always added
         * 
         * @since SQLite 3.39
         */
        void deserialize(const string_param & schema_name, 
                         allocated_bytes buf, 
                         size_t size, 
                         size_t buf_size,
                         unsigned flags = 0);

        /** @} */
#endif

        /** @{
         * @name Snapshots
         */

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) && (THINSQLITEPP_ENABLE_EXPERIMENTAL || THINSQLITEPP_ENABLE_EXPIREMENTAL)
        /**
         * Record a database snapshot
         * 
         * Equivalent to ::sqlite3_snapshot_get
         * 
         * Requires THINSQLITEPP_ENABLE_EXPERIMENTAL macro defined to 1 as the underlying SQLite
         * feature is experimental.
         * 
         * @since SQLite 3.10
         */
        std::unique_ptr<snapshot> get_snapshot(const string_param & schema);

        /**
         * Start a read transaction on an historical snapshot
         * 
         * Equivalent to ::sqlite3_snapshot_open
         * 
         * Requires THINSQLITEPP_ENABLE_EXPERIMENTAL macro defined to 1 as the underlying SQLite
         * feature is experimental.
         * 
         * @since SQLite 3.10
         */
        void open_snapshot(const string_param & schema, const snapshot & snap)
            { check_error(sqlite3_snapshot_open(c_ptr(), schema.c_str(), snap.c_ptr())); }

        /**
         * Recover snapshots from a wal file
         * 
         * Equivalent to ::sqlite3_snapshot_recover
         * 
         * Requires THINSQLITEPP_ENABLE_EXPERIMENTAL macro defined to 1 as the underlying SQLite
         * feature is experimental.
         * 
         * @since SQLite 3.10
         */
        void recover_snapshot(const string_param & db)
            { check_error(sqlite3_snapshot_recover(c_ptr(), db.c_str())); }

#endif

        /** @} */

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 39, 0)
        /**
         * Return schema names
         *
         * Equivalent to ::sqlite3_db_name
         * 
         * @param idx Schema index. 0 means the main database file and 1 is 
         * the "temp" schema. Larger values correspond to various ATTACH-ed databases.
         * 
         * @since SQLite 3.39
         */
        const char * db_name(int idx) noexcept
            { return sqlite3_db_name(c_ptr(), idx); }

    #endif

        //MARK: - Private methods

    private:
        void check_error(int res) const
        {
            if (res != SQLITE_OK)
                throw exception(res, this);
        }

        template<class T>
        static int call_sqlite3_enable_load_extension(T * db, int onoff)
        {
            if constexpr (database_detector::has_enable_load_extension<T>)
                return sqlite3_enable_load_extension(db, onoff);
            else
                return SQLITE_ERROR;
        }

        template<class T>
        static int call_sqlite3_load_extension(T * db, const char * file, const char * proc, char ** err)
        {
            if constexpr (database_detector::has_load_extension<T>)
                return sqlite3_load_extension(db, file, proc, err);
            else
                return SQLITE_ERROR;
        }
    };

    /** @} */

    /** @cond PRIVATE */

    SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN

    #if SQLITEPP_USE_VARARG_POUND_POUND_TRICK

        #define SQLITEPP_DEFINE_DB_OPTION(code, ...) \
            template<> struct database::config_mapping<code> { using type = database::config_option<code, ##__VA_ARGS__>; };

        //Idiotic GCC in pedantic mode warns on MACRO(arg) for MARCO(x,...) in < C++20 mode
        //with no way to disable the warning(!!!). 
        #define SQLITEPP_DEFINE_VTAB_OPTION_0(code) \
            template<> struct database::vtab_config_mapping<code> { using type = database::vtab_config_option<code>; };
        #define SQLITEPP_DEFINE_VTAB_OPTION_N(code, ...) \
            template<> struct database::vtab_config_mapping<code> { using type = database::vtab_config_option<code, ##__VA_ARGS__>; };

    #else

        #define SQLITEPP_DEFINE_DB_OPTION(code, ...) \
            template<> struct database::config_mapping<code> { using type = database::config_option<code __VA_OPT__(,) __VA_ARGS__>; };

        #define SQLITEPP_DEFINE_VTAB_OPTION_N(code, ...) \
            template<> struct database::vtab_config_mapping<code> { using type = database::vtab_config_option<code __VA_OPT__(,) __VA_ARGS__>; };

        #define SQLITEPP_DEFINE_VTAB_OPTION_0(code) SQLITEPP_DEFINE_VTAB_OPTION_N(code)

    #endif

    //@ [DB Options]
#ifdef SQLITE_DBCONFIG_MAINDBNAME
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_MAINDBNAME,              const char *);
#endif
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_LOOKASIDE,               void *, int, int);
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_FKEY,             int, int *);
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_TRIGGER,          int, int *);
#ifdef SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER,   int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION,   int, int *);
#endif
#ifdef SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE,        int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_QPSG
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_QPSG,             int, int *);
#endif
#ifdef SQLITE_DBCONFIG_TRIGGER_EQP
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_TRIGGER_EQP,             int, int *);
#endif
#ifdef SQLITE_DBCONFIG_RESET_DATABASE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_RESET_DATABASE,          int, int *);
#endif
#ifdef SQLITE_DBCONFIG_DEFENSIVE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_DEFENSIVE,               int, int *);
#endif
#ifdef SQLITE_DBCONFIG_WRITABLE_SCHEMA
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_WRITABLE_SCHEMA,         int, int *);
#endif
#ifdef SQLITE_DBCONFIG_LEGACY_ALTER_TABLE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_LEGACY_ALTER_TABLE,      int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_VIEW
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_VIEW,             int, int *);
#endif
#ifdef SQLITE_DBCONFIG_LEGACY_FILE_FORMAT
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_LEGACY_FILE_FORMAT,      int, int *);
#endif
#ifdef SQLITE_DBCONFIG_TRUSTED_SCHEMA
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_TRUSTED_SCHEMA,          int, int *);
#endif
#ifdef SQLITE_DBCONFIG_STMT_SCANSTATUS
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_STMT_SCANSTATUS,         int, int *);
#endif
#ifdef SQLITE_DBCONFIG_REVERSE_SCANORDER
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_REVERSE_SCANORDER,       int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_ATTACH_CREATE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_ATTACH_CREATE,    int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_ATTACH_WRITE
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_ATTACH_WRITE,     int, int *);
#endif
#ifdef SQLITE_DBCONFIG_ENABLE_COMMENTS
    SQLITEPP_DEFINE_DB_OPTION( SQLITE_DBCONFIG_ENABLE_COMMENTS,         int, int *);
#endif
    //@ [DB Options]

    //@ [VTab Options]

    SQLITEPP_DEFINE_VTAB_OPTION_N(SQLITE_VTAB_CONSTRAINT_SUPPORT,       int);
#ifdef SQLITE_VTAB_INNOCUOUS
    SQLITEPP_DEFINE_VTAB_OPTION_0(SQLITE_VTAB_INNOCUOUS                 );
#endif
#ifdef SQLITE_VTAB_DIRECTONLY
    SQLITEPP_DEFINE_VTAB_OPTION_0(SQLITE_VTAB_DIRECTONLY                );
#endif
#ifdef SQLITE_VTAB_USES_ALL_SCHEMAS
    SQLITEPP_DEFINE_VTAB_OPTION_0(SQLITE_VTAB_USES_ALL_SCHEMAS          );
#endif

    //@ [VTab Options]

    #undef SQLITEPP_DEFINE_DB_OPTION
    #undef SQLITEPP_DEFINE_VTAB_OPTION_0
    #undef SQLITEPP_DEFINE_VTAB_OPTION_N

    SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END

    /** @endcond */
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

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
    SQLITEPP_EXPORTED
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

namespace thinsqlitepp
{
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Dynamically Typed Value Object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_value. 
     * 
     * `#include <thinsqlitepp/value.hpp>`
     */
    SQLITEPP_EXPORTED
    class value final : public handle<sqlite3_value, value>
    {
    public:
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 18, 0)
        /**
         * Creates a new value by copying an original one
         * 
         * Equivalent to ::sqlite3_value_dup
         * 
         * @since SQLite 3.18
         * 
         * @param src Original @ref value. Can be nullptr
         * @returns A new @ref value object which is a copy of the original
         * or nullptr if the original is nullptr
         */
        static std::unique_ptr<value> dup(const value * src)
        {
            auto ret = sqlite3_value_dup(src ? src->c_ptr() : nullptr);
            if (!ret && src)
                throw exception(SQLITE_NOMEM);
            return std::unique_ptr<value>((value *)ret);
        }

        /// @overload
        static std::unique_ptr<value> dup(const std::unique_ptr<value> & src) 
            { return dup(src.get()); }

        /**  
         * Equivalent to ::sqlite3_value_free
         * 
         * @since SQLite 3.18
         */
        ~value() noexcept
            { sqlite3_value_free(c_ptr()); }
#endif

    private:
        template<typename T>
        static constexpr bool supported_column_type = 
            std::is_same_v<T, int> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string_view> ||
        #if __cpp_char8_t >= 201811
            std::is_same_v<T, std::u8string_view> ||
        #endif
            std::is_same_v<T, blob_view>;

    public:

        /**
         * Obtain value's content 
         * 
         * Wraps @ref sqlite3_value_ function family. Unlike the C API you specify the
         * desired type via T template parameter
         * 
         * @tparam T Desired output type. Must be one of:
         * - int
         * - int64_t
         * - double
         * - std::string_view
         * - std::u8string_view (if `char8_t` is supported by your compiler/library)
         * - blob_view
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_column_type<T>,
        T) get() const noexcept;

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)

        /**
         * Obtain a pointer stored in the value
         * 
         * Wraps @ref sqlite3_value_pointer function. 
         * 
         * @param type the "type name" of the stored pointer. If nullptr 
         * the result of `typeid(T).name()` is used.
         * 
         * @see 
         * - @ref bind_pointer "statement::bind(int, T * , const char * , void( * )(T * ))"
         * - @ref "statement::bind(int, std::unique_ptr<T>)"
         * - @ref result_pointer "context::result(T * , const char * , void( * )(T * ))"
         * - @ref "context::result(std::unique_ptr<T>)"
         * 
         * @since SQLite 3.20
         */
        template<class T>
        SQLITEPP_ENABLE_IF(std::is_pointer_v<T>,
        T) get(const char * type = nullptr) const noexcept 
            { return static_cast<T>(sqlite3_value_pointer(c_ptr(), type ? type : typeid(T).name())); }

    #endif

        /**
         * Default datatype of the value
         * 
         * Equivalent to ::sqlite3_value_type
         * 
         * @returns One of the SQLite [datatype constants](https://www.sqlite.org/c3ref/c_blob.html)
         */
        int type() const noexcept
            { return sqlite3_value_type(c_ptr()); }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 9, 0)
        /**
         * Subtype of the value
         * 
         * Equivalent to ::sqlite3_value_subtype
         * 
         * @since SQLite 3.9
         */
        unsigned subtype()const noexcept
            { return sqlite3_value_subtype(c_ptr()); }
#endif

        /**
         * Best numeric datatype of the value
         * 
         * Equivalent to ::sqlite3_value_numeric_type
         * 
         * @returns One of the SQLite [datatype constants](https://www.sqlite.org/c3ref/c_blob.html)
         */
        int numeric_type() const noexcept
            { return sqlite3_value_numeric_type(c_ptr()); }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 22, 0)
        /**
         * Whether the column is unchanged in an UPDATE against a virtual table.
         * 
         * Equivalent to ::sqlite3_value_nochange
         * 
         * @since SQLite 3.22
         */
        bool nochange() const noexcept
            { return sqlite3_value_nochange(c_ptr()); }
#endif

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 28, 0)
        /**
         * Whether if value originated from a [bound parameter](https://www.sqlite.org/lang_expr.html#varparam)
         * 
         * Equivalent to ::sqlite3_value_frombind
         * 
         * @since SQLite 3.28
         */
        bool frombind() const noexcept
            { return sqlite3_value_frombind(c_ptr()); }
#endif

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 38, 0)
        /**
         * Get first element on the right-hand side of an IN constraint
         * 
         * Equivalent to ::sqlite3_vtab_in_first
         */
        value * in_first() const 
        {
            sqlite3_value * ret;
            int res = sqlite3_vtab_in_first(c_ptr(), &ret);
            if (res != SQLITE_OK && res != SQLITE_DONE)
                throw exception(res);
            return from(ret);
        }

        /**
         * Get next element on the right-hand side of an IN constraint
         * 
         * Equivalent to ::sqlite3_vtab_in_next
         */
        value * in_next() const 
        {
            sqlite3_value * ret;
            int res = sqlite3_vtab_in_next(c_ptr(), &ret);
            if (res != SQLITE_OK && res != SQLITE_DONE)
                throw exception(res);
            return from(ret);
        }
#endif
    };

    /** @} */

    /// @cond PRIVATE

    template<>
    inline int value::get<int>() const noexcept
        { return sqlite3_value_int(c_ptr()); }

    template<>
    inline int64_t value::get<int64_t>() const noexcept
        { return sqlite3_value_int64(c_ptr()); }

    template<>
    inline std::string_view value::get<std::string_view>() const noexcept
    {
        auto first = (const char *)sqlite3_value_text(c_ptr());
        auto size = (size_t)sqlite3_value_bytes(c_ptr());
        return std::string_view(first, size);
    }

#if __cpp_char8_t >= 201811
    template<>
    inline std::u8string_view value::get<std::u8string_view>() const noexcept
    {
        auto first = (const char8_t *)sqlite3_value_text(c_ptr());
        auto size = (size_t)sqlite3_value_bytes(c_ptr());
        return std::u8string_view(first, size);
    }
#endif

    template<>
    inline double value::get<double>() const noexcept
        { return sqlite3_value_double(c_ptr()); }

    template<>
    inline blob_view value::get<blob_view>() const noexcept
    {
        auto first = (const std::byte *)sqlite3_value_blob(c_ptr());
        auto size = sqlite3_value_bytes(c_ptr());
        return blob_view(first, first + size);
    }

    /// @endcond
}

namespace thinsqlitepp
{
    SQLITEPP_EXPORTED class database;

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * SQL Function Context Object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_context.
     * 
     * `#include <thinsqlitepp/context.hpp>`
     * 
     */
    SQLITEPP_EXPORTED
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
         * 
         * Note that passing a string_view longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         */
        void error(const std::string_view & value) noexcept
            { sqlite3_result_error(c_ptr(), value.size() ? &value[0] : "", int_size(value.size())); }
    #if __cpp_char8_t >= 201811
        /// @overload
        void error(const std::u8string_view & value) noexcept
            { sqlite3_result_error(c_ptr(), value.size() ? (const char *)&value[0] : "", int_size(value.size())); }
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
         * 
         * Note that passing a string_view longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         */
        void result(const std::string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), data, int_size(value.size()), SQLITE_TRANSIENT);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #if __cpp_char8_t >= 201811
        /// @overload
        void result(const std::u8string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), (const char *)data, int_size(value.size()), SQLITE_TRANSIENT);
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
         * 
         * Note that passing a string_view longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         */
        void result_reference(const std::string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), data, int_size(value.size()), SQLITE_STATIC);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #if __cpp_char8_t >= 201811
        /// @overload
        void result_reference(const std::u8string_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_text(c_ptr(), (const char *)data, int_size(value.size()), SQLITE_STATIC);
            else
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
        }
    #endif
        /**
         * Return a string by reference from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_text(..., unref)
         * 
         * The string content is used **by reference**. 
         * 
         * Note that passing a string_view longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         * 
         * @param value reference to string to return
         * @param unref called when the reference is no longer needed.
         * Its argument will be the pointer returned from `value.data()`
         */
        void result_reference(const std::string_view & value, void (*unref)(const char *) noexcept) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
            {
                sqlite3_result_text(c_ptr(), data, int_size(value.size()), (void (*)(void*))unref);
            }
            else 
            {
                unref(nullptr);
                sqlite3_result_text(c_ptr(), "", 0, SQLITE_STATIC);
            }
        }

    #if __cpp_char8_t >= 201811
        /// @overload
        void result_reference(const std::u8string_view & value, void (*unref)(const char8_t *) noexcept) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
            {
                sqlite3_result_text(c_ptr(), (const char *)data, int_size(value.size()), (void (*)(void *))unref);
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
         * 
         * Note that passing a blob longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         */
        void result(const blob_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_blob(c_ptr(), data, int_size(value.size()), SQLITE_TRANSIENT);
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
         * 
         * Note that passing a blob longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         */
        void result_reference(const blob_view & value) noexcept
        {
            //passing a null pointer to sqlite3_result_ returns NULL not zero length text
            if (auto data = value.data())
                sqlite3_result_blob(c_ptr(), data, int_size(value.size()), SQLITE_STATIC);
            else
                sqlite3_result_zeroblob(c_ptr(), 0);
        }

        /**
         * Return a blob by reference from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_blob(..., unref)
         * 
         * The blob content is used **by reference**. 
         * 
         * Note that passing a blob longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
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
                sqlite3_result_blob(c_ptr(), data, int_size(value.size()), (void (*)(void *))unref);
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
         * Note that passing a blob longer than std::numeric_limits<int>::max()
         * will result in std::terminate().
         * 
         */
        void result(const zero_blob & value) noexcept
            { sqlite3_result_zeroblob(c_ptr(), int_size(value.size())); }

        ///@anchor result_pointer
        /**
         * 
         * Return a custom pointer from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_pointer()
         * 
         * @tparam D Anything convertible to `void(*)(T *) noexcept`.
         *   Can be a function pointer or a matching lambda.
         * 
         */
        template<class T, class D>
        SQLITEPP_ENABLE_IF((std::is_convertible_v<D, void(*)(T *) noexcept>),
        void) result(T * ptr, const char * type, D destroy) noexcept
            { sqlite3_result_pointer(this->c_ptr(), ptr, type, (void(*)(void*))(void(*)(T *))destroy); }

        /**
         * Return a custom pointer from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_pointer()
         * 
         * This is a safer overload of 
         * @ref result_pointer "result(T * , const char * , void( * )(T * ))"
         * that takes a pointer via std::unique_ptr ownership transfer. The inferred
         * "type" for ::sqlite3_result_pointer is `typeid(T).name()`.
         */
        template<class T>
        void result(std::unique_ptr<T> ptr) noexcept
            { this->result(ptr.release(), typeid(T).name(), [](T * p) noexcept { delete p;}); }

        /**
         * Return a copy of the passed @ref value from the implemented SQL function.
         * 
         * Equivalent to ::sqlite3_result_value
         */
        void result(const value & val) noexcept
            { sqlite3_result_value(c_ptr(), val.c_ptr()); }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 9, 0)
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

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 22, 0)
        /**
         * Return if a value being fetched as part of an UPDATE operation during which the column value will not change.
         * 
         * Equivalent to ::sqlite3_vtab_nochange
         */
        bool vtab_nochange() const noexcept 
            { return sqlite3_vtab_nochange(c_ptr()); }
#endif
    };

    /** @} */

}

#if __has_include(<sys/uio.h>)
#endif

struct iovec;

namespace thinsqlitepp
{
    SQLITEPP_EXPORTED class database;
    SQLITEPP_EXPORTED class value;

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * A fallback version of struct iovec for platforms that lack it.
     * 
     * @see thinsqlitepp::iovec
     */
    SQLITEPP_EXPORTED
    struct iovec_fallback {
        /// @brief Base address of a memory region for input or output. 
        void *iov_base;
        /// @brief The size of the memory pointed to by iov_base. 
        size_t iov_len;
    };

    template<class T=struct ::iovec, size_t = sizeof(T)>
    constexpr struct ::iovec * detect_iovec(T *) { return{}; }
    constexpr auto             detect_iovec(...) { return (iovec_fallback *)nullptr; }

    /**
     * A portable version of struct iovec that SQLite uses for carray() functionality.
     * 
     * On Posix platforms this is a typedef to **struct iovec** from 
     * [`<sys/uio.h>`](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_uio.h.html)
     * On other platforms this is a typedef to the `thinsqlitepp::iovec_fallback`.
     *  
     * 
     * Thus, for portability, you can use thinsqlitepp::iovec on any platform
     */
    SQLITEPP_EXPORTED
    using iovec = std::remove_reference_t<decltype(*detect_iovec((::iovec *)nullptr))>;

    /**
     * Prepared Statement Object
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_stmt.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     * 
     */
    SQLITEPP_EXPORTED
    class statement final : public handle<sqlite3_stmt, statement>
    {
    public:
        /**
         * Compile an SQL statement
         * 
         * This is a wrapper over ::sqlite3_prepare_v3 or ::sqlite3_prepare_v2, if the former is
         * not available.
         * 
         * @param db The database to create statement for
         * @param sql The statement to be compiled. Must be in UTF-8.
         * @param flags Zero or more SQLITE_PREPARE_ flags. Only available for SQLite 3.2 or greater
         */
        static std::unique_ptr<statement> create(const database & db, const string_param & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 );

        /**
         * Compile an SQL statement
         * 
         * This is a wrapper over ::sqlite3_prepare_v3 or ::sqlite3_prepare_v2, if the former is
         * not available.
         * 
         * @param db The database to create statement for
         * @param sql The statement to be compiled. Must be in UTF-8. This is an input-output parameter.
         *            On output the string_view is adjusted to contain any text past the end of the first SQL statement.
         *            See `pzTail` argument description for ::sqlite3_prepare_v3
         * @param flags Zero or more SQLITE_PREPARE_ flags. Only available for SQLite 3.2 or greater
         */
        static std::unique_ptr<statement> create(const database & db, std::string_view & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 );

#if __cpp_char8_t >= 201811
        /**
         * Compile an SQL statement
         * 
         * char8_t overload for create(const database &, const string_param &, unsigned int)
         */ 
        static std::unique_ptr<statement> create(const database & db, const u8string_param & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 )
        {
            return create(db, (const char *)sql.c_str()
                        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                          , flags
                        #endif
                   );
        }

        /**
         * Compile an SQL statement
         * 
         * char8_t overload for create(const database &, std::string_view &, unsigned int)
         */
        static std::unique_ptr<statement> create(const database & db, std::u8string_view & sql
                                            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                 , unsigned int flags = 0
                                            #endif
                                                 )
        {
            return create(db, *reinterpret_cast<std::string_view *>(&sql)
                        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                          , flags
                        #endif
                   );
        }
#endif

        /// Equivalent to ::sqlite3_finalize
        ~statement() noexcept
            { sqlite3_finalize(c_ptr()); }

        /**
         * Returns the database to which this statement belongs
         * 
         * Equivalent to ::sqlite3_db_handle
         */
        class database & database() const noexcept
            { return *(class database *)sqlite3_db_handle(c_ptr()); }

        /**
         * Evaluate the statement
         * 
         * Equivalent to ::sqlite3_step.
         * 
         * Returns true if a row was retrieved (#SQLITE_ROW) or false if the 
         * statement has finished executing successfully (#SQLITE_DONE).
         * 
         * All other ::sqlite3_step return codes result in @ref exception being thrown
         */
        bool step();

        /**
         * Reset the statement
         * 
         * Equivalent to ::sqlite3_reset
         */
        void reset() noexcept
            { sqlite3_reset(c_ptr()); }

        /**
         * Determine if the statement has been reset
         * 
         * Equivalent to ::sqlite3_stmt_busy
         */
        bool busy() const noexcept
            { return sqlite3_stmt_busy(c_ptr()); }

        /** 
         * Return type for isexplain()
         */ 
        enum class explain_type : int
        {
            not_explain = 0,        ///< The statement is an ordinary statement
            explain = 1,            ///< The statement is an EXPLAIN statement
            explain_query_plan = 2  ///< The statement is an EXPLAIN QUERY PLAN
        };

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 31, 1)
        /**
         * Query the EXPLAIN Setting for the statement
         * 
         * Equivalent to ::sqlite3_stmt_isexplain. 
         * 
         * @since SQLite 3.31
         */
        explain_type isexplain() const noexcept
            { return explain_type(sqlite3_stmt_isexplain(c_ptr())); }
#endif

        /**
         * Determine if the statement writes to the database
         * 
         * Equivalent to ::sqlite3_stmt_readonly
         */
        bool readonly() const noexcept
            { return sqlite3_stmt_readonly(c_ptr()); }

        /** @{
         * @anchor statement_bind
         * @name Binding values to parameters
         * 
         * This set of overloaded functions wraps @ref sqlite3_bind_ function
         * group. 
         */

        /**
         * Bind a NULL value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_null
         */
        void bind(int idx, std::nullptr_t)
            { check_error(sqlite3_bind_null(c_ptr(), idx)); }
        /**
         * Bind an int value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_int
         */
        void bind(int idx, int val)
            { check_error(sqlite3_bind_int(c_ptr(), idx, val)); }
        /**
         * Bind an int64_t value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_int64
         */
        void bind(int idx, int64_t val)
            { check_error(sqlite3_bind_int64(c_ptr(), idx, val)); }
        /**
         * Bind a double value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_double
         */
        void bind(int idx, double val)
            { check_error(sqlite3_bind_double(c_ptr(), idx, val)); }

        /**
         * Bind a string value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_text with #SQLITE_TRANSIENT.
         * 
         * The string content is used **by value** and copied into the statement.
         * Thus the lifetime of the string referred to by `value` parameter is 
         * independent of the statement's
         */
        void bind(int idx, const std::string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_text with #SQLITE_STATIC.
         * 
         * The string content is used **by reference**.
         * Thus the string referred to by `value` parameter must
         * remain valid during this statement's lifetime.
         */
        void bind_reference(int idx, const std::string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_text(..., unref)
         * 
         * The string content is used **by reference**.
         * 
         * @param idx index of the SQL parameter to be bound 
         * @param val reference to string to bind to the parameter
         * @param unref called when the reference is no longer needed. 
         * Its argument is the pointer returned from `value.data()`
         */
        void bind_reference(int idx, const std::string_view & val, void (*unref)(const char *) noexcept);

    #if __cpp_char8_t >= 201811
        /**
         * Bind a string value to a parameter of the statement
         * 
         * char8_t overload for bind(int, const std::string_view &)
         */
        void bind(int idx, const std::u8string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * char8_t overload for bind_reference(int, const std::string_view &)
         */
        void bind_reference(int idx, const std::u8string_view & val);

        /**
         * Bind a string reference to a parameter of the statement
         * 
         * char8_t overload for bind_reference(int, const std::string_view &, void (*)(const char *))
         */
        void bind_reference(int idx, const std::u8string_view & val, void (*unref)(const char8_t *) noexcept);
    #endif

        /**
         * Bind a blob value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_blob with #SQLITE_TRANSIENT.
         * 
         * The blob content is used **by value** and copied into the statement.
         * Thus the lifetime of the blob referred to by `value` parameter is 
         * independent of the statement's
         */
        void bind(int idx, const blob_view & val);

        /**
         * Bind a blob reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_blob with #SQLITE_STATIC.
         * 
         * The blob content is used **by reference**.
         * Thus the string referred to by `value` parameter must
         * remain valid during this statement's lifetime.
         */
        void bind_reference(int idx, const blob_view & val);

        /**
         * Bind a blob reference to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_blob (..., unref)
         * 
         * The blob content is used **by reference**.
         * 
         * @param idx index of the SQL parameter to be bound 
         * @param val reference to blob to bind to the parameter
         * @param unref called when the reference is no longer needed. 
         * Its argument is the pointer returned from `value.data()`
         */
        void bind_reference(int idx, const blob_view & val, void (*unref)(const std::byte *) noexcept);

        /**
         * Bind a blob of zeroes to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_zeroblob.
         */
        void bind(int idx, const zero_blob & val)
            { check_error(sqlite3_bind_zeroblob(c_ptr(), idx, int(val.size()))); }

        ///@anchor bind_pointer
        /**
         * Bind a custom pointer to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_pointer. 
         * 
         * The `type` parameter should be a static string, preferably a string literal.
         */
        template<class T, class D>
        SQLITEPP_ENABLE_IF((std::is_convertible_v<D, void(*)(T *) noexcept>),
        void) bind(int idx, T * ptr, const char * type, D destroy)
            { check_error(sqlite3_bind_pointer(this->c_ptr(), idx, ptr, type, (void(*)(void*))(void(*)(T *) noexcept)destroy)); }

        /**
         * Bind a custom pointer to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_pointer. 
         * 
         * This is a safer overload of 
         * @ref bind_pointer "bind(int, T * , const char * , void( * )(T * ))"
         * that takes a pointer via std::unique_ptr ownership transfer. The inferred
         * "type" for ::sqlite3_bind_pointer is `typeid(T).name()`.
         */
        template<class T>
        void bind(int idx, std::unique_ptr<T> ptr)
            { this->bind(idx, ptr.release(), typeid(T).name(), [](T * p) noexcept { delete p; }); }

        /**
         * Bind a dynamically typed value to a parameter of the statement
         * 
         * Equivalent to ::sqlite3_bind_value. 
         */
        void bind(int idx, const value & val);

        ///@}

        /** @{
         * @anchor statement_carray_bind
         * @name Binding carray() values to parameters
         * 
         * This set of overloaded functions wraps @ref sqlite3_carray_bind and
         * @ref sqlite3_carray_bind_v2 functions. 
         */

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 52, 0)

    private:
        template<class T>
        static constexpr bool supported_carray_type = 
            std::is_same_v<std::remove_const_t<T>, int> ||
            std::is_same_v<std::remove_const_t<T>, int64_t> ||
            std::is_same_v<std::remove_const_t<T>, double> ||
            std::is_same_v<std::remove_const_t<T>, char *> ||
        #if __cpp_char8_t >= 201811
            std::is_same_v<std::remove_const_t<T>, char8_t *> ||
        #endif
            std::is_same_v<std::remove_const_t<T>, iovec>;

        template<class T>
        static constexpr int carray_type()
        {
            if constexpr (std::is_same_v<std::remove_const_t<T>, int>)
                return SQLITE_CARRAY_INT32;
            else if constexpr (std::is_same_v<std::remove_const_t<T>, int64_t>)
                return SQLITE_CARRAY_INT64;
            else if constexpr (std::is_same_v<std::remove_const_t<T>, double>)
                return SQLITE_CARRAY_DOUBLE;
            else if constexpr (std::is_same_v<std::remove_const_t<T>, char *>)
                return SQLITE_CARRAY_TEXT;
        #if __cpp_char8_t >= 201811
            else if constexpr (std::is_same_v<std::remove_const_t<T>, char8_t *>)
                return SQLITE_CARRAY_TEXT;
        #endif
            else if constexpr (std::is_same_v<std::remove_const_t<T>, iovec>)
                return SQLITE_CARRAY_BLOB;
            else
                static_assert(dependent_false<T>, "unsupported carray type");
        }

    public:

        /**
         * Bind the content of an array for the CARRAY table-valued function in the statement
         * 
         * Equivalent to ::sqlite3_carray_bind_v2 with #SQLITE_TRANSIENT.
         * 
         * The span content is copied into the statement.
         * Thus the lifetime of the data referred to by `array` parameter is 
         * independent of the statement's
         * 
         * @tparam T Array content type. Can be one of:
         * - int
         * - int64_t
         * - double
         * - char *
         * - char8_t * (if `char8_t` is supported by your compiler/library)
         * - iovec
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_carray_type<T>,
        void) carray_bind(int idx, span<T> array)
            { this->carray_bind(idx, array, (void (*)(void *) noexcept)SQLITE_TRANSIENT, nullptr); }

        /**
         * Bind a reference to an array for the CARRAY table-valued function in the statement
         * 
         * Equivalent to ::sqlite3_carray_bind_v2 with #SQLITE_STATIC.
         * 
         * The span content is used **by reference**.
         * Thus the data referred to by `array` parameter must
         * remain valid during this statement's lifetime.
         * 
         * @tparam T Array content type. Can be one of:
         * - int
         * - int64_t
         * - double
         * - char *
         * - char8_t * (if `char8_t` is supported by your compiler/library)
         * - iovec
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_carray_type<T>,
        void) carray_bind_reference(int idx, span<T> array)
            { this->carray_bind(idx, array, (void (*)(void *) noexcept)SQLITE_STATIC, nullptr); }

        /**
         * Bind a reference to an array for the CARRAY table-valued function in the statement
         * 
         * Equivalent to ::sqlite3_carray_bind_v2
         * 
         * The span content is used **by reference**.
         * The `destroy` function will be invoked by SQLite when it is safe to dispose of the
         * array data (on both success and failure).
         * 
         * @tparam T Array content type. Can be one of:
         * - int
         * - int64_t
         * - double
         * - char *
         * - char8_t * (if `char8_t` is supported by your compiler/library)
         * - iovec
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_carray_type<T>,
        void) carray_bind(int idx, span<T> array, void (*destroy)(void *) noexcept, void * destroy_arg = nullptr)
        {
            check_error(sqlite3_carray_bind_v2(this->c_ptr(), idx, (void*)array.data(), int(array.size()), 
                                               carray_type<T>(), (void(*)(void *))destroy, destroy_arg));
        }

    #endif

        ///@} 

        /** @{
         * @anchor statement_managing_binding
         * @name Managing parameter bindings
         */

        /**
         * Reset all bindings on the statement
         * 
         * Equivalent to ::sqlite3_clear_bindings. 
         */
        void clear_bindings() noexcept
            { sqlite3_clear_bindings(c_ptr()); }

        /**
         * Returns the number of SQL parameters
         * 
         * Equivalent to ::sqlite3_bind_parameter_count
         */
        int bind_parameter_count() const noexcept
            { return sqlite3_bind_parameter_count(c_ptr()); }

        /**
         * Returns the index of a parameter with a given name
         * 
         * Equivalent to ::sqlite3_bind_parameter_index
         */
        int bind_parameter_index(const string_param & name) const noexcept
            { return sqlite3_bind_parameter_index(c_ptr(), name.c_str()); }

        /**
         * Returns the name of a parameter with a given index
         * 
         * Equivalent to ::sqlite3_bind_parameter_name
         */
        const char * bind_parameter_name(int idx) const noexcept
            { return sqlite3_bind_parameter_name(c_ptr(), idx); }

        ///@}

        /**
         * Number of columns in a result set
         * 
         * Equivalent to ::sqlite3_column_count
         * 
         * Note that ::sqlite3_column_count represented by this function and 
         * ::sqlite3_data_count represented by data_count() are subtly and confusingly 
         * different. See their respective documentation for details.
         * 
         * @see data_count
         */
        int column_count() const noexcept
            { return sqlite3_column_count(c_ptr()); }

        /**
         * Number of columns in a result set
         * 
         * Equivalent to ::sqlite3_data_count
         * 
         * Note that ::sqlite3_data_count represented by this function and 
         * ::sqlite3_column_count represented by column_count() are subtly and confusingly 
         * different. See their respective documentation for details.
         * 
         * @see column_count
         */
        int data_count() const noexcept
            { return sqlite3_data_count(c_ptr()); }

    private:
        template<class T>
        static constexpr bool supported_column_type = 
            std::is_same_v<T, int> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string_view> ||
        #if __cpp_char8_t >= 201811
            std::is_same_v<T, std::u8string_view> ||
        #endif
            std::is_same_v<T, blob_view>;

    public:

        /** @{
         * @anchor statement_column_info
         * @name Obtaining query results information by column
         */

        /**
         * Get result value from a query 
         * 
         * Wraps @ref sqlite3_column_ function family. Unlike the C API you specify the
         * desired type via T template parameter
         * 
         * @tparam T Desired output type. Must be one of:
         * - int
         * - int64_t
         * - double
         * - std::string_view
         * - std::u8string_view (if `char8_t` is supported by your compiler/library)
         * - blob_view
         * @param idx Column index
         */
        template<class T>
        SQLITEPP_ENABLE_IF(supported_column_type<T>,
        T) column_value(int idx) const noexcept;

        /**
         * Get result values from a query as a raw @ref value object
         * 
         * Equivalent to ::sqlite3_column_value
         */
        const value & raw_column_value(int idx) const noexcept
            { return *(const value *)sqlite3_column_value(c_ptr(), idx); }

        /**
         * Default datatype of the result column
         * 
         * Equivalent to ::sqlite3_column_type
         * 
         * @returns One of the [SQLite fundamental datatypes](https://www.sqlite.org/c3ref/c_blob.html)
         */
        int column_type(int idx) const noexcept
            { return sqlite3_column_type(c_ptr(), idx); }

        /**
         * Name of the result column
         * 
         * Equivalent to ::sqlite3_column_name
         * 
         * The returned string pointer is valid until either the 
         * statement is destroyed or until the statement is automatically 
         * re-prepared by the first call to step() for a particular run or 
         * until the next call to column_name() on the same column.
         */
        const char * column_name(int idx) const noexcept
            { return sqlite3_column_name(c_ptr(), idx); }

        /**
         * Database that is the origin of a result column
         * 
         * Equivalent to ::sqlite3_column_database_name
         */
        const char * column_database_name(int idx) const noexcept
            { return sqlite3_column_database_name(c_ptr(), idx); }

        /**
         * Table that is the origin of a result column
         * 
         * Equivalent to ::sqlite3_column_table_name
         */
        const char * column_table_name(int idx) const noexcept
            { return sqlite3_column_table_name(c_ptr(), idx); }

        /**
         * Table column that is the origin of a result column
         * 
         * Equivalent to ::sqlite3_column_origin_name
         */
        const char * column_origin_name(int idx) const noexcept
            { return sqlite3_column_origin_name(c_ptr(), idx); }

        /**
         * Declared datatype of a result column
         * 
         * Equivalent to ::sqlite3_column_decltype
         */
        const char * column_declared_type(int idx) const noexcept
            { return sqlite3_column_decltype(c_ptr(), idx); }

        ///@}

        /**
         * Returns a pointer to a copy of the SQL text used to create the statement
         * 
         * Equivalent to ::sqlite3_sql
         */
        const char * sql() const noexcept
            { return sqlite3_sql(c_ptr()); }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 14, 0)
        /**
         * Returns SQL text of the statement with bound parameters expanded
         * 
         * Equivalent to ::sqlite3_expanded_sql
         * 
         * @since SQLite 3.14
         */
        allocated_string expanded_sql() const;
#endif

    private:
        void check_error(int res) const;
    };

    /// @cond PRIVATE

    template<>
    inline int statement::column_value<int>(int idx) const noexcept
        { return sqlite3_column_int(c_ptr(), idx); }

    template<>
    inline int64_t statement::column_value<int64_t>(int idx) const noexcept
        { return sqlite3_column_int64(c_ptr(), idx); }

    template<>
    inline double statement::column_value<double>(int idx) const noexcept
        { return sqlite3_column_double(c_ptr(), idx); }

    /// @endcond

    /** @} */

    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /**
     * Parses text containing multiple SQL statements
     * 
     * This helper class allows you to iterate over text containing multiple SQL
     * statements and generate @ref statement instances from them 
     */
    SQLITEPP_EXPORTED
    class statement_parser
    {
    public:
        /// Create a parser for the given database and SQL text
        statement_parser(const database & db, std::string_view sql):
            _db(&db),
            _sql(sql)
        {}

        /**
         * Return the next statement if any
         * 
         * @returns Next statement or nullptr when done
         */
        std::unique_ptr<statement> next();
    private:
        const database * _db;
        std::string_view _sql;
    };

    /**
     * Bitwise mask of resets to perform for thinsqlitepp::auto_reset
     * 
     * This enum supports all the normal bitwise operations: `&`, `|`, `^` and `~`
     */
    SQLITEPP_EXPORTED
    enum class auto_reset_flags: unsigned
    {
        none = 0,               ///< Reset nothing
        reset = 1,              ///< Reset the statement (does not affect the bindings)
        clear_bindings = 2,     ///< Reset the bindings 
        all = 3                 ///< Reset everything
    };
    SQLITEPP_EXPORTED constexpr auto_reset_flags operator|(auto_reset_flags lhs, auto_reset_flags rhs)
        { return auto_reset_flags(unsigned(lhs) | unsigned(rhs)); }
    SQLITEPP_EXPORTED constexpr auto_reset_flags operator&(auto_reset_flags lhs, auto_reset_flags rhs)
        { return auto_reset_flags(unsigned(lhs) & unsigned(rhs)); }
    SQLITEPP_EXPORTED constexpr auto_reset_flags operator^(auto_reset_flags lhs, auto_reset_flags rhs)
        { return auto_reset_flags(unsigned(lhs) ^ unsigned(rhs)); }
    SQLITEPP_EXPORTED constexpr auto_reset_flags operator~(auto_reset_flags arg)
        { return auto_reset_flags(~unsigned(arg)); }

    /**
     * RAII wrapper that resets @ref statement on destruction
     * 
     * This class allows you to restore the state of a @ref statement after using it.
     * This allows you to reuse the statement cleanly without having to worry about resetting
     * it properly on different code paths.
     * 
     * @tparam Flags @ref auto_reset_flags specifying what kind of reset to perform on destruction
     */
    SQLITEPP_EXPORTED
    template<auto_reset_flags Flags>
    class auto_reset
    {
    public:
        /// Constructs an empty instance with no statement
        auto_reset():
            _st(nullptr)
        {}
        /**
         * Constructs an instance referring to a given statement
         * 
         * The statement is being held by reference and must exist as long as
         * this object is existing.
         */
        auto_reset(const std::unique_ptr<statement> & st) noexcept:
            _st(st.get())
        {}

        /// @overload
        auto_reset(statement * st) noexcept:
            _st(st)
        {}

        auto_reset(const auto_reset &) = delete;
        auto_reset & operator=(const auto_reset &) = delete;
        auto_reset(auto_reset && src) noexcept:
            _st(src._st)
        {
            src._st = nullptr;
        }
        auto_reset & operator=(auto_reset && src) noexcept
        {
            destroy();
            _st = src._st;
            src._st = nullptr;
            return *this;
        }

        /// Resets the statement if present
        ~auto_reset() noexcept
            { destroy(); }

        /// Access the stored @ref statement 
        statement * operator->() const noexcept
            { return _st; }

    private:
        void destroy() noexcept
        {
            if (_st)
            {
                if constexpr ((Flags & auto_reset_flags::reset) != auto_reset_flags::none)
                    _st->reset();
                if constexpr ((Flags & auto_reset_flags::clear_bindings) != auto_reset_flags::none)
                    _st->clear_bindings();
            }
        }
    private:
        statement * _st;
    };

    /** @} */

}

namespace thinsqlitepp
{
    inline std::unique_ptr<statement> statement::create(const class database & db, const string_param & sql
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                        , unsigned int flags
#endif
                                                        )
    {
        const char * tail = nullptr;
        sqlite3_stmt * ret = nullptr;
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
        int res = sqlite3_prepare_v3(db.c_ptr(), sql.c_str(), -1, flags, &ret, &tail);
#else
        int res = sqlite3_prepare_v2(db.c_ptr(), sql.c_str(), -1, &ret, &tail);
#endif
        if (res != SQLITE_OK)
            throw exception(res, db);
        return std::unique_ptr<statement>(from(ret));
    }

    inline std::unique_ptr<statement> statement::create(const class database & db, std::string_view & sql
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
                                                        , unsigned int flags
#endif
                                                        )
    {
        const char * start = sql.size() ? &sql[0] : "";
        const char * tail = nullptr;
        sqlite3_stmt * ret = nullptr;
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
        int res = sqlite3_prepare_v3(db.c_ptr(), start, int_size(sql.size()), flags, &ret, &tail);
#else
        int res = sqlite3_prepare_v2(db.c_ptr(), start, int_size(sql.size()), &ret, &tail);
#endif
        if (res != SQLITE_OK)
            throw exception(res, db);
        sql.remove_prefix(size_t(tail - start));
        return std::unique_ptr<statement>(from(ret));
    }

    inline bool statement::step()
    {
        int res = sqlite3_step(c_ptr());
        if (res == SQLITE_DONE)
            return false;
        if (res == SQLITE_ROW)
            return true;
        throw exception(res, database());
    }

    inline void statement::bind(int idx, const std::string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, data, int_size(value.size()), SQLITE_TRANSIENT));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, data, int_size(value.size()), SQLITE_STATIC));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::string_view & value, void (*unref)(const char *) noexcept)
    {
        if (auto data = value.data())
        {
            check_error(sqlite3_bind_text(c_ptr(), idx, data, int_size(value.size()), (void (*)(void *))unref));
        }
        else
        {
            unref(data);
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
        }
    }

#if __cpp_char8_t >= 201811
    inline void statement::bind(int idx, const std::u8string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, (const char *)data, int_size(value.size()), SQLITE_TRANSIENT));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::u8string_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_text(c_ptr(), idx, (const char *)data, int_size(value.size()), SQLITE_STATIC));
        else
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
    }

    inline void statement::bind_reference(int idx, const std::u8string_view & value, void (*unref)(const char8_t *) noexcept)
    {
        if (auto data = value.data())
        {
            check_error(sqlite3_bind_text(c_ptr(), idx, (const char *)data, int_size(value.size()), (void (*)(void *))unref));
        }
        else
        {
            unref(data);
            check_error(sqlite3_bind_text(c_ptr(), idx, "", 0, SQLITE_STATIC));
        }
    }
#endif

    inline void statement::bind(int idx, const blob_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_blob(c_ptr(), idx, data, int_size(value.size()), SQLITE_TRANSIENT));
        else
            check_error(sqlite3_bind_zeroblob(c_ptr(), idx, 0));
    }

    inline void statement::bind_reference(int idx, const blob_view & value)
    {
        if (auto data = value.data())
            check_error(sqlite3_bind_blob(c_ptr(), idx, data, int_size(value.size()), SQLITE_STATIC));
        else
            check_error(sqlite3_bind_zeroblob(c_ptr(), idx, 0));
    }

    inline void statement::bind_reference(int idx, const blob_view & value, void (*unref)(const std::byte *) noexcept)
    {
        if (auto data = value.data())
        {
            check_error(sqlite3_bind_blob(c_ptr(), idx, data, int_size(value.size()), (void (*)(void *))unref));
        }
        else
        {
            unref(data);
            check_error(sqlite3_bind_zeroblob(c_ptr(), idx, 0));
        }
    }

    inline void statement::bind(int idx, const value & val)
    {
        check_error(sqlite3_bind_value(c_ptr(), idx, val.c_ptr()));
    }

    template<>
    inline std::string_view statement::column_value<std::string_view>(int idx) const noexcept
    {
        auto first = (const char *)sqlite3_column_text(c_ptr(), idx);
        auto size = (size_t)sqlite3_column_bytes(c_ptr(), idx);
        return std::string_view(first, size);
    }

#if __cpp_char8_t >= 201811
    template<>
    inline std::u8string_view statement::column_value<std::u8string_view>(int idx) const noexcept
    {
        auto first = (const char8_t *)sqlite3_column_text(c_ptr(), idx);
        auto size = (size_t)sqlite3_column_bytes(c_ptr(), idx);
        return std::u8string_view(first, size);
    }
#endif

    template<>
    inline blob_view statement::column_value<blob_view>(int idx) const noexcept
    {
        auto first = (const std::byte *)sqlite3_column_blob(c_ptr(), idx);
        auto size = sqlite3_column_bytes(c_ptr(), idx);
        return blob_view(first, first + size);
    }

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 14, 0)
    inline allocated_string statement::expanded_sql() const
    {
        auto ret = sqlite3_expanded_sql(c_ptr());
        if (!ret)
            throw exception(SQLITE_NOMEM);
        return allocated_string(ret);
    }
#endif

    inline void statement::check_error(int res) const
    {
        if (res != SQLITE_OK)
            throw exception(res, database());
    }

    inline std::unique_ptr<statement> statement_parser::next()
    {
        while (!_sql.empty())
        {
            auto stmt = statement::create(*_db, _sql);
            if (!stmt) //this happens for a comment or white-space
                continue;

            //trim whitespace after statement
            while (!_sql.empty() && isspace((unsigned char)_sql[0]))
                _sql.remove_prefix(1);

            return stmt;
        }
        return nullptr;
    }

}

namespace thinsqlitepp
{
    /**
     * @addtogroup STLQuery STL interface to queries
     * @{
     */

    /**
     * A cell in a @ref row
     * 
     * A cell represents a column value at a given index for the current row 
     * of a @ref statement.
     * 
     * The @ref statement it accesses is held by reference and must exist as 
     * long as this object is alive
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     */
    SQLITEPP_EXPORTED
    class cell
    {
    public:
        /**
         * Construct a cell for a given statement and column index
         */
        cell(const statement * owner, int idx) noexcept:
            _owner(owner), _idx(idx)
        {}
        /// @overload
        cell(const std::unique_ptr<statement> & owner, int idx) noexcept:
            cell(owner.get(), idx)
        {}

        /**
         * Default datatype of the cell
         * 
         * Equivalent to ::sqlite3_column_type
         * 
         * @returns One of the [SQLite fundamental datatypes](https://www.sqlite.org/c3ref/c_blob.html)
         * @see statement::column_type
         */
        int type() const noexcept
            { return _owner->column_type(_idx); }
        /**
         * Name of the cell's column
         * 
         * Equivalent to ::sqlite3_column_name
         * 
         * The returned string pointer is valid until either the 
         * statement is destroyed or until the statement is automatically 
         * re-prepared by the first call to step() for a particular run or 
         * until the next call to column_name() on the same column.
         * 
         * @see statement::column_name
         */
        const char * name() const noexcept
            { return _owner->column_name(_idx); }
        /**
         * Cell's value
         * 
         * @tparam T Desired output type. Must be one of:
         * - int
         * - int64_t
         * - double
         * - std::string_view
         * - std::u8string_view (if `char8_t` is supported by your compiler/library)
         * - blob_view
         * @see statement::column_value
         */
        template<class T> T value() const noexcept
            { return _owner->column_value<T>(_idx); }

        /**
         * Database that is the origin of the cell
         * 
         * Equivalent to ::sqlite3_column_database_name
         * 
         * @see statement::column_database_name
         */ 
        const char * database_name() const noexcept
            { return _owner->column_database_name(_idx); }
        /**
         * Table that is the origin of the cell
         * 
         * Equivalent to ::sqlite3_column_table_name
         * 
         * @see statement::column_table_name
         */
        const char * table_name() const noexcept
            { return _owner->column_table_name(_idx); }

        /**
         * Table column that is the origin of the cell
         * 
         * Equivalent to ::sqlite3_column_origin_name
         * 
         * @see statement::column_origin_name
         */
        const char * origin_name() const noexcept
            { return _owner->column_origin_name(_idx); }

        /**
         * Declared datatype of the cell
         * 
         * Equivalent to ::sqlite3_column_decltype
         * 
         * @see statement::column_declared_type
         */
        const char * declared_type() const noexcept
            { return _owner->column_declared_type(_idx); }

    protected:
        const statement * _owner;
        int _idx;
    };

    /**
     * Row result of a @ref statement
     * 
     * The row is a random access STL range of @ref cell objects.
     * 
     * Note that the actual row represented by this class is the *current* 
     * @ref statement result and changes every time the statement makes 
     * a statement::step.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     * 
     */
    SQLITEPP_EXPORTED
    class row
    {
    public:
        using value_type = cell;
        using size_type = int;
        using difference_type = int;
        using reference = value_type;
        using pointer = void;

        class const_iterator : private cell
        {
        friend class row;
        public:
            using value_type = row::value_type;
            using size_type = row::size_type;
            using difference_type = row::difference_type;
            using reference = row::reference;
            using pointer = row::pointer;
            using iterator_category = std::random_access_iterator_tag;
        public:
            const_iterator() noexcept:
                cell(nullptr, -1)
            {}

            cell operator*() const noexcept
                { return *this; }
            const cell * operator->() const noexcept
                { return this; }
            cell operator[](size_type idx) const noexcept
                { return *(*this + idx); }

            const_iterator & operator++() noexcept
                { ++_idx; return *this; }
            const_iterator operator++(int) noexcept
                { return const_iterator(_owner, _idx++);  }
            const_iterator & operator+=(int val) noexcept
                { _idx += val; return *this; }
            const_iterator & operator--() noexcept
                { --_idx; return *this; }
            const_iterator operator--(int) noexcept
                { return const_iterator(_owner, _idx--);  }
            const_iterator & operator-=(int val) noexcept
                { _idx -= val; return *this; }

            friend const_iterator operator+(const const_iterator & lhs, int rhs) noexcept
                { return const_iterator(lhs._owner, lhs._idx + rhs); }
            friend const_iterator operator+(int lhs, const const_iterator & rhs) noexcept
                { return const_iterator(rhs._owner, rhs._idx + lhs); }

            friend int operator-(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx - rhs._idx; }
            friend const_iterator operator-(const const_iterator & lhs, int rhs) noexcept
                { return const_iterator(lhs._owner, lhs._idx - rhs); }

            friend bool operator==(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx == rhs._idx; }
            friend bool operator!=(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx != rhs._idx; }
            friend bool operator<(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx < rhs._idx; }
            friend bool operator<=(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx <= rhs._idx; }
            friend bool operator>(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx > rhs._idx; }
            friend bool operator>=(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx >= rhs._idx; }
        private:
            const_iterator(const statement * owner, int idx) noexcept:
                cell(owner, idx)
            {}
        };
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = std::reverse_iterator<iterator>;
    public:
        /**
         * Construct a row for a given statement.
         * 
         * The @ref statement is held by reference and must exist as long as this object
         * is alive
         */
        row(const statement * owner) noexcept:
            _owner(owner)
        {}
        /// @overload
        row(const std::unique_ptr<statement> & owner) noexcept:
            row(owner.get())
        {}

        int size() const noexcept
            { return _owner->data_count(); }
        bool empty() const noexcept
            { return size() == 0; }

        cell operator[](int idx) const noexcept
            { return cell(_owner, idx); }

        const_iterator begin() const noexcept
            { return const_iterator(_owner, 0); }
        const_iterator cbegin() const noexcept
            { return const_iterator(_owner, 0); }
        const_iterator end() const noexcept
            { return const_iterator(_owner, size()); }
        const_iterator cend() const noexcept
            { return const_iterator(_owner, size()); }

        const_reverse_iterator rbegin() const noexcept
            { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept
            { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept
            { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept
            { return const_reverse_iterator(begin()); }
    protected:
        const statement * _owner;
    };

    /**
     * An [input iterator](https://en.cppreference.com/w/cpp/iterator/input_iterator) 
     * for @ref statement results.
     * 
     * This class stores the @ref statement *by reference*. Thus @ref statement must remain
     * valid for the lifetime duration of this class.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     */
    SQLITEPP_EXPORTED
    class row_iterator : private row
    {
    public:
        using value_type = row;
        using size_type = int;
        using difference_type = int;
        using reference = row;
        using pointer = void;
        using iterator_category = std::input_iterator_tag;

    public:
        /**
         * Create an empty iterator
         * 
         * Such iterator is usable as an end of range sentinel
         */
        row_iterator() noexcept:
            row(nullptr)
        {}
        /**
         * Create an instance referring to a given statement
         * 
         * Note that the iterator mutates the statement while iterating,
         * hence the argument must be non-const.
         */
        row_iterator(statement * owner):
            row(owner)
        {
            if (_owner)
                increment();
        }

        /// @overload
        row_iterator(std::unique_ptr<statement> & owner):
            row_iterator(owner.get())
        {}

        row operator*() const noexcept
            { return *this; }

        const row * operator->() const noexcept
            { return this; }

        row_iterator & operator++()
            { increment(); return *this; }
        void operator++(int)
            { increment(); }

        friend bool operator==(const row_iterator & lhs, const row_iterator & rhs) noexcept
            { return lhs._owner == rhs._owner; }
        friend bool operator!=(const row_iterator & lhs, const row_iterator & rhs) noexcept
            { return lhs._owner != rhs._owner; }
    private:
        void increment()
        {
            if (!const_cast<statement *>(_owner)->step())
                _owner = nullptr;
        }
    };

    /**
     * An [input range](https://en.cppreference.com/w/cpp/ranges/input_range) 
     * for @ref statement results.
     * 
     * This class stores the @ref statement *by reference*. Thus @ref statement must remain
     * valid for the lifetime duration of this class.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     */
    SQLITEPP_EXPORTED
    class row_range {
    public:
        using value_type = row;
        using size_type = int;
        using difference_type = int;
        using reference = value_type;
        using pointer = void;

        using const_iterator = row_iterator;
        using iterator = const_iterator;
    public:
        /**
         * Create an instance referring to a given statement
         * 
         * Note that the iterator mutates the statement while iterating,
         * hence the argument must be non-const.
         */
        row_range(statement * owner):
            _it(owner)
        {}

        /// @overload
        row_range(std::unique_ptr<statement> & owner):
            row_range(owner.get())
        {}

        const_iterator begin() const noexcept
            { return _it; }
        const_iterator cbegin() const noexcept
            { return _it; }
        const_iterator end() const noexcept
            { return const_iterator(); }
        const_iterator cend() const noexcept
            { return const_iterator(); }

    private:
        row_iterator _it;
    };

    /** @} */
}

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
                        return (*data)(span<const std::byte>((std::byte *)lhs_bytes, size_t(lhs_len)), span<const std::byte>((std::byte *)rhs_bytes, size_t(rhs_len)));
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

#if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 51, 1)
    inline struct database::status64 database::status64(int op, bool reset) const
    {
        struct status64 ret;
        check_error(sqlite3_db_status64(c_ptr(), op, &ret.current, &ret.high, reset));
        return ret;
    }
#endif

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

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0) && (THINSQLITEPP_ENABLE_EXPERIMENTAL || THINSQLITEPP_ENABLE_EXPIREMENTAL)

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

namespace thinsqlitepp
{
    inline error::error(int error_code) noexcept:
        _error_code(error_code),
        _message(sqlite3_errstr(error_code))
    {}

    inline error::error(int error_code, const database * db) noexcept:
        error(error_code)
    {
        auto db_error_code = sqlite3_extended_errcode(c_ptr(db));
        if ((error_code == SQLITE_MISUSE && (db_error_code & 0x0FF) != SQLITE_MISUSE) ||
            db_error_code == SQLITE_OK)
            return;

        _error_code = db_error_code;
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 12, 0)
        _system_error_code = sqlite3_system_errno(c_ptr(db));
#endif
    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 38, 0)
        _offset = sqlite3_error_offset(c_ptr(db));
    #endif
        auto db_message = sqlite3_errmsg(c_ptr(db));
        if (db_message != _message.get())
            _message = copy_message(db_message);
    }

    inline error::message_ptr error::copy_message(const char * src) noexcept
    {
        if (!src)
            return nullptr;
        const auto len = strlen(src);
        char * const ret = (char *)sqlite_allocate_nothrow(len + 1);
        if (!ret)
            return nullptr;
        memcpy(ret, src, len + 1);
        return message_ptr(ret, sqlite3_free);
    }

    inline const char * exception::what() const noexcept
    {
        auto message = _error.message();
        return message ? message : "<no message available>";
    }
}

/**
 * ThinSQLite++ namespace
 */
namespace thinsqlitepp
{
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Initialize the SQLite library
     * 
     * Equivalent to ::sqlite3_initialize
     * 
     * `#include <thinsqlitepp/global.hpp>`
     */
    SQLITEPP_EXPORTED
    inline void initialize()
    {
        int res = sqlite3_initialize();
        if (res != SQLITE_OK)
            throw exception(res);
    }

    /**
     * Deinitialize the SQLite library
     * 
     * Equivalent to ::sqlite3_shutdown
     * 
     * `#include <thinsqlitepp/global.hpp>`
     */
    SQLITEPP_EXPORTED
    inline void shutdown() noexcept
    {
        sqlite3_shutdown();
    }

    /// Return type for @ref status()
    SQLITEPP_EXPORTED
    struct status_value
    {
    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0)
        using counter_type = sqlite3_int64;
    #else
        using counter_type = int;
    #endif

        counter_type current;
        counter_type highwater;
    };

    /**
     * Obtain SQLite runtime status
     * 
     * Equivalent to `::sqlite3_status64` or `::sqlite3_status`, if the former is not available
     */
    SQLITEPP_EXPORTED
    inline status_value status(int op, bool reset = false)
    {
        status_value ret;
    #if  SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0)
        int res = sqlite3_status64(op, &ret.current, &ret.highwater, reset);
    #else
        int res = sqlite3_status(op, &ret.current, &ret.highwater, reset);
    #endif
        if (res != SQLITE_OK)
            throw exception(res);
        return ret;
    }

    /** @cond PRIVATE */

    namespace internal
    {
        template<int Code, class ...Args>
        struct config_option
        {
            static void apply(Args && ...args)
            {
                int res = sqlite3_config(Code, std::forward<Args>(args)...);
                if (res != SQLITE_OK)
                    throw exception(res);
            }
        };

        template<int Code> struct config_mapping;

    }

    /** @endcond */

    /**
     * Configures SQLite library.
     * 
     * Wraps ::sqlite3_config
     * 
     * @tparam Code One of the SQLITE_CONFIG_ options. Needs to be explicitly specified
     * @tparam Args depend on the `Code` template parameter
     * 
     * `#include <thinsqlitepp/global.hpp>`
     * 
     * The following table lists required argument types for each option.
     * Supplying wrong argument types will result in compile-time error.
     * 
     * @include{doc} global-options.md
     */
    SQLITEPP_EXPORTED
    template<int Code, class ...Args>
    inline
    auto config(Args && ...args) -> 
        //void but prevents instantiation with wrong types 
        decltype(
          internal::config_mapping<Code>::type::apply(std::forward<decltype(args)>(args)...)
        )
        { internal::config_mapping<Code>::type::apply(std::forward<Args>(args)...); }

    /** @} */

    /** @cond PRIVATE */

    namespace internal 
    {
        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_BEGIN

        #if SQLITEPP_USE_VARARG_POUND_POUND_TRICK

            //Idiotic GCC in pedantic mode warns on MACRO(arg) for MARCO(x,...) in < C++20 mode
            //with no way to disable the warning(!!!). 
            #define SQLITEPP_DEFINE_OPTION_0(code) \
                template<> struct config_mapping<code> { using type = config_option<code>; };
            #define SQLITEPP_DEFINE_OPTION_N(code, ...) \
                template<> struct config_mapping<code> { using type = config_option<code, ##__VA_ARGS__>; };

        #else

            #define SQLITEPP_DEFINE_OPTION(code, ...) \
                template<> struct config_mapping<code> { using type = config_option<code __VA_OPT__(,) __VA_ARGS__>; };

            #define SQLITEPP_DEFINE_OPTION_N(code, ...) SQLITEPP_DEFINE_OPTION(code __VA_OPT__(,) __VA_ARGS__)
            #define SQLITEPP_DEFINE_OPTION_0(code) SQLITEPP_DEFINE_OPTION_N(code)

        #endif

        //@ [Config Options]

        SQLITEPP_DEFINE_OPTION_0( SQLITE_CONFIG_SINGLETHREAD          );
        SQLITEPP_DEFINE_OPTION_0( SQLITE_CONFIG_MULTITHREAD           );
        SQLITEPP_DEFINE_OPTION_0( SQLITE_CONFIG_SERIALIZED            );
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MALLOC,               sqlite3_mem_methods *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_GETMALLOC,            sqlite3_mem_methods *);
        #ifdef SQLITE_CONFIG_SMALL_MALLOC
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_SMALL_MALLOC,         int);
        #endif
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MEMSTATUS,            int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PAGECACHE,            void *, int, int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_HEAP,                 void *, int, int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MUTEX,                sqlite3_mutex_methods *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_GETMUTEX,             sqlite3_mutex_methods *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_LOOKASIDE,            int, int);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PCACHE2,              sqlite3_pcache_methods2 *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_GETPCACHE2,           sqlite3_pcache_methods2 *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_LOG,                  void (*)(void*, int, const char *), void *);
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_URI,                  int);
        #ifdef SQLITE_CONFIG_COVERING_INDEX_SCAN
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_COVERING_INDEX_SCAN,  int);
        #endif
        #ifdef SQLITE_CONFIG_SQLLOG
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_SQLLOG,               void (*)(void *, sqlite3 *, const char *, int), void *);
        #endif
        #ifdef SQLITE_CONFIG_MMAP_SIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MMAP_SIZE,            int64_t, int64_t);
        #endif
        #ifdef SQLITE_CONFIG_WIN32_HEAPSIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_WIN32_HEAPSIZE,       int);
        #endif
        #ifdef SQLITE_CONFIG_PCACHE_HDRSZ
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PCACHE_HDRSZ,         int *);
        #endif
        #ifdef SQLITE_CONFIG_PMASZ
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_PMASZ,                unsigned int);
        #endif
        #ifdef SQLITE_CONFIG_STMTJRNL_SPILL
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_STMTJRNL_SPILL,       int);
        #endif
        #ifdef SQLITE_CONFIG_SORTERREF_SIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_SORTERREF_SIZE,       int);
        #endif
        #ifdef SQLITE_CONFIG_MEMDB_MAXSIZE
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_MEMDB_MAXSIZE,        int64_t);
        #endif
        #ifdef SQLITE_CONFIG_ROWID_IN_VIEW
        SQLITEPP_DEFINE_OPTION_N( SQLITE_CONFIG_ROWID_IN_VIEW,        int);
        #endif

        //@ [Config Options]

        #undef SQLITEPP_DEFINE_OPTION_0
        #undef SQLITEPP_DEFINE_OPTION_N
        #ifdef SQLITEPP_DEFINE_OPTION
            #undef SQLITEPP_DEFINE_OPTION
        #endif

        SQLITEPP_SUPPRESS_SILLY_VARARG_WARNING_END

    }
    /** @endcond */
}

namespace thinsqlitepp
{
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 24, 0)
    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * STL interface to SQLite keywords
     * 
     * Wraps manual calls to ::sqlite3_keyword_count, ::sqlite3_keyword_name and
     * ::sqlite3_keyword_check in a convenient STL range interface.
     * 
     * `#include <thinsqlitepp/global.hpp>`
     */
    SQLITEPP_EXPORTED
    class keywords 
    {
    public:
        using value_type = std::string_view;
        using size_type = int;
        using difference_type = int;
        using reference = value_type;
        using pointer = void;

        class const_iterator 
        {
            friend class keywords;
        public:
            using value_type = keywords::value_type;
            using size_type = keywords::size_type;
            using difference_type = keywords::difference_type;
            using reference = keywords::reference;
            using pointer = void;
            using iterator_category = std::random_access_iterator_tag;

        public:
            const_iterator() noexcept = default;

            std::string_view operator*() const
            { 
                const char * name = nullptr;
                int len = 0;
                int res = sqlite3_keyword_name(_idx, &name, &len);
                if (res != SQLITE_OK)
                    throw exception(res);
                return std::string_view(name, size_t(len)); 
            }
            std::string_view operator[](size_type idx) const noexcept
                { return *(*this + idx); }

            const_iterator & operator++()
                { ++_idx; return *this; }
            const_iterator operator++(int)
                { return const_iterator(_idx++); }
            const_iterator & operator+=(int val) noexcept
                { _idx += val; return *this; }
            const_iterator & operator--()
                { --_idx; return *this; }
            const_iterator operator--(int)
                { return const_iterator(_idx--); }
            const_iterator & operator-=(int val) noexcept
                { _idx -= val; return *this; }

            friend const_iterator operator+(const const_iterator & lhs, int rhs) noexcept
                { return const_iterator(lhs._idx + rhs); }
            friend const_iterator operator+(int lhs, const const_iterator & rhs) noexcept
                { return const_iterator(rhs._idx + lhs); }

            friend int operator-(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx - rhs._idx; }
            friend const_iterator operator-(const const_iterator & lhs, int rhs) noexcept
                { return const_iterator(lhs._idx - rhs); }

            friend bool operator==(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx == rhs._idx; }
            friend bool operator!=(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx != rhs._idx; }
            friend bool operator<(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx < rhs._idx; }
            friend bool operator<=(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx <= rhs._idx; }
            friend bool operator>(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx > rhs._idx; }
            friend bool operator>=(const const_iterator & lhs, const const_iterator & rhs) noexcept
                { return lhs._idx >= rhs._idx; }

        private:
            const_iterator(int idx) noexcept : _idx(idx)
            {}
        private:
            int _idx = 0;
        };

        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = std::reverse_iterator<iterator>;

    public:
        int size() const noexcept
            { return sqlite3_keyword_count(); }
        bool empty() const noexcept
            { return size() == 0; }

        std::string_view operator[](int idx) const noexcept
            { return *const_iterator(idx); }

        const_iterator begin() const noexcept
            { return const_iterator(0); }
        const_iterator cbegin() const noexcept
            { return const_iterator(0); }
        const_iterator end() const noexcept
            { return const_iterator(size()); }
        const_iterator cend() const noexcept
            { return const_iterator(size()); }

        const_reverse_iterator rbegin() const noexcept
            { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept
            { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept
            { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept
            { return const_reverse_iterator(begin()); }

        /**
         * Checks to see whether or not the text argument is a keyword
         * 
         * Equivalent to ::sqlite3_keyword_check
         */
        static bool check(std::string_view text)
            { return sqlite3_keyword_check(text.data(), int_size(text.size())); }
    };

    /** @} */
#endif
}

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
    SQLITEPP_EXPORTED
    class sqlite_version
    {
    public:
        /// Construct an instance from an `int` encoded in SQLite format
        explicit constexpr sqlite_version(int val): _value(val) {}

        /// Construct an instance from major, minor, release parts
        static constexpr sqlite_version from_parts(unsigned major, unsigned minor, unsigned release)
        { 
            if (release >= 1000)
                throw std::runtime_error("invalid release value");
            auto val = int(release);
            if (minor >= 1000)
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
            auto val = unsigned(_value);
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

        friend constexpr std::strong_ordering operator<=>(const sqlite_version & lhs, const sqlite_version & rhs) noexcept
            { return lhs._value <=> rhs._value; }
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

namespace thinsqlitepp {

    /**
     * @addtogroup SQL SQLite API Wrappers
     * @{
     */

    /**
     * Virtual Table Indexing Information
     * 
     * This is a [fake wrapper class](https://github.com/gershnik/thinsqlitepp#fake-classes) for 
     * sqlite3_index_info.
     * 
     * It is used by your functions overriding @ref vtab::best_index but can also be used 
     * standalone if manually implementing @ref xBestIndex.
     * 
     * Unlike other SQLite data types sqlite3_index_info is a real struct, not an opaque data
     * type. If desired you can directly access the struct members via c_ptr() but this wrapper provides
     * convenient and safe inline accessor methods for all members. 
     * 
     * @tparam T The type of the index data in sqlite3_index_info::idxStr. Must be a pointer or `void`.
     * If `void` storing data is disabled.
     * 
     * `#include <thinsqlitepp/vtab.hpp>`
     */
    SQLITEPP_EXPORTED
    template<class T = void>
    class index_info : public handle<sqlite3_index_info, index_info<T>>
    {
        static_assert(std::is_void_v<T> || (std::is_pointer_v<T> && std::is_trivially_destructible_v<T>),
                     "template argument must be void or a pointer to a trivially destructible type");
    public:
        /// Alias for unwieldy C struct name 
        using constraint = sqlite3_index_info::sqlite3_index_constraint;
        /// Alias for unwieldy C struct name 
        using constraint_usage = sqlite3_index_info::sqlite3_index_constraint_usage;
        /// Alias for unwieldy C struct name 
        using orderby = sqlite3_index_info::sqlite3_index_orderby;
    public:
        ~index_info() = delete;

        /// Returns the table of WHERE clause constraints 
        span<const constraint> constraints() const noexcept
            { return { this->c_ptr()->aConstraint, size_t(this->c_ptr()->nConstraint)}; };
        /// Returns the table of ORDER BY clause constraints 
        span<const orderby> orderbys() const noexcept
            { return { this->c_ptr()->aOrderBy, size_t(this->c_ptr()->nOrderBy)}; };

        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0)

            /**
             * Returns mask of columns used by statement
             * @since SQLite 3.10.0
             */
            uint64_t columns_used() const noexcept
                { return uint64_t(this->c_ptr()->colUsed); }

        #endif

        /**
         * Determine the collation for a constraint
         * 
         * Equivalent to ::sqlite3_vtab_collation
         */
        const char * collation(int constraint_idx) const noexcept
            { return sqlite3_vtab_collation(this->c_ptr(), constraint_idx); }

        /**
         * Determine if the query is DISTINCT
         * 
         * Equivalent to ::sqlite3_vtab_distinct
         */
        int distinct() const noexcept 
            { return sqlite3_vtab_distinct(this->c_ptr()); }

        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 38, 0)

            /**
             * Determine if a constraint is an IN that can be processed all at once
             * 
             * Equivalent to ::sqlite3_vtab_in with last argument -1 
             */
            bool is_in(int constraint_idx) const noexcept 
                { return sqlite3_vtab_in(this->c_ptr(), constraint_idx, -1); }

            /**
             * Set all-at-once processing of an IN operator
             * 
             * Equivalent to ::sqlite3_vtab_in
             */
            void handle_in(int constraint_idx, bool handle) const noexcept 
                { sqlite3_vtab_in(this->c_ptr(), constraint_idx, handle); }

         #endif

        /**
         * Returns the desired usage of the constraints
         * 
         * The size of this span is the same as returned by @ref constraints.
         */
        span<const constraint_usage> constraints_usage() const noexcept
            { return { this->c_ptr()->aConstraintUsage, size_t(this->c_ptr()->nConstraint)}; }
        /**
         * Returns the desired usage of the constraints
         * 
         * The size of this span is the same as returned by @ref constraints.
         */
        span<constraint_usage> constraints_usage() noexcept
            { return { this->c_ptr()->aConstraintUsage, size_t(this->c_ptr()->nConstraint)}; }

        /// Returns number used to identify the index
        int index_number() const noexcept
            { return this->c_ptr()->idxNum; }
        /// Sets number used to identify the index
        void set_index_number(int val) noexcept
            { this->c_ptr()->idxNum = val; }

        /**
         * Returns data associated with the index
         * 
         * Only meaningful if template parameter T is non-void.
         * Otherwise does nothing and returns nothing.
         */
        T index_data() const noexcept
            { return (T)this->c_ptr()->idxStr; }

        /**
         * Set the index data.
         * 
         * Enabled only if template parameter T is non-void and
         * @p data pointer can be converted to it.
         * 
         * @param data data to set
         * @param allocated if true SQLite will automatically free the data
         * using ::sqlite3_free. Otherwise you are responsible for the pointed
         * data lifecycle 
         */
        template<class X>
        SQLITEPP_ENABLE_IF((std::is_convertible_v<X *, T>),
        void) set_index_data(X * data, bool allocated = false) noexcept
        {
            this->c_ptr()->idxStr = (char *)data;
            this->c_ptr()->needToFreeIdxStr = allocated;
        }

        /**
         * Set the index data
         * 
         * This is a convenience overload of set_index_data(T, bool) that
         * takes a std::unique_pointer with an sqlite_deleter.
         * 
         * Enabled only if template parameter T is non-void, trivially destructible and
         * @p data pointer type can be converted to it.
         */
        template<class X>
        SQLITEPP_ENABLE_IF((std::is_convertible_v<X *, T> && std::is_trivially_destructible_v<X>),
        void) set_index_data(std::unique_ptr<X, sqlite_deleter<X>> data) noexcept
            { set_index_data(data.release(), true); }

        /**
         * Set the index data
         * 
         * This is a convenience overload of set_index_data(T *, bool) that
         * takes a std::unique_pointer to T. 
         * 
         * Enabled only if template parameter T is a pointer to a trivially destructible class
         * derived from sqlite_allocated
         */
        template<class X>
        SQLITEPP_ENABLE_IF((
            std::is_convertible_v<X *, T> &&
            std::is_trivially_destructible_v<X> &&
            std::is_base_of_v<sqlite_allocated, X>),
        void) set_index_data(std::unique_ptr<X> data) noexcept
            { set_index_data(data.release(), true); }

        /// Returns whether the cursor output is already ordered
        bool order_by_consumed() const noexcept
            { return this->c_ptr()->orderByConsumed != 0; }
        /// Sets whether the cursor output is already ordered
        void set_order_by_consumed(bool val) noexcept
            { this->c_ptr()->orderByConsumed = val; }

        /// Returns estimated cost of using this index
        double estimated_cost() const noexcept
            { return this->c_ptr()->estimatedCost; }
        /// Sets estimated cost of using this index
        void set_estimated_cost(double val) noexcept
            { this->c_ptr()->estimatedCost = val; }

        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 8, 2)

            /**
             * Returns estimated number of rows returned
             * @since SQLite 3.8.2
             */
            int64_t estimated_rows() const noexcept
                { return int64_t(this->c_ptr()->estimatedRows); }
            /**
             * Sets estimated number of rows returned
             * @since SQLite 3.8.2
             */
            void set_estimated_rows(int64_t val) noexcept
                { this->c_ptr()->estimatedRows = sqlite3_int64(val); }

        #endif

        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 9, 0)

            /**
             * Returns mask of SQLITE_INDEX_SCAN_ flags
             * @since SQLite 3.9.0
             */
            int index_flags() const noexcept
                { return this->c_ptr()->idxFlags; }

            /**
             * Sets mask of SQLITE_INDEX_SCAN_ flags
             * @since SQLite 3.9.0
             */
            void set_index_flags(int val) noexcept
                { this->c_ptr()->idxFlags = val; }

        #endif

    };

    /** @} */

    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /**
     * Base class for virtual table object implementations
     * 
     * This class greatly simplifies development of [virtual tables](https://www.sqlite.org/vtab.html)
     * by encapsulating management of sqlite3_module, providing type safety, error handling and
     * RAII and reasonable defaults.
     * 
     * It is intended to be used as [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) 
     * base class for your own virtual table implementations. 
     * 
     * @important
     * This documentation only describes the base class interface. 
     * Please refer to @ref vtab-guide for detailed information about how to create classes derived from @ref vtab
     * to implement virtual tables.  
     * 
     * `#include <thinsqlitepp/vtab.hpp>`
     */
    SQLITEPP_EXPORTED
    template<class Derived>
    class vtab : private sqlite3_vtab
    {
    public:
        /**
         * Type of data passed via create_module to the constructor(s)
         * 
         * You can override this default by declaring a different typedef in your
         * derived class
         * 
         * The default is `void`, meaning no data is stored and passed
         */
        using constructor_data_type = void;

        /**
         * Type of data stored in index_info and passed between
         * @ref best_index and @ref cursor::filter.
         * 
         * You can override this default by declaring a different typedef in your
         * derived class.
         * 
         * The default is `void`, meaning no data is stored and passed
         */
        using index_data_type = void;

        /**
         * Base class for cursors
         * 
         * It wraps sqlite3_vtab_cursor and provides default implementation
         * of the required methods. The default implementation returns no rows.
         * Re-define various methods in your derived classes.
         */
        class cursor : private sqlite3_vtab_cursor
        {
        friend vtab;
        public:
            cursor(cursor &) = delete;
            cursor & operator=(cursor &) = delete;

            /// Access the underlying sqlite3_vtab_cursor struct
            sqlite3_vtab_cursor * c_ptr() const noexcept
                { return const_cast<cursor *>(this); }

            /**
             * Begins a search of a virtual table
             * 
             * Equivalent to @ref xFilter
             * 
             * This method is called if @ref index_data_type is defined as a pointer.
             * 
             * Re-define this method as a non-templated function in your derived class. 
             * Your implementation can throw exceptions to indicate errors.
             * 
             * This method can be called multiple times and should initialize cursor internals
             * to start cursor iteration anew (do not rely on constructor to do that).
             * 
             * The default implementation does nothing.
             * 
             * @tparam D Defers resolution of nested data types declared in derived class. This is an 
             * internal implementation detail - your re-defined implementations do not need to be templated.
             * @param idx value passed to index_info::set_index_number in @ref best_index. Its significance is
             * entirely up to you
             * @param idx_data pointer passed to index_info::set_index_data. Its significance is
             * entirely up to you
             * @param argc count of items in @p argv array
             * @param argv requested values of certain expressions from @ref index_info::constraint_usage
             */
            template<class D=Derived> //defer resolution of nested data types
            SQLITEPP_ENABLE_IF((std::is_pointer_v<typename D::index_data_type>),
            void) filter([[maybe_unused]] int idx, 
                         [[maybe_unused]] typename D::index_data_type idx_data, 
                         [[maybe_unused]] int argc, 
                         [[maybe_unused]] value ** argv)
            {
                static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            }

            /**
             * Begins a search of a virtual table
             * 
             * Equivalent to @ref xFilter
             * 
             * This method is called if @ref index_data_type is void.
             * 
             * Re-define this method as a non-templated function in your derived class. 
             * Your implementation can throw exceptions to indicate errors.
             * 
             * This method can be called multiple times and should initialize cursor internals
             * to start cursor iteration anew (do not rely on constructor to do that).
             * 
             * The default implementation does nothing.
             * 
             * @tparam D Defers resolution of nested data types declared in derived class. This is an 
             * internal implementation detail - your re-defined implementations do not need to be templated.
             * @param idx value passed to index_info::set_index_number in @ref best_index. Its significance is
             * entirely up to you
             * @param argc count of items in @p argv array
             * @param argv requested values of certain expressions from @ref index_info::constraint_usage
             */
            template<class D=Derived> //defer resolution of nested data types
            SQLITEPP_ENABLE_IF((std::is_void_v<typename D::index_data_type>),
            void) filter([[maybe_unused]] int idx, 
                         [[maybe_unused]] int argc, 
                         [[maybe_unused]] value ** argv)
            {
                static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            }

            /**
             * Whether the cursor reached the end
             * 
             * Equivalent to @ref xEof
             * 
             * Re-define this method together with @ref next() in your derived class.
             * Your implementation must also be `noexcept`.
             * 
             * This default implementation always returns `true`
             */
            bool eof() const noexcept
                { return true; }

            /**
             * Advances the cursor
             * 
             * Equivalent to @ref xNext
             * 
             * Re-define this method together with @ref eof() in your derived class.
             * Your implementation can throw exceptions to indicate errors.
             * 
             * This default implementation should never be called since default @ref eof() 
             * always returns `true`. If called it will report an error.
             */
            void next()
                { throw exception(SQLITE_INTERNAL, error::message_ptr("cursor::next is not implemented")); }

            /**
             * Retrieves the value of the virtual table column in a row cursor is currently pointing at
             * 
             * Equivalent to @ref xColumn
             * 
             * Re-define this method in your derived class.
             * Your implementation can throw exceptions to indicate errors.
             * 
             * This default implementation always return null
             * 
             * @param ctxt the context to set the column value on
             * @param idx column index
             */
            void column(context & ctxt, [[maybe_unused]] int idx) const
                { ctxt.result(nullptr); }

            /**
             * Retrieves the rowid of the row cursor is currently pointing at
             * 
             * Equivalent to @ref xRowid
             * 
             * Re-define this method in your derived class.
             * Your implementation can throw exceptions to indicate errors.
             * 
             * This default implementation always reports an error
             */
            sqlite_int64 rowid() const
                { throw exception(SQLITE_INTERNAL, error::message_ptr("cursor::rowid is not implemented")); }
        protected:
            /**
             * Constructs an instance with a given owner
             */
            cursor(Derived * owner):
                sqlite3_vtab_cursor{owner}
            {}
            ~cursor()
            {}

            /**
             * Returns the owning @ref vtab - derived class
             * 
             * Equivalent to accessing the `pVtab` field of sqlite3_vtab_cursor
             * 
             * @note this is safe to call from your derived class constructor
             */
            Derived * owner() const noexcept
                { return static_cast<Derived *>(this->pVtab); }
        };
    public:
        /// Marker type that marks the constructor of @p Derived to be used to create a new table
        struct create_t {};

        /// Marker type that tells the constructor of @p Derived to be used to connect to an existing table
        struct connect_t {};
    public:
        /**
         * Register a virtual table implementation with a database connection
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * If @ref constructor_data_type is not void using this method causes `nullptr` to be 
         * passed to derived class constructor.
         * 
         * @param db database to register the implementation with
         * @param name module name
         */
        static void create_module(database & db, const string_param & name)
        {
            db.create_module(name, vtab::get_module());
        }

        /**
         * Register a virtual table implementation with a database connection
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * This overload is available if @ref constructor_data_type is not `void`.
         * 
         * @tparam D Defers resolution of nested data types declared in derived class. This is an 
         * internal implementation detail - never specify it explicitly.
         * 
         * @param db database to register the implementation with
         * @param name module name
         * @param data data to be passed to your derived class constructor. Can be nullptr. 
         * You can change the type of the data by re-defining @ref constructor_data_type in 
         * your derived class.
         * @param destructor an optional destructor function for the data pointer. Can be nullptr.
         */
        template<class D=Derived>
        SQLITEPP_ENABLE_IFP(
        static,
            (std::is_pointer_v<typename D::constructor_data_type>),
        void) create_module(database & db,
                            const string_param & name, 
                            typename D::constructor_data_type data, 
                            void(*destructor)(typename D::constructor_data_type) noexcept = nullptr)
        {
            static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            db.create_module(name, vtab::get_module(), data, destructor);
        }

        /**
         * Register a virtual table implementation with a database connection
         * 
         * Equivalent to ::sqlite3_create_module_v2
         * 
         * This overload is available if @ref constructor_data_type is not `void`.
         * 
         * @tparam D Defers resolution of nested data types declared in derived class. This is an 
         * internal implementation detail - never specify it explicitly.
         * 
         * @param db database to register the implementation with
         * @param name module name
         * @param data data to be passed to your derived class constructor. Can be nullptr. 
         * You can change the type of the data by re-defining @ref constructor_data_type in 
         * your derived class.
         */
        template<class D=Derived> 
        SQLITEPP_ENABLE_IFP(
        static,
            (std::is_pointer_v<typename D::constructor_data_type>),
        void) create_module(database & db,
                            const string_param & name, 
                            std::unique_ptr<std::remove_pointer_t<typename D::constructor_data_type>> data)
        {
            static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            db.create_module(name, vtab::get_module(), data.release(), [](typename D::constructor_data_type ptr) noexcept {
                delete ptr;
            });
        }

        /**
         * Determines the best way to access the virtual table
         * 
         * Equivalent to @ref xBestIndex
         * 
         * Re-define this method in your derived class.
         * Your implementation can throw exceptions to indicate errors.
         * 
         * This method communicates information to SQLite as well cursor::filter.
         * This default implementation does essentially nothing and allows all filters.
         * 
         * @tparam D Defers resolution of nested data types declared in derived class. This is an 
         * internal implementation detail - your re-defined implementations do not need to be templated.
         * 
         * @param info The SQLite core communicates with the best_index method by populating
         * fields of the index_info passing it to best_index. The best_index method 
         * fills out writable fields of this object which forms the reply.
         * @returns true on success or false if the particular combination of input parameters specified is 
         * insufficient for the virtual table to do its job. This is logically the same as calling the 
         * index_info::set_estimated_cost with an infinity. If every call to @ref best_index for a particular 
         * query plan returns false, that means there is no way for the virtual table to be safely used, 
         * and the statement::create() call will fail with a "no query solution" error.
         * Returning false is equivalent to returning #SQLITE_CONSTRAINT from @ref xBestIndex.
         */
        template<class D=Derived> 
        bool best_index(index_info<typename D::index_data_type> & info) const
        {
            static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            info.set_estimated_cost(0);
            return true;
        }

        /**
         * Creates a new cursor used for accessing the virtual table
         * 
         * Equivalent to @ref xOpen
         * 
         * If your cursor `Derived::cursor` class constructor has the form `cursor::cursor(Derived *)` you
         * do not need to re-define this function. Otherwise re-define it to create cursor in the appropriate
         * way. Your implementation can throw exceptions to indicate errors.
         * 
         * @tparam D Defers resolution of nested data types declared in derived class. This is an 
         * internal implementation detail - your re-defined implementations do not need to be templated.
         * @returns A unique pointer to the cursor instance
         */
        template<class D=Derived>
        std::unique_ptr<typename D::cursor> open()
        {
            static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            return std::unique_ptr<typename D::cursor>(
                        new typename D::cursor(static_cast<D *>(this))
                   );
        }

        /**
         * Obtains the singleton ::sqlite3_module for this virtual table
         * 
         * Usually you do not need to call this function. It can be used to obtain 
         * sqlite3_module instance for your derived class if you plan to use it manually
         * via database::create_module or ::sqlite3_create_module_v2
         */ 
        static sqlite3_module * get_module();

    protected:
        /// This class is default constructible only by derived classes
        vtab():
            sqlite3_vtab{nullptr, 0, nullptr}
        {}
        /// You cannot copy (or move) this class
        vtab(const vtab &) = delete;
        /// You cannot assign this class
        vtab & operator=(const vtab &) = delete;
        /// This class is destructible only by derived classes
        ~vtab()
        {}

    private:
        void set_error_message(error & err) const
        {
            auto me = const_cast<vtab *>(this);
            if (me->zErrMsg)
                sqlite3_free(me->zErrMsg);
            auto message = err.extract_message();
            me->zErrMsg = (char *)message.release();
        }

        void set_error_message(exception & ex) const
            { set_error_message(ex.error()); }

        void set_error_message(std::exception & ex) const
        {
            auto me = const_cast<vtab *>(this);
            if (me->zErrMsg)
            {
                sqlite3_free(me->zErrMsg);
                me->zErrMsg = nullptr;
            }
            auto message = ex.what();
            const auto len = strlen(message) + 1;
            if (char * const ret = (char *)sqlite_allocate_nothrow(len))
            {
                memcpy(ret, message, len);
                me->zErrMsg = ret;
            }
        }

        static constexpr void check_requirements();

        #define SQLITEPP_DECLARE_IMPL(xname, name) \
            static std::remove_pointer_t<decltype(sqlite3_module::xname)> name##_impl
        #define SQLITEPP_DECLARE_CONDITIONAL_IMPL(xname, name) \
            SQLITEPP_DECLARE_IMPL(xname, name); \
            static constexpr decltype(sqlite3_module::xname) get_##name##_impl()

        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xCreate, create);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xConnect, connect);
        SQLITEPP_DECLARE_IMPL(xBestIndex, best_index);
        SQLITEPP_DECLARE_IMPL(xDisconnect, disconnect);
        SQLITEPP_DECLARE_IMPL(xDestroy, destroy);
        SQLITEPP_DECLARE_IMPL(xOpen, open);
        SQLITEPP_DECLARE_IMPL(xClose, close);
        SQLITEPP_DECLARE_IMPL(xEof, eof);
        SQLITEPP_DECLARE_IMPL(xFilter, filter);
        SQLITEPP_DECLARE_IMPL(xNext, next);
        SQLITEPP_DECLARE_IMPL(xColumn, column);
        SQLITEPP_DECLARE_IMPL(xRowid, rowid);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xUpdate, update);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xFindFunction, find_function);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xBegin, begin);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xSync, sync);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xCommit, commit);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xRollback, rollback);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xRename, rename);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xSavepoint, savepoint);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xRelease, release);
        SQLITEPP_DECLARE_CONDITIONAL_IMPL(xRollbackTo, rollback_to);

        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)

            SQLITEPP_DECLARE_CONDITIONAL_IMPL(xShadowName, shadow_name);

        #endif

        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)

            SQLITEPP_DECLARE_CONDITIONAL_IMPL(xIntegrity, integrity);

        #endif

        #undef SQLITEPP_DECLARE_IMPL
        #undef SQLITEPP_DECLARE_CONDITIONAL_IMPL

    };

    /** @} */

}

namespace thinsqlitepp
{
    #if __cpp_designated_initializers >= 201707L
        #define SQLITEPP_DESIGNATED(name, value)  .name = value
    #else
        #define SQLITEPP_DESIGNATED(name, value)  value
    #endif

    template<class Derived>
    inline sqlite3_module * vtab<Derived>::get_module()
    {
        check_requirements();
        static sqlite3_module the_module = {
            SQLITEPP_DESIGNATED(iVersion,       4),
            SQLITEPP_DESIGNATED(xCreate,        get_create_impl()),
            SQLITEPP_DESIGNATED(xConnect,       get_connect_impl()),
            SQLITEPP_DESIGNATED(xBestIndex,     best_index_impl),
            SQLITEPP_DESIGNATED(xDisconnect,    disconnect_impl),
            SQLITEPP_DESIGNATED(xDestroy,       destroy_impl),
            SQLITEPP_DESIGNATED(xOpen,          open_impl),
            SQLITEPP_DESIGNATED(xClose,         close_impl),
            SQLITEPP_DESIGNATED(xFilter,        filter_impl),
            SQLITEPP_DESIGNATED(xNext,          next_impl),
            SQLITEPP_DESIGNATED(xEof,           eof_impl),
            SQLITEPP_DESIGNATED(xColumn,        column_impl),
            SQLITEPP_DESIGNATED(xRowid,         rowid_impl),
            SQLITEPP_DESIGNATED(xUpdate,        get_update_impl()),
            SQLITEPP_DESIGNATED(xBegin,         get_begin_impl()),
            SQLITEPP_DESIGNATED(xSync,          get_sync_impl()),
            SQLITEPP_DESIGNATED(xCommit,        get_commit_impl()),
            SQLITEPP_DESIGNATED(xRollback,      get_rollback_impl()),
            SQLITEPP_DESIGNATED(xFindFunction,  get_find_function_impl()),
            SQLITEPP_DESIGNATED(xRename,        get_rename_impl()),
            SQLITEPP_DESIGNATED(xSavepoint,     get_savepoint_impl()),
            SQLITEPP_DESIGNATED(xRelease,       get_release_impl()),
            SQLITEPP_DESIGNATED(xRollbackTo,    get_rollback_to_impl()),
            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)
            SQLITEPP_DESIGNATED(xShadowName,    get_shadow_name_impl()),
            #endif
            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)
            SQLITEPP_DESIGNATED(xIntegrity,     get_integrity_impl()),
            #endif
        };

        return &the_module;
    }

    #undef SQLITEPP_DESIGNATED

    //Detect vtab related things existing in a class
    struct vtab_detector
    {
    public:
        template<class T> static constexpr bool has_common_constructor = std::is_void_v<typename T::constructor_data_type> ?
                                                                                 std::is_constructible_v<T, 
                                                                                                         database *, 
                                                                                                         int, 
                                                                                                         const char * const *> :
                                                                                 std::is_constructible_v<T, 
                                                                                                         database *, 
                                                                                                         typename T::constructor_data_type, 
                                                                                                         int, 
                                                                                                         const char * const *>;

        template<class T> static constexpr bool has_create_constructor = std::is_void_v<typename T::constructor_data_type> ?
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::create_t, 
                                                                                                        database *, 
                                                                                                        int, 
                                                                                                        const char * const *> :
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::create_t, 
                                                                                                        database *, 
                                                                                                        typename T::constructor_data_type, 
                                                                                                        int, 
                                                                                                        const char * const *>;
        template<class T> static constexpr bool has_connect_constructor = std::is_void_v<typename T::constructor_data_type> ?
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::connect_t, 
                                                                                                        database *, 
                                                                                                        int, 
                                                                                                        const char * const *> :
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::connect_t, 
                                                                                                        database *, 
                                                                                                        typename T::constructor_data_type, 
                                                                                                        int, 
                                                                                                        const char * const *>;

        SQLITEPP_STATIC_METHOD_DETECTOR(void, disconnect, std::unique_ptr<T>{});
        SQLITEPP_STATIC_METHOD_DETECTOR(void, destroy, std::unique_ptr<T>{});

        SQLITEPP_METHOD_DETECTOR(int64_t, update, int{}, (value **)nullptr);
        SQLITEPP_METHOD_DETECTOR(int, find_function, int{}, 
                                                     (const char *)nullptr, 
                                                     (void (**)(context*,int,value**) noexcept)nullptr,
                                                     (void **)nullptr);
        SQLITEPP_METHOD_DETECTOR_0(void, begin);
        SQLITEPP_METHOD_DETECTOR_0(void, sync);
        SQLITEPP_METHOD_DETECTOR_0(void, commit);
        SQLITEPP_METHOD_DETECTOR_0(void, rollback);
        SQLITEPP_METHOD_DETECTOR(void, rename, (const char *)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, savepoint, int{});
        SQLITEPP_METHOD_DETECTOR(void, release, int{});
        SQLITEPP_METHOD_DETECTOR(void, rollback_to, int{});
        SQLITEPP_STATIC_METHOD_DETECTOR(bool, shadow_name, (const char *)nullptr);
        SQLITEPP_METHOD_DETECTOR(allocated_string, integrity, (const char *)nullptr, (const char *)nullptr, int{});
    };

    #if __cpp_concepts >= 201907L && __cpp_lib_concepts >= 202002L

        template<class T>
        concept is_vtab = 
            std::is_base_of_v<vtab<T>, T> && 
            (std::is_void_v<typename T::constructor_data_type> || std::is_pointer_v<typename T::constructor_data_type>) &&
            (std::is_void_v<typename T::index_data_type> || 
                (std::is_pointer_v<typename T::index_data_type> && std::is_trivially_destructible_v<typename T::index_data_type>)) &&
            std::is_base_of_v<typename vtab<T>::cursor, typename T::cursor> &&
            requires(T obj, const T cobj, index_info<typename T::index_data_type> & index) 
            {
                { cobj.best_index(index) } -> std::convertible_to<bool>;
                { obj.open() } -> std::convertible_to<std::unique_ptr<typename T::cursor>>;
            } &&
            requires(typename T::cursor & cur, const typename T::cursor & ccur, context & ctxt)
            {
                { ccur.eof() } noexcept -> std::convertible_to<bool>;
                requires 
                    (
                        !std::is_void_v<typename T::index_data_type> && 
                        requires { { cur.filter(int{}, (typename T::index_data_type *)nullptr, int{}, (value **)nullptr) } -> std::same_as<void>; }
                    ) || (
                        std::is_void_v<typename T::index_data_type> && 
                        requires { { cur.filter(int{}, int{}, (value **)nullptr) } -> std::same_as<void>; }
                    );
                { cur.next() } -> std::same_as<void>;
                { ccur.column(ctxt, int{}) } -> std::same_as<void>;
                { ccur.rowid() } -> std::same_as<int64_t>;
            };

    #endif

    template<class Derived>
    constexpr void vtab<Derived>::check_requirements()
    {
        #if __cpp_concepts >= 201907L && __cpp_lib_concepts >= 202002L

            static_assert(is_vtab<Derived>);

        #endif

        if constexpr (vtab_detector::has_common_constructor<Derived>)
        {
            static_assert(!vtab_detector::has_create_constructor<Derived>,
                            "if you declare a common constructor you cannot also have a constructor that takes create_t");
            static_assert(!vtab_detector::has_connect_constructor<Derived>,
                            "if you declare a common constructor you cannot also have a constructor that takes connect_t");
        }
        else
        {
            static_assert(vtab_detector::has_connect_constructor<Derived>,
                          "you must declare either a constructor that takes a connect_t OR a common constructor");

            #if SQLITE_VERSION_NUMBER < SQLITEPP_SQLITE_VERSION(3, 9, 0)

                static_assert(vtab_detector::has_create_constructor<Derived>,
                              "you must declare either a constructor that takes a create_t OR a common constructor");

            #endif
        }

        if constexpr (vtab_detector::has_disconnect<Derived>) 
            static_assert(vtab_detector::has_noexcept_disconnect<Derived>, "disconnect() must be noexcept");

        if constexpr (vtab_detector::has_destroy<Derived>) 
            static_assert(vtab_detector::has_noexcept_destroy<Derived>, "destroy() must be noexcept");

        if constexpr (vtab_detector::has_find_function<Derived>) 
            static_assert(vtab_detector::has_noexcept_find_function<Derived>, "find_function() must be noexcept");

        if constexpr (vtab_detector::has_shadow_name<Derived>) 
            static_assert(vtab_detector::has_noexcept_shadow_name<Derived>, "shadow_name() must be noexcept");

        static_assert(std::is_base_of_v<vtab<Derived>::cursor, typename Derived::cursor>,
                      "Derived::cursor type must derive from vtab<Derived>::cursor");
    }

    template<class Derived>
    int vtab<Derived>::create_impl(sqlite3 * db, [[maybe_unused]] void * aux, int argc, const char * const * argv, sqlite3_vtab ** pp_vtab, char ** err)
    {
        try 
        {
            if constexpr (vtab_detector::has_common_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else if constexpr (vtab_detector::has_create_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(create_t{}, database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(create_t{}, database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else
                static_assert(dependent_false<Derived>, "neither required constructor form is present");
            return SQLITE_OK;
        }
        catch(exception & ex) 
        {
            auto message = ex.error().extract_message();
            *err = (char *)message.release();
        }
        catch(std::exception & ex)
        {
            auto message = ex.what();
            const auto len = strlen(message) + 1;
            if (char * const ret = (char *)sqlite_allocate_nothrow(len))
            {
                memcpy(ret, message, len);
                *err = ret;
            }
        }
        return SQLITE_ERROR;
    }

    template<class Derived>
    int vtab<Derived>::connect_impl(sqlite3 * db, [[maybe_unused]] void * aux, int argc, const char * const * argv, sqlite3_vtab ** pp_vtab, char ** err)
    {
        try 
        {
            if constexpr (vtab_detector::has_common_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else if constexpr(vtab_detector::has_connect_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(connect_t{}, database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(connect_t{}, database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else
                static_assert(dependent_false<Derived>, "neither required constructor form is present");
            return SQLITE_OK;
        }
        catch(exception & ex) 
        {
            auto message = ex.error().extract_message();
            *err = (char *)message.release();
        }
        catch(std::exception & ex)
        {
            auto message = ex.what();
            const auto len = strlen(message) + 1;
            if (char * const ret = (char *)sqlite_allocate_nothrow(len))
            {
                memcpy(ret, message, len);
                *err = ret;
            }
        }
        return SQLITE_ERROR;
    }

    template<class Derived>
    constexpr decltype(sqlite3_module::xCreate) vtab<Derived>::get_create_impl() 
    {
        if constexpr (vtab_detector::has_common_constructor<Derived> || vtab_detector::has_create_constructor<Derived>)
        {
            return create_impl;
        }
        else 
        {
            return nullptr;
        }
    }

    template<class Derived>
    constexpr decltype(sqlite3_module::xCreate) vtab<Derived>::get_connect_impl() 
    {
        if constexpr (vtab_detector::has_common_constructor<Derived>)
        {
            return create_impl;
        }
        else
        {
            return connect_impl;
        }
    }

    #define SQLITEPP_BEGIN_CALLBACK try
    #define SQLITEPP_END_CALLBACK   catch(exception & ex) { \
                                        me->set_error_message(ex); \
                                        return ex.extended_error_code(); \
                                    } catch(std::exception & ex) { \
                                        me->set_error_message(ex); \
                                        return SQLITE_ERROR; \
                                    }

    template<class Derived>
    int vtab<Derived>::best_index_impl(sqlite3_vtab * vtab, sqlite3_index_info * info)
    {
        const auto * me = static_cast<Derived *>(vtab);
        auto myinfo = index_info<typename Derived::index_data_type>::from(info);
        SQLITEPP_BEGIN_CALLBACK 
        {
            bool res = me->best_index(*myinfo);
            return res ? SQLITE_OK : SQLITE_CONSTRAINT;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::disconnect_impl(sqlite3_vtab * vtab)
    {
        auto me = std::unique_ptr<Derived>(static_cast<Derived *>(vtab));

        if constexpr (vtab_detector::has_disconnect<Derived>) {
            Derived::disconnect(std::move(me));
        }
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::destroy_impl(sqlite3_vtab * vtab)
    {
        auto me = std::unique_ptr<Derived>(static_cast<Derived *>(vtab));

        if constexpr (vtab_detector::has_destroy<Derived>) {
            Derived::destroy(std::move(me));
        }
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::open_impl(sqlite3_vtab * vtab, sqlite3_vtab_cursor ** cursor)
    {
        auto me = static_cast<Derived *>(vtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            std::unique_ptr<typename Derived::cursor> res = me->open();
            *cursor = res.release()->c_ptr();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::close_impl(sqlite3_vtab_cursor * cursor)
    {
        auto me = static_cast<typename Derived::cursor *>(cursor);
        delete me;
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::eof_impl(sqlite3_vtab_cursor * cursor)
    {
        auto me = static_cast<typename Derived::cursor *>(cursor);

        static_assert(noexcept(me->eof()), "cursor's eof() must be noexcept");

        bool res = me->eof();
        return res;
    }

    template<class Derived>
    int vtab<Derived>::filter_impl(sqlite3_vtab_cursor * cursor, int idx_num, const char * idx_str,
                                   int argc, sqlite3_value ** argv)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            if constexpr (std::is_void_v<typename Derived::index_data_type>)
                me_cursor->filter(idx_num, argc, (value **)argv);
            else
                me_cursor->filter(idx_num, (typename Derived::index_data_type)idx_str, 
                                  argc, (value **)argv);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::next_impl(sqlite3_vtab_cursor * cursor)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            me_cursor->next();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::column_impl(sqlite3_vtab_cursor * cursor, sqlite3_context * ctxt, int n)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);
        auto myctxt = context::from(ctxt);

        SQLITEPP_BEGIN_CALLBACK
        {
            me_cursor->column(*myctxt, n);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::rowid_impl(sqlite3_vtab_cursor * cursor, sqlite_int64 * rowid)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            *rowid = sqlite_int64(me_cursor->rowid());
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::update_impl(sqlite3_vtab * vtab, int argc, sqlite3_value ** argv, sqlite_int64 * rowid)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            *rowid = sqlite_int64(me->update(argc, (value **)argv));
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::find_function_impl(sqlite3_vtab * vtab, int n_arg, const char * name, 
                                          void (**func)(sqlite3_context*,int,sqlite3_value**), void ** args)
    {
        auto me = static_cast<Derived *>(vtab);
        return me->find_function(n_arg, name, (void (**)(context*,int,value**) noexcept)func, args);
    }

    template<class Derived>
    int vtab<Derived>::begin_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->begin();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK

    }

    template<class Derived>
    int vtab<Derived>::sync_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->sync();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK

    }

    template<class Derived>
    int vtab<Derived>::commit_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->commit();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK

    }

    template<class Derived>
    int vtab<Derived>::rollback_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->rollback();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::rename_impl(sqlite3_vtab * vtab, const char * new_name)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->rename(new_name);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::savepoint_impl(sqlite3_vtab * vtab, int point)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->savepoint(point);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::release_impl(sqlite3_vtab * vtab, int point )
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->release(point);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::rollback_to_impl(sqlite3_vtab * vtab, int point)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->rollback_to(point);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)

        template<class Derived>
        int vtab<Derived>::shadow_name_impl(const char * name) 
        {
            return Derived::shadow_name(name);
        }
    #endif

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)

        template<class Derived>
        int vtab<Derived>::integrity_impl(sqlite3_vtab * vtab, const char * schema,
                                          const char * table_name, int flags, char ** err)
        {
            auto me = static_cast<Derived *>(vtab);
            SQLITEPP_BEGIN_CALLBACK
            {
                auto message =  me->integrity(schema, table_name, flags);
                *err = message.release();
                return SQLITE_OK;
            }
            SQLITEPP_END_CALLBACK
        }

    #endif

    #undef SQLITEPP_END_CALLBACK
    #undef SQLITEPP_BEGIN_CALLBACK

    #define SQLITEPP_SIMPLE_GET_IMPL(xname, name) \
    template<class Derived> \
    constexpr decltype(sqlite3_module::xname) vtab<Derived>::get_##name##_impl() \
    { \
        if constexpr (vtab_detector::has_##name<Derived>) \
            return name##_impl; \
        else \
            return nullptr; \
    }

    SQLITEPP_SIMPLE_GET_IMPL(xUpdate, update)
    SQLITEPP_SIMPLE_GET_IMPL(xFindFunction, find_function)
    SQLITEPP_SIMPLE_GET_IMPL(xBegin, begin)
    SQLITEPP_SIMPLE_GET_IMPL(xSync, sync)
    SQLITEPP_SIMPLE_GET_IMPL(xCommit, commit)
    SQLITEPP_SIMPLE_GET_IMPL(xRollback, rollback)
    SQLITEPP_SIMPLE_GET_IMPL(xRename, rename)
    SQLITEPP_SIMPLE_GET_IMPL(xSavepoint, savepoint)
    SQLITEPP_SIMPLE_GET_IMPL(xRelease, release)
    SQLITEPP_SIMPLE_GET_IMPL(xRollbackTo, rollback_to)

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)
        SQLITEPP_SIMPLE_GET_IMPL(xShadowName, shadow_name)
    #endif

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)
        SQLITEPP_SIMPLE_GET_IMPL(xIntegrity, integrity)
    #endif

    #undef SQLITEPP_SIMPLE_GET_IMPL

}
