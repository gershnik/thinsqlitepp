#
# Copyright 2019 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
#

cmake_minimum_required(VERSION 3.16)


set(SRCDIR ${CMAKE_CURRENT_LIST_DIR})
set(LIBNAME thinsqlitepp)

add_library(${LIBNAME} INTERFACE)

target_include_directories(${LIBNAME}
    
    INTERFACE
    ${SRCDIR}/inc
)

target_sources(${LIBNAME}
    PRIVATE
        inc/thinsqlitepp/context.hpp
        inc/thinsqlitepp/database.hpp
        inc/thinsqlitepp/exception.hpp
        inc/thinsqlitepp/global.hpp
        inc/thinsqlitepp/mutex.hpp
        inc/thinsqlitepp/statement.hpp
        inc/thinsqlitepp/value.hpp

        inc/thinsqlitepp/impl/config.hpp
        inc/thinsqlitepp/impl/context_iface.hpp
        inc/thinsqlitepp/impl/database_iface.hpp
        inc/thinsqlitepp/impl/database_impl.hpp
        inc/thinsqlitepp/impl/exception_iface.hpp
        inc/thinsqlitepp/impl/exception_impl.hpp
        inc/thinsqlitepp/impl/global_iface.hpp
        inc/thinsqlitepp/impl/handle.hpp
        inc/thinsqlitepp/impl/memory.hpp
        inc/thinsqlitepp/impl/meta.hpp
        inc/thinsqlitepp/impl/mutex_iface.hpp
        inc/thinsqlitepp/impl/row_iterator.hpp
        inc/thinsqlitepp/impl/span.hpp
        inc/thinsqlitepp/impl/statement_iface.hpp
        inc/thinsqlitepp/impl/statement_impl.hpp
        inc/thinsqlitepp/impl/string_param.hpp
        inc/thinsqlitepp/impl/value_iface.hpp

)

