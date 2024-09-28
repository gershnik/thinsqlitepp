#include <doctest.h>
#include "mock_sqlite.hpp"

#include <thinsqlitepp/blob.hpp>
#include <thinsqlitepp/database.hpp>
#include <thinsqlitepp/statement.hpp>

#include <vector>

using namespace thinsqlitepp;
using namespace std;
using namespace std::literals;


TEST_SUITE_BEGIN("database");

TEST_CASE( "blob type properties") {

    CHECK(is_class_v<blob>);
    CHECK(is_final_v<blob>);
    CHECK(is_empty_v<blob>);
    CHECK(!is_polymorphic_v<blob>);
    
    CHECK(!is_default_constructible_v<blob>);
    CHECK(!is_copy_constructible_v<blob>);
    CHECK(!is_copy_assignable_v<blob>);
    CHECK(!is_move_assignable_v<blob>);
    CHECK(is_destructible_v<blob>);
    CHECK(!is_swappable_v<blob>);
}


TEST_CASE( "basics" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(value BLOB)  ");
    db->exec("INSERT INTO foo(value) VALUES ('abc'), ('xyz')");

    auto b = blob::open(*db, "main", "foo", "value", 1, true);
    REQUIRE(b);
    std::vector<std::byte> buf(10);
    b->read(0, thinsqlitepp::span<std::byte>(buf.data(), 3));
    CHECK(buf[0] == std::byte('a'));
    CHECK(buf[1] == std::byte('b'));
    CHECK(buf[2] == std::byte('c'));

    buf[3] = std::byte('q');
    buf[4] = std::byte('r');
    buf[5] = std::byte('s');

    b->write(0, thinsqlitepp::span<std::byte>(buf.data() + 3, 3));

    db->exec("SELECT value FROM foo WHERE rowid = 1", [](int, row r) noexcept {
        auto val = r[0].value<blob_view>();
        CHECK(val[0] == std::byte('q'));
        CHECK(val[1] == std::byte('r'));
        CHECK(val[2] == std::byte('s'));
        return true;
    });

    b->reopen(2);
    b->read(0, thinsqlitepp::span<std::byte>(buf.data(), 3));
    CHECK(buf[0] == std::byte('x'));
    CHECK(buf[1] == std::byte('y'));
    CHECK(buf[2] == std::byte('z'));
}

#if __cpp_lib_ranges >= 201911L

TEST_CASE( "ranges" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(value BLOB)  ");
    db->exec("INSERT INTO foo(value) VALUES (x'01020304'), (x'01020304')");

    auto b = blob::open(*db, "main", "foo", "value", 1, true);
    REQUIRE(b);

    std::vector<int32_t> buf(1);
    b->read(0, buf);
    union {
        uint8_t byte[4];
        int32_t value;
    } int_by_bytes = {.byte = {0x1, 0x2, 0x3, 0x4}};
    CHECK(buf[0] == int_by_bytes.value);

    int_by_bytes.value = 0x05060708;
    buf[0] = int_by_bytes.value;

    b->write(0, buf);
    db->exec("SELECT value FROM foo WHERE rowid = 1", [&](int, row r) noexcept {
        auto val = r[0].value<blob_view>();
        CHECK(val[0] == std::byte(int_by_bytes.byte[0]));
        CHECK(val[1] == std::byte(int_by_bytes.byte[1]));
        CHECK(val[2] == std::byte(int_by_bytes.byte[2]));
        CHECK(val[3] == std::byte(int_by_bytes.byte[3]));
        return true;
    });
}

#endif


TEST_SUITE_END();
