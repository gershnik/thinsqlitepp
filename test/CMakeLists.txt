cmake_minimum_required(VERSION 3.16)


foreach(SQLITE_VERSION ${SQLITE_VERSIONS})

    add_executable(test-${SQLITE_VERSION})


    target_include_directories(test-${SQLITE_VERSION} PRIVATE
        ../lib/inc
    )

    target_link_libraries(test-${SQLITE_VERSION} PRIVATE
        thinsqlitepp
        sqlite3-${SQLITE_VERSION}
        "$<$<PLATFORM_ID:Linux>:dl>"
    )

    #target_compile_definitions(test-${SQLITE_VERSION} PRIVATE
    #    SQLITE_OMIT_LOAD_EXTENSION=1
    #)

    set(LIB_PUBLIC_HEADERS
        ../lib/inc/thinsqlitepp/context.hpp
        ../lib/inc/thinsqlitepp/database.hpp
        ../lib/inc/thinsqlitepp/exception.hpp
        ../lib/inc/thinsqlitepp/global.hpp
        ../lib/inc/thinsqlitepp/mutex.hpp
        ../lib/inc/thinsqlitepp/statement.hpp
        ../lib/inc/thinsqlitepp/value.hpp
    )
    source_group("Library Headers" FILES ${LIB_PUBLIC_HEADERS})

    set(LIB_IMPL_HEADERS
        ../lib/inc/thinsqlitepp/impl/config.hpp
        ../lib/inc/thinsqlitepp/impl/context_iface.hpp
        ../lib/inc/thinsqlitepp/impl/database_iface.hpp
        ../lib/inc/thinsqlitepp/impl/database_impl.hpp
        ../lib/inc/thinsqlitepp/impl/exception_iface.hpp
        ../lib/inc/thinsqlitepp/impl/exception_impl.hpp
        ../lib/inc/thinsqlitepp/impl/global_iface.hpp
        ../lib/inc/thinsqlitepp/impl/handle.hpp
        ../lib/inc/thinsqlitepp/impl/memory.hpp
        ../lib/inc/thinsqlitepp/impl/meta.hpp
        ../lib/inc/thinsqlitepp/impl/mutex_iface.hpp
        ../lib/inc/thinsqlitepp/impl/row_iterator.hpp
        ../lib/inc/thinsqlitepp/impl/statement_iface.hpp
        ../lib/inc/thinsqlitepp/impl/statement_impl.hpp
        ../lib/inc/thinsqlitepp/impl/span.hpp
        ../lib/inc/thinsqlitepp/impl/string_param.hpp
        ../lib/inc/thinsqlitepp/impl/value_iface.hpp
    )
    source_group("Library Implementation Headers" FILES ${LIB_IMPL_HEADERS})

    target_sources(test-${SQLITE_VERSION} PRIVATE

        mock_sqlite.hpp
        mock_sqlite.cpp
        test_database.cpp
        test_main.cpp
        test_statement.cpp
        test_general.cpp
        
        catch.hpp
     
        ${LIB_PUBLIC_HEADERS}
        ${LIB_IMPL_HEADERS}
    )
    
endforeach()

set(TEST_COMMAND "")
foreach(SQLITE_VERSION ${SQLITE_VERSIONS})
    list(APPEND TEST_COMMAND COMMAND)
    list(APPEND TEST_COMMAND test-${SQLITE_VERSION})
endforeach()


add_custom_target(run-test 
    ${TEST_COMMAND}
)
