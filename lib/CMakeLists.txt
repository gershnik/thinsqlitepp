#
# Copyright 2019 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
#

cmake_minimum_required(VERSION 3.24)

if (NOT THSQPP_VERSION)
    file(READ VERSION THSQPP_VERSION)
    if (NOT THSQPP_VERSION)
        message(FATAL_ERROR "Cannot determine library version (VERSION file not found)")
    endif()
    string(STRIP ${THSQPP_VERSION} THSQPP_VERSION)
endif()
project(ThinSQLite++ VERSION ${THSQPP_VERSION})

set(SRCDIR ${CMAKE_CURRENT_LIST_DIR})
set(LIBNAME thinsqlitepp)

add_library(${LIBNAME} INTERFACE)
add_library(${LIBNAME}::${LIBNAME} ALIAS ${LIBNAME})

set_target_properties(${LIBNAME} PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_features(${LIBNAME} 
INTERFACE 
    cxx_std_17
)

target_include_directories(${LIBNAME}
INTERFACE
    $<BUILD_INTERFACE:${SRCDIR}/inc>
    $<INSTALL_INTERFACE:include>  # <prefix>/include
)

set(PUBLIC_HEADERS
    inc/thinsqlitepp/context.hpp
    inc/thinsqlitepp/database.hpp
    inc/thinsqlitepp/exception.hpp
    inc/thinsqlitepp/global.hpp
    inc/thinsqlitepp/mutex.hpp
    inc/thinsqlitepp/statement.hpp
    inc/thinsqlitepp/value.hpp
)
source_group("Public Headers" FILES ${PUBLIC_HEADERS})

set(IMPL_HEADERS
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
    inc/thinsqlitepp/impl/statement_iface.hpp
    inc/thinsqlitepp/impl/statement_impl.hpp
    inc/thinsqlitepp/impl/span.hpp
    inc/thinsqlitepp/impl/string_param.hpp
    inc/thinsqlitepp/impl/value_iface.hpp
)
source_group("Implementation Headers" FILES ${IMPL_HEADERS})

target_sources(${LIBNAME}
INTERFACE 
    FILE_SET HEADERS BASE_DIRS ${SRCDIR}/inc FILES
        ${PUBLIC_HEADERS}
        ${IMPL_HEADERS}
PRIVATE
    ${PUBLIC_HEADERS}
    ${IMPL_HEADERS}
)

if (PROJECT_IS_TOP_LEVEL)
    include(cmake/install.cmake)
endif()

