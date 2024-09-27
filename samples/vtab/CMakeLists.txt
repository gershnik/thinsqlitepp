cmake_minimum_required(VERSION 3.24)

project(sample)

add_subdirectory(../../lib lib)

add_executable(sample)

target_link_libraries(sample
PRIVATE
    thinsqlitepp::thinsqlitepp
    sqlite3
)

target_sources(sample
PRIVATE
    main.cpp
)


