
include(FetchContent)

FetchContent_Declare(doctest
    URL  https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(doctest)

set (SQLITE_VERSIONS
    3.7.15.2
    3.34.0
    3.42.0
    3.45.0
)

set (CXX_STANDARDS
    17
    20
)

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
        $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/utf-8;/W4;/WX>
        $<$<CXX_COMPILER_ID:Clang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
        $<$<CXX_COMPILER_ID:AppleClang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
        $<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-pedantic>
    )

    target_compile_definitions(${target}
    PRIVATE
        CATCH_AMALGAMATED_CUSTOM_MAIN
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
        test_blob.cpp
        test_database.cpp
        test_main.cpp
        test_snapshot.cpp
        test_statement.cpp
        test_general.cpp
        test_context.cpp
        test_vtab.cpp
    )
    

endfunction()

set(TEST_COMMAND "")

foreach(SQLITE_VERSION ${SQLITE_VERSIONS})
    add_library(sqlite3-${SQLITE_VERSION} STATIC)

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
    )

    target_sources(sqlite3-${SQLITE_VERSION} PRIVATE
        sqlite/${SQLITE_VERSION}/sqlite3.c
    )

    foreach(CXX_STANDARD ${CXX_STANDARDS})

        add_executable(test-${CXX_STANDARD}-${SQLITE_VERSION})

        configure_test_target(test-${CXX_STANDARD}-${SQLITE_VERSION})

        set_target_properties(test-${CXX_STANDARD}-${SQLITE_VERSION} PROPERTIES
            CXX_STANDARD ${CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
        )

        target_link_libraries(test-${CXX_STANDARD}-${SQLITE_VERSION} 
        PRIVATE
            sqlite3-${SQLITE_VERSION}
        )

        target_compile_options(test-${CXX_STANDARD}-${SQLITE_VERSION}
        PRIVATE 
            $<$<BOOL:${EMSCRIPTEN}>:-fexceptions>
        )

        target_link_options(test-${CXX_STANDARD}-${SQLITE_VERSION}
        PRIVATE 
            $<$<BOOL:${EMSCRIPTEN}>:-sNO_DISABLE_EXCEPTION_CATCHING -fexceptions>
        )

        list(APPEND TEST_COMMAND COMMAND echo C++${CXX_STANDARD} SQLite-${SQLITE_VERSION})
        list(APPEND TEST_COMMAND COMMAND)
        if (DEFINED EMSCRIPTEN)
            list(APPEND TEST_COMMAND node --trace-uncaught $<TARGET_FILE:test-${CXX_STANDARD}-${SQLITE_VERSION}>)
        else()
            list(APPEND TEST_COMMAND $<TARGET_FILE:test-${CXX_STANDARD}-${SQLITE_VERSION}>)
        endif()
    
    endforeach()
endforeach()

find_package(SQLite3)
if (SQLite3_FOUND)
    message("Found native sqlite3 ${SQLite3_VERSION}, configuring native tests")

    foreach(CXX_STANDARD ${CXX_STANDARDS})

        add_executable(test-${CXX_STANDARD}-native)

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
        )

        list(APPEND TEST_COMMAND COMMAND echo C++${CXX_STANDARD} SQLite: native)
        list(APPEND TEST_COMMAND COMMAND)
        list(APPEND TEST_COMMAND test-${CXX_STANDARD}-native)

    endforeach()
else()
    message("Native sqlite3 not found, skipping native tests")
endif()

add_custom_target(run-test 
    ${TEST_COMMAND}
)
