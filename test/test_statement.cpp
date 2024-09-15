#include <string_view>
#include <ostream>
#include <doctest.h>
#include "mock_sqlite.hpp"

#include <thinsqlitepp/statement.hpp>
#include <thinsqlitepp/database.hpp>

#if __cpp_lib_ranges >= 201911L
    #include <ranges>
#endif

using namespace thinsqlitepp;
using namespace std;

#if __cpp_lib_ranges >= 201911L
static_assert(std::forward_iterator<row_iterator>);
static_assert(std::ranges::random_access_range<row>);
static_assert(std::ranges::forward_range<row_range>);
#endif

TEST_SUITE_BEGIN("statement");

TEST_CASE( "statement type properties" ) {
    
    CHECK(is_class_v<statement>);
    CHECK(is_final_v<statement>);
    CHECK(is_empty_v<statement>);
    CHECK(!is_polymorphic_v<statement>);
    
    CHECK(!is_default_constructible_v<statement>);
    CHECK(!is_copy_constructible_v<statement>);
    CHECK(!is_copy_assignable_v<statement>);
    CHECK(!is_move_assignable_v<statement>);
    CHECK(is_destructible_v<statement>);
    CHECK(!is_swappable_v<statement>);
}

TEST_CASE( "statement looping" ) {
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(name TEXT PRIMARY key)  ");
    db->exec("INSERT INTO foo(name) VALUES ('abc'), ('xyz')");
    auto stmt = statement::create(*db, "SELECT * FROM foo");
    REQUIRE(stmt);
    
    CHECK(stmt->sql() == "SELECT * FROM foo"s);
#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 14, 0)
    CHECK(stmt->expanded_sql().get() == "SELECT * FROM foo"s);
#endif
    
    REQUIRE(stmt->column_count() == 1);
    
    CHECK(stmt->column_name(0) == "name"s);
    CHECK(stmt->column_database_name(0) == "main"s);
    CHECK(stmt->column_table_name(0) == "foo"s);
    
    std::string expected[] = { "abc", "xyz" };
    int resultIdx = 0;
    while(stmt->step())
    {
        REQUIRE(stmt->data_count() == 1);
        for(int idx = 0, count = stmt->data_count(); idx < count; ++idx)
        {
            CHECK(stmt->column_value<std::string_view>(idx) == expected[resultIdx++]);
        }
    }
    
    stmt->reset();
    
    resultIdx = 0;
    while(stmt->step())
    {
        row row(stmt);
        REQUIRE(row.size() == 1);
        for(auto cit = row.begin(), cend = row.end(); cit != cend; ++cit)
        {
            CHECK(cit->value<std::string_view>() == expected[resultIdx++]);
        }
    }
    
    stmt->reset();
    
    resultIdx = 0;
    for(row_iterator it(stmt), end; it != end; ++it)
    {
        REQUIRE(it->size() == 1);
        for(auto cit = it->begin(), cend = it->end(); cit != cend; ++cit)
        {
            CHECK(cit->value<std::string_view>() == expected[resultIdx++]);
        }
    }
    
}

TEST_SUITE_END();
