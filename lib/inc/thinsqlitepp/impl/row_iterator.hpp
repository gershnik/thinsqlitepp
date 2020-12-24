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
    class cell
    {
    public:
        cell(const statement * owner, int idx) noexcept:
            _owner(owner), _idx(idx)
        {}
        cell(const std::unique_ptr<statement> & owner, int idx) noexcept:
            cell(owner.get(), idx)
        {}
        
        int type() const noexcept
            { return _owner->column_type(_idx); }
        const char * name() const noexcept
            { return _owner->column_name(_idx); }
        template<class T> T value() const noexcept
            { return _owner->column_value<T>(_idx); }
        
        const char * database_name() const noexcept
            { return _owner->column_database_name(_idx); }
        const char * table_name() const noexcept
            { return _owner->column_table_name(_idx); }
        const char * origin_name() const noexcept
            { return _owner->column_origin_name(_idx); }
        const char * declared_type() const noexcept
            { return _owner->column_declared_type(_idx); }
        
    protected:
        const statement * _owner;
        int _idx;
    };

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
        row(const statement * owner) noexcept:
            _owner(owner)
        {}
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
        row_iterator() noexcept:
            row(nullptr)
        {}
        row_iterator(statement * owner):
            row(owner)
        {
            if (_owner)
                increment();
        }
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

}

#endif
