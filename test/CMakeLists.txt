set (SQLITE_VERSIONS
    3.7.15.2
    3.34.0
)

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
    )

    # target_compile_options(sqlite3-${SQLITE_VERSION} 
    # PRIVATE
    #     "$<$<CXX_COMPILER_ID:Clang>:${APPLE_CLANG_OPTIONS};-Wno-unused-function;-Wno-format-pedantic>"
    #     "$<$<C_COMPILER_ID:Clang>:${APPLE_CLANG_OPTIONS};-Wno-unused-function;-Wno-format-pedantic>"
    #     "$<$<CXX_COMPILER_ID:AppleClang>:${APPLE_CLANG_OPTIONS};-Wno-unused-function;-Wno-format-pedantic>"
    #     "$<$<CXX_COMPILER_ID:MSVC>:{MSVC_OPTIONS};/Wv:18;/W3;/wd4244;/wd4146;/wd4267;/wd4996>"
    #     "$<$<CXX_COMPILER_ID:GNU>:${GCC_OPTIONS}>"
    # )

    target_sources(sqlite3-${SQLITE_VERSION} PRIVATE
        sqlite/${SQLITE_VERSION}/sqlite3.c
    )

    add_executable(test-${SQLITE_VERSION})

    set_target_properties(test-${SQLITE_VERSION} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_compile_options(test-${SQLITE_VERSION} 
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/utf-8;/W4;/WX>
        $<$<CXX_COMPILER_ID:Clang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
        $<$<CXX_COMPILER_ID:AppleClang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
        $<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-pedantic>
    )

    target_compile_definitions(test-${SQLITE_VERSION} 
    PRIVATE
        CATCH_AMALGAMATED_CUSTOM_MAIN
    )

    target_link_libraries(test-${SQLITE_VERSION} 
    PRIVATE
        thinsqlitepp::thinsqlitepp
        sqlite3-${SQLITE_VERSION}
        "$<$<PLATFORM_ID:Linux>:dl>"
    )

    #target_compile_definitions(test-${SQLITE_VERSION} PRIVATE
    #    SQLITE_OMIT_LOAD_EXTENSION=1
    #)

    target_sources(test-${SQLITE_VERSION} PRIVATE

        mock_sqlite.hpp
        mock_sqlite.cpp
        test_database.cpp
        test_main.cpp
        test_statement.cpp
        test_general.cpp
        
        catch_amalgamated.hpp
        catch_amalgamated.cpp
    )
    

    list(APPEND TEST_COMMAND COMMAND)
    list(APPEND TEST_COMMAND test-${SQLITE_VERSION})
    
endforeach()


add_custom_target(run-test 
    ${TEST_COMMAND}
)
