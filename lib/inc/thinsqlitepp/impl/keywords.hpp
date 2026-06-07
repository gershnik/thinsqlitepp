/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_KEYWORDS_INCLUDED
#define HEADER_SQLITEPP_KEYWORDS_INCLUDED

#include "exception_iface.hpp"

#include <string_view>
#include <iterator>

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

#endif

