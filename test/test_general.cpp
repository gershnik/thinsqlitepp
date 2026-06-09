#include <doctest.h>

#if !SQLITEPP_USE_MODULES
    #include <thinsqlitepp/global.hpp>
#else
    #include "mock_sqlite.hpp"
    import thinsqlitepp;
#endif

using namespace thinsqlitepp;

TEST_SUITE_BEGIN("config");

TEST_CASE( "global config") {

    //config<SQLITE_CONFIG_MULTITHREAD>();
    
}

TEST_SUITE_END();

