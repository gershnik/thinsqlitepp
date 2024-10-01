/*
 Copyright 2019 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_META_INCLUDED
#define HEADER_SQLITEPP_META_INCLUDED

#include "config.hpp"

#include <memory>
#include <type_traits>
#if __cpp_impl_three_way_comparison >= 201907
    #include <compare>
#endif
#if __cpp_lib_bit_cast >= 201806L
    #include <bit>
#endif

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
#define SQLITEPP_METHOD_DETECTOR_0(rettype, name) SQLITEPP_CALL_DETECTOR_0(name, rettype, ((T*)nullptr)->name)
#define SQLITEPP_METHOD_DETECTOR(rettype, name, ...) SQLITEPP_CALL_DETECTOR(name, rettype, ((T*)nullptr)->name, __VA_ARGS__)
    


namespace thinsqlitepp
{
#if __cplusplus <= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG <= 201703L)

    template< class T > struct type_identity { using type = T; };

    template< class T > using type_identity_t = typename type_identity<T>::type;

#else

    template< class T > using type_identity = std::type_identity<T>;
    
    template< class T > using type_identity_t = std::type_identity_t<T>;

#endif

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

    #if __cpp_impl_three_way_comparison >= 201907

    
        inline std::strong_ordering strong_ordering_from_int(int val)
        {
            using equivalent = size_equivalent<std::strong_ordering>;
            if constexpr (!std::is_void_v<equivalent>)
            {
                equivalent eq_val = equivalent((val > 0) - (val < 0));
                #if __cpp_lib_bit_cast >= 201806L
                    return std::bit_cast<std::strong_ordering>(eq_val);
                #else
                    return *(std::strong_ordering *)&eq_val;
                #endif
            }
            else
            {
                return val < 0 ? std::strong_ordering::less : (
                       val == 0 ? std::strong_ordering::equal : (
                       std::strong_ordering::greater 
                       ));
            }
        }

    #endif


    /** @endcond */
    
}

#endif

