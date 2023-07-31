#
# Copyright 2019 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS thinsqlitepp EXPORT thinsqlitepp FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT thinsqlitepp NAMESPACE thinsqlitepp:: FILE thinsqlitepp-exports.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/thinsqlitepp)

configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/thinsqlitepp-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/thinsqlitepp-config.cmake
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/thinsqlitepp
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/thinsqlitepp-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/thinsqlitepp-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/thinsqlitepp-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/thinsqlitepp
)

file(RELATIVE_PATH FROM_PCFILEDIR_TO_PREFIX ${CMAKE_INSTALL_FULL_DATAROOTDIR}/thinsqlitepp ${CMAKE_INSTALL_PREFIX})
string(REGEX REPLACE "/+$" "" FROM_PCFILEDIR_TO_PREFIX "${FROM_PCFILEDIR_TO_PREFIX}") 

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/thinsqlitepp.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/thinsqlitepp.pc
    @ONLY
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/thinsqlitepp.pc
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)