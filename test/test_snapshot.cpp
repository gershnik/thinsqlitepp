#include <doctest.h>
#include "mock_sqlite.hpp"

#define THINSQLITEPP_ENABLE_EXPIREMENTAL 1

#include <thinsqlitepp/snapshot.hpp>
#include <thinsqlitepp/database.hpp>

using namespace thinsqlitepp;

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 10, 0)

TEST_SUITE_BEGIN("snapshot");

TEST_CASE( "snapshot type properties") {

    using namespace std;

    CHECK(is_class_v<snapshot>);
    CHECK(is_final_v<snapshot>);
    CHECK(is_empty_v<snapshot>);
    CHECK(!is_polymorphic_v<snapshot>);
    
    CHECK(!is_default_constructible_v<snapshot>);
    CHECK(!is_copy_constructible_v<snapshot>);
    CHECK(!is_copy_assignable_v<snapshot>);
    CHECK(!is_move_assignable_v<snapshot>);
    CHECK(is_destructible_v<snapshot>);
    CHECK(!is_swappable_v<snapshot>);
}

TEST_CASE( "basics" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("PRAGMA journal_mode=WAL");
    db->exec("BEGIN");
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(value TEXT)  ");
    db->exec("END");
    db->exec("BEGIN");  
    auto sn = db->get_snapshot("main");
    REQUIRE(sn);
    db->exec("INSERT INTO foo(value) VALUES ('abc'), ('xyz')");
    db->exec("END");
    db->exec("BEGIN");
    db->open_snapshot("main", *sn);
    db->exec("SELECT count(value) FROM foo", [](int, row r) noexcept {
        CHECK(r[0].value<int>() == 0);
        return true;
    });

}


TEST_SUITE_END();


#endif

