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

