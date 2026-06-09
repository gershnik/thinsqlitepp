#include <doctest.h>
#include "mock_sqlite.hpp"

#if !SQLITEPP_USE_MODULES
    #include <thinsqlitepp/global.hpp>
#else
    import thinsqlitepp;
#endif

using namespace thinsqlitepp;

TEST_SUITE_BEGIN("config");

TEST_CASE( "global config") {

    //config<SQLITE_CONFIG_MULTITHREAD>();
    
}

TEST_SUITE_END();

