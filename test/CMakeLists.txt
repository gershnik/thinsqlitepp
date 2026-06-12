
include(FetchContent)

FetchContent_Declare(doctest
    URL  https://raw.githubusercontent.com/doctest/doctest/v2.5.2/doctest/doctest.h
    DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(doctest)

find_package(SQLite3)
if (SQLite3_FOUND)
    message("Found native sqlite3 ${SQLite3_VERSION}, configuring native tests")
else()
    message("Native sqlite3 not found, skipping native tests")
endif()

if ("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
    set(FRONT_MSVC True)
else()
    set(FRONT_MSVC False)
endif()

set (CLANG_CL "$<AND:$<BOOL:${FRONT_MSVC}>,$<CXX_COMPILER_ID:Clang>>")
set (CLANG_NORMAL "$<AND:$<NOT:$<BOOL:${FRONT_MSVC}>>,$<CXX_COMPILER_ID:Clang>>")


set (SQLITE_VERSIONS
    3.7.15.2
    3.34.0
    3.42.0
    3.53.1
)

set (CXX_STANDARDS
    17
    20
    23
)

if (THSQPP_ENABLE_MODULE)
    if (CMAKE_VERSION VERSION_LESS "3.28")
        message(WARNING "CMake version ${CMAKE_VERSION} doesn't support modules, ignoring THSQPP_ENABLE_MODULE")
        set(THSQPP_ENABLE_MODULE False)
    endif()
endif()

function(configure_test_target target)

    set_target_properties(${target} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_compile_options(${target}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/utf-8;/Zc:preprocessor;/wd5285>
        $<${CLANG_NORMAL}:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move;-Wconversion;-Wsign-conversion>
        $<$<CXX_COMPILER_ID:AppleClang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move;-Wconversion;-Wsign-conversion>
        $<${CLANG_CL}:/W4 /WX -Wpedantic;-Wconversion;-Wsign-conversion;-Wno-cast-function-type-mismatch> 
        $<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-pedantic>
    )

    target_compile_definitions(${target}
    PRIVATE
        CATCH_AMALGAMATED_CUSTOM_MAIN
        "THINSQLITEPP_CUSTOM_SQLITE_H=\"${CMAKE_CURRENT_LIST_DIR}/mock_sqlite.hpp\""
        THINSQLITEPP_ENABLE_EXPERIMENTAL=1
        THINSQLITEPP_NO_MUTEX=1
    )

    target_link_libraries(${target}
    PRIVATE
        thinsqlitepp::thinsqlitepp
        "$<$<PLATFORM_ID:Linux>:dl>"
    )

    target_include_directories(${target}
    PRIVATE
        ${doctest_SOURCE_DIR}
    )

    target_sources(${target} 
    PRIVATE
        mock_sqlite.hpp
        mock_sqlite.cpp
        test_backup.cpp
        test_blob.cpp
        test_database.cpp
        test_keywords.cpp
        test_main.cpp
        test_memory.cpp
        test_snapshot.cpp
        test_statement.cpp
        test_general.cpp
        test_context.cpp
        test_version.cpp
        test_vtab.cpp
    )
    

endfunction()


foreach(SQLITE_VERSION ${SQLITE_VERSIONS})
    add_library(sqlite3-${SQLITE_VERSION} STATIC EXCLUDE_FROM_ALL)

    set_target_properties(sqlite3-${SQLITE_VERSION} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_include_directories(sqlite3-${SQLITE_VERSION} SYSTEM BEFORE
    PUBLIC
        sqlite/${SQLITE_VERSION}
    )

    target_compile_definitions(sqlite3-${SQLITE_VERSION} 
    PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>"
        SQLITE_THREADSAFE=0
        SQLITE_OMIT_DEPRECATED=1
        SQLITE_ENABLE_API_ARMOR=1
        SQLITE_ENABLE_MEMORY_MANAGEMENT=1
        SQLITE_MEMDEBUG=1
        SQLITE_ENABLE_COLUMN_METADATA=1
        SQLITE_ENABLE_SNAPSHOT=1
        SQLITE_ENABLE_SETLK_TIMEOUT=1
    PUBLIC
        SQLITE_ENABLE_PREUPDATE_HOOK=1
        SQLITE_ENABLE_CARRAY=1
    )

    target_sources(sqlite3-${SQLITE_VERSION} PRIVATE
        sqlite/${SQLITE_VERSION}/sqlite3.c
    )
endforeach()

add_custom_target(tests ALL)

foreach(CXX_STANDARD ${CXX_STANDARDS})

    add_custom_target(tests-${CXX_STANDARD})
    add_dependencies(tests tests-${CXX_STANDARD})
    
    foreach(SQLITE_VERSION ${SQLITE_VERSIONS})

        set(target test-${CXX_STANDARD}-${SQLITE_VERSION})

        add_executable(${target} EXCLUDE_FROM_ALL)

        configure_test_target(${target})

        set_target_properties(${target} PROPERTIES
            CXX_STANDARD ${CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
        )

        if (THSQPP_ENABLE_MODULE AND CXX_STANDARD GREATER_EQUAL 20)
            set_target_properties(${target} PROPERTIES
                CXX_SCAN_FOR_MODULES ON
            )
            target_compile_definitions(${target} PRIVATE
                SQLITEPP_USE_MODULES=1
            )
            thinsqlitepp_add_module(${target} PRIVATE)
        else()
            target_link_libraries(${target}
            PRIVATE
                thinsqlitepp::thinsqlitepp
            )
        endif()

        target_link_libraries(${target}
        PRIVATE
            sqlite3-${SQLITE_VERSION}
        )

        target_compile_options(${target}
        PRIVATE 
            $<$<BOOL:${EMSCRIPTEN}>:-fexceptions>
        )

        target_link_options(${target}
        PRIVATE 
            $<$<BOOL:${EMSCRIPTEN}>:-sNO_DISABLE_EXCEPTION_CATCHING -fexceptions>
        )

        if (DEFINED EMSCRIPTEN)
            set(TEST_COMMAND node --trace-uncaught $<TARGET_FILE:${target}> -ni)
        else()
            set(TEST_COMMAND $<TARGET_FILE:${target}> -ni -fc)
        endif()

        add_dependencies(tests-${CXX_STANDARD} ${target})

        add_test(
            NAME "C++${CXX_STANDARD} SQLite-${SQLITE_VERSION}"
            COMMAND ${TEST_COMMAND}
        )

    endforeach()

    if (SQLite3_FOUND)
        add_executable(test-${CXX_STANDARD}-native EXCLUDE_FROM_ALL)

        configure_test_target(test-${CXX_STANDARD}-native)

        set_target_properties(test-${CXX_STANDARD}-native PROPERTIES
            CXX_STANDARD ${CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
        )

        target_link_libraries(test-${CXX_STANDARD}-native 
        PRIVATE
            SQLite::SQLite3
        )

        target_compile_definitions(test-${CXX_STANDARD}-native  
        PRIVATE
            $<$<PLATFORM_ID:Darwin>:THINSQLITEPP_OMIT_LOAD_EXTENSION=1>
            THINSQLITEPP_OMIT_SNAPSHOT=1
            SQLITEPP_TEST_IMPRECISE_VERSION=1
        )

        add_dependencies(tests-${CXX_STANDARD} test-${CXX_STANDARD}-native)

        add_test(
            NAME "C++${CXX_STANDARD} SQLite: native"
            COMMAND test-${CXX_STANDARD}-native -ni -fc
        )

    endif()

endforeach()



