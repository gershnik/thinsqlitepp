/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_ROW_ITERATOR_INCLUDED
#define HEADER_SQLITEPP_ROW_ITERATOR_INCLUDED

#include "statement_iface.hpp"

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
     * A [forward iterator](https://en.cppreference.com/w/cpp/iterator/forward_iterator) 
     * for @ref statement results.
     * 
     * This class stores the @ref statement *by reference*. Thus @ref statement must remain
     * valid for the lifetime duration of this class.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     */
    class row_iterator : private row
    {
    public:
        using value_type = row;
        using size_type = int;
        using difference_type = int;
        using reference = row;
        using pointer = void;
        using iterator_category = std::forward_iterator_tag;

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
        row_iterator operator++(int)
            { increment(); return *this; }
        
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
     * A [forward range](https://en.cppreference.com/w/cpp/ranges/forward_range) 
     * for @ref statement results.
     * 
     * This class stores the @ref statement *by reference*. Thus @ref statement must remain
     * valid for the lifetime duration of this class.
     * 
     * `#include <thinsqlitepp/statement.hpp>`
     */
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

#endif
