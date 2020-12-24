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

namespace thinsqlitepp
{
#if __cplusplus <= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG <= 201703L)

    template< class T > struct type_identity { using type = T; };

    template< class T > using type_identity_t = typename type_identity<T>::type;

#else

    template< class T > using type_identity = std::type_identity<T>;
    
    template< class T > using type_identity_t = std::type_identity_t<T>;

#endif

    //MARK:- is_pointer_to_callback

    template<class R, class T, class... ArgTypes>
    constexpr bool is_pointer_to_callback =  std::is_null_pointer_v<T> ||
        (std::is_pointer_v<T> && std::is_nothrow_invocable_r_v<R, std::remove_pointer_t<T>, ArgTypes...>);

    class context;
    class value;

    //MARK:- is_aggregate_function
    template<class T>
    constexpr bool is_aggregate_helper(decltype(std::declval<T>().step((context *)nullptr, int(), (value **)nullptr)) * a,
                                       decltype(std::declval<T>().last((context *)nullptr)) * b)
    {
        return std::is_same_v<std::remove_pointer_t<decltype(a)>, void> &&
               std::is_same_v<std::remove_pointer_t<decltype(b)>, void> &&
               noexcept(std::declval<T>().step((context *)nullptr, int(), (value **)nullptr)) &&
               noexcept(std::declval<T>().last((context *)nullptr));
    }
    
    template<class T>
    constexpr bool is_aggregate_helper(...)
        { return false; }

    template<class T>
    constexpr bool is_aggregate_function = is_aggregate_helper<T>(nullptr, nullptr);

    //MARK:- is_aggregate_window_function
    template<class T>
    constexpr bool is_aggregate_window_helper(decltype(std::declval<T>().inverse((context *)nullptr, int(), (value **)nullptr)) * a,
                                              decltype(std::declval<T>().current((context *)nullptr)) * b)
    {
        return std::is_same_v<std::remove_pointer_t<decltype(a)>, void> &&
               std::is_same_v<std::remove_pointer_t<decltype(b)>, void> &&
               noexcept(std::declval<T>().inverse((context *)nullptr, int(), (value **)nullptr)) &&
               noexcept(std::declval<T>().current((context *)nullptr));
    }

    template<class T>
    constexpr bool is_aggregate_window_helper(...)
        { return false; }

    template<class T>
    constexpr bool is_aggregate_window_function = is_aggregate_function<T> && is_aggregate_window_helper<T>(nullptr, nullptr);
    
}

#endif

