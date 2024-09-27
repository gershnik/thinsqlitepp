cmake_minimum_required(VERSION 3.24)

project(sample)

# on macOS system provided SQLite has extensions disabled
# so let's use ours
set(SQLITE_DIR ../../test/sqlite/3.45.0)

add_subdirectory(../../lib lib)

add_executable(sample)

target_link_libraries(sample
PRIVATE
    thinsqlitepp::thinsqlitepp
)

target_include_directories(sample
PRIVATE
    ${SQLITE_DIR}
)

target_sources(sample
PRIVATE
    main.cpp
    ${SQLITE_DIR}/sqlite3.c
)

add_library(sample-extension SHARED)

target_link_libraries(sample-extension
PRIVATE
    thinsqlitepp::thinsqlitepp
)

target_include_directories(sample-extension
PRIVATE
    ${SQLITE_DIR}
)

target_sources(sample-extension
PRIVATE
    extension.cpp
)

add_dependencies(sample sample-extension)


