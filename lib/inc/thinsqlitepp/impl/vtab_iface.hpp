/*
 Copyright 2024 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_VTAB_IFACE_INCLUDED
#define HEADER_SQLITEPP_VTAB_IFACE_INCLUDED

#include "database_iface.hpp"
#include "exception_iface.hpp"
#include "value_iface.hpp"
#include "context_iface.hpp"
#include "memory_iface.hpp"
#include "span.hpp"

#include <type_traits>
#include <string.h>

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
     */
    template<class T>
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
            sqlite3_uint64 columns_used() const noexcept
                { return this->c_ptr()->colUsed; }

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
         * Enabled only if template parameter T is non-void and
         * @p data pointer type can be converted to it.
         */
        template<class X>
        SQLITEPP_ENABLE_IF((std::is_convertible_v<X *, T>),
        void) set_index_data(std::unique_ptr<X, sqlite_deleter<X>> data) noexcept
            { set_index_data(data.release(), true); }

        /**
         * Set the index data
         * 
         * This is a convenience overload of set_index_data(T *, bool) that
         * takes a std::unique_pointer to T. 
         * 
         * Enabled only if template parameter T is a pointer to a class
         * derived from sqlite_allocated
         */
        template<class X>
        SQLITEPP_ENABLE_IF((
            std::is_convertible_v<X *, T> &&
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
            sqlite3_int64 estimated_rows() const noexcept
                { return this->c_ptr()->estimatedRows; }
            /**
             * Sets estimated number of rows returned
             * @since SQLite 3.8.2
             */
            void set_estimated_rows(sqlite3_int64 val) noexcept
                { this->c_ptr()->estimatedRows = val; }

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
     */
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
        static 
        SQLITEPP_ENABLE_IF((std::is_pointer_v<typename D::constructor_data_type>),
        void) create_module(database & db,
                            const string_param & name, 
                            typename D::constructor_data_type data, 
                            void(*destructor)(typename D::constructor_data_type) = nullptr)
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
        static 
        SQLITEPP_ENABLE_IF((std::is_pointer_v<typename D::constructor_data_type>),
        void) create_module(database & db,
                            const string_param & name, 
                            std::unique_ptr<std::remove_pointer_t<typename D::constructor_data_type>> data)
        {
            static_assert(std::is_same_v<D, Derived>, "please invoke this function only with default template parameter");
            db.create_module(name, vtab::get_module(), data.release(), [](typename D::constructor_data_type ptr) {
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
            if (char * const ret = (char *)sqlite3_malloc(int(len)))
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


#endif
