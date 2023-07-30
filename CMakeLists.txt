#
# Copyright 2019 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
#

cmake_minimum_required(VERSION 3.24)

file(READ VERSION THSQPP_VERSION)
if (NOT THSQPP_VERSION)
    message(FATAL_ERROR "Cannot determine library version (VERSION file not found)")
endif()
string(STRIP ${THSQPP_VERSION} THSQPP_VERSION)

project(ThinSQLite++ VERSION ${THSQPP_VERSION} LANGUAGES C CXX)


add_subdirectory(lib)

if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()

