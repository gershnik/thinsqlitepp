/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_SPAN_INCLUDED
#define HEADER_SQLITEPP_SPAN_INCLUDED

#include "config.hpp"

#include <iterator>
#include <cassert>

#if __cplusplus > 201703L

    #include <span>

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
        template<class T>
        using span = std::span<T>;
    
        /** @} */
    }

#else

    #include <iterator>
    #include <array>

    namespace thinsqlitepp
    {

        template <class T> class span;

        namespace span_internal
        {
            template <class T>
            struct is_span_impl : std::false_type {};
             
            template <class T>
            struct is_span_impl<span<T>> : std::true_type {};
             
            template <class T>
            using is_span = is_span_impl<std::decay_t<T>>;
             
            template <class T>
            struct is_std_array_impl : std::false_type {};
             
            template <class T, size_t N>
            struct is_std_array_impl<std::array<T, N>> : std::true_type {};
             
            template <class T>
            using is_std_array = is_std_array_impl<std::decay_t<T>>;
             
            template <class T>
            using is_c_array = std::is_array<std::remove_reference_t<T>>;
             
            template <class From, class To>
            using is_legal_data_conversion = std::is_convertible<From (*)[], To (*)[]>;
             
            template <class Container, class T>
            using container_has_convertible_data = is_legal_data_conversion<
                std::remove_pointer_t<decltype(std::data(std::declval<Container>()))>,
                T>;
             
            template <class Container>
            using container_has_integral_size =
                std::is_integral<decltype(std::size(std::declval<Container>()))>;
             
            // SFINAE check if Container can be converted to a Span<T>.
            template <class Container, class T>
            using is_span_compatible_container =
                std::conditional_t<!is_span<Container>::value &&
                                   !is_std_array<Container>::value &&
                                   !is_c_array<Container>::value &&
                                   container_has_convertible_data<Container, T>::value &&
                                   container_has_integral_size<Container>::value,
                                    std::true_type,
                                    std::false_type>;
             

        }

        template <class T>
        class span
        {
        public:
            using element_type           = T;
            using value_type             = std::remove_cv_t<T>;
            using index_type             = size_t;
            using difference_type        = ptrdiff_t;
            using pointer                = T *;
            using const_pointer          = const T *;
            using reference              = T &;
            using const_reference        = const T &;
            using iterator               = pointer;
            using const_iterator         = const_pointer;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        public:
            constexpr span() noexcept : _data{nullptr}, _size{0} {}

            constexpr span           (const span&) noexcept = default;
            constexpr span& operator=(const span&) noexcept = default;

            constexpr span(pointer ptr, index_type count) noexcept : _data{ptr}, _size{count} {}
            constexpr span(pointer first, pointer last) noexcept : _data{first}, _size{static_cast<size_t>(std::distance(first, last))} {}

            template <size_t N>
            constexpr span(element_type (&arr)[N]) noexcept : _data{arr}, _size{N} {}

            template <size_t N>
            constexpr span(std::array<value_type, N>& arr) noexcept : _data{arr.data()}, _size{N} {}

            template <size_t N>
            constexpr span(const std::array<value_type, N>& arr) noexcept : _data{arr.data()}, _size{N} {}

            template <class Container>
            constexpr span(Container& cont,
                    std::enable_if_t<span_internal::is_span_compatible_container<Container, T>::value, std::nullptr_t> = nullptr)
                : _data{std::data(cont)}, _size{(index_type) std::size(cont)} {}

            template <class Container>
            constexpr span(const Container& cont,
                    std::enable_if_t<span_internal::is_span_compatible_container<const Container, T>::value, std::nullptr_t> = nullptr)
                : _data{std::data(cont)}, _size{(index_type) std::size(cont)} {}


            template <class OtherElementType>
            constexpr span(const span<OtherElementType>& other,
                               std::enable_if_t<
                                  std::is_convertible_v<OtherElementType(*)[], element_type (*)[]>,
                                  std::nullptr_t> = nullptr) noexcept
                : _data{other.data()}, _size{other.size()} {}


            constexpr span<element_type> first(index_type count) const noexcept
            {
                assert(count <= size());
                return {data(), count};
            }

            constexpr span<element_type> last (index_type count) const noexcept
            {
                assert(count <= size());
                return {data() + size() - count, count};
            }

            constexpr span<element_type>
            subspan(index_type offset, index_type count = size_t(-1)) const noexcept
            {
                assert(offset <= size());
                assert(count  <= size() || count == size_t(-1));
                if (count == size_t(-1))
                    return {data() + offset, size() - offset};
                assert(offset <= size() - count);
                return {data() + offset, count};
            }

            constexpr index_type size()       const noexcept { return _size; }
            constexpr index_type size_bytes() const noexcept { return _size * sizeof(element_type); }
            constexpr bool empty()            const noexcept { return _size == 0; }

            constexpr reference operator[](index_type idx) const noexcept
            {
                assert(idx < size());
                return _data[idx];
            }

            constexpr reference front() const noexcept
            {
                assert(!empty());
                return _data[0];
            }

            constexpr reference back() const noexcept
            {
                assert(!empty());
                return _data[size()-1];
            }


            constexpr pointer data()                         const noexcept { return _data; }

            constexpr iterator                 begin() const noexcept { return iterator(data()); }
            constexpr iterator                   end() const noexcept { return iterator(data() + size()); }
            constexpr const_iterator          cbegin() const noexcept { return const_iterator(data()); }
            constexpr const_iterator            cend() const noexcept { return const_iterator(data() + size()); }
            constexpr reverse_iterator        rbegin() const noexcept { return reverse_iterator(end()); }
            constexpr reverse_iterator          rend() const noexcept { return reverse_iterator(begin()); }
            constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
            constexpr const_reverse_iterator   crend() const noexcept { return const_reverse_iterator(cbegin()); }

            constexpr void swap(span &other) noexcept
            {
                pointer p = _data;
                _data = other._data;
                other._data = p;

                index_type s = _size;
                _size = other._size;
                other._size = s;
            }

        private:
            pointer    _data;
            index_type _size;
        };

    }

#endif


namespace thinsqlitepp
{
    /**
     * @addtogroup Utility Utilities
     * @{
     */

    /// A blob_view is a span of bytes
    using blob_view = span<const std::byte>;
    

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
            constexpr const_iterator & operator+=(difference_type diff) noexcept { _idx += diff; return *this; }
            
            constexpr const_iterator & operator--() noexcept { --_idx; return *this; }
            constexpr const_iterator operator--(int) noexcept { return _idx--; }
            constexpr const_iterator & operator-=(difference_type diff) noexcept { _idx -= diff; return *this; }
            
            friend constexpr difference_type operator-(const_iterator lhs, const_iterator rhs) noexcept { return lhs._idx - rhs._idx; }
            
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

#endif

