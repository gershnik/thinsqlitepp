#include <doctest.h>
#include "mock_sqlite.hpp"

#include <thinsqlitepp/backup.hpp>
#include <thinsqlitepp/database.hpp>

using namespace thinsqlitepp;

TEST_SUITE_BEGIN("backup");

TEST_CASE( "backup type properties") {

    using namespace std;

    CHECK(is_class_v<backup>);
    CHECK(is_final_v<backup>);
    CHECK(is_empty_v<backup>);
    CHECK(!is_polymorphic_v<backup>);
    
    CHECK(!is_default_constructible_v<backup>);
    CHECK(!is_copy_constructible_v<backup>);
    CHECK(!is_copy_assignable_v<backup>);
    CHECK(!is_move_assignable_v<backup>);
    CHECK(is_destructible_v<backup>);
    CHECK(!is_swappable_v<backup>);
}

TEST_CASE( "basics" ) {
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    auto bdb = database::open("backup.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(value TEXT)  ");
    db->exec("INSERT INTO foo(value) VALUES ('abc'), ('xyz')");

    auto b = backup::init(*bdb, "main", *db, "main");
    REQUIRE(b);
    do
    {
        while(b->step(1) != backup::done)
            ;
        CHECK(b->pagecount() > 0);
    }
    while(b->remaining());

}

TEST_SUITE_END();
