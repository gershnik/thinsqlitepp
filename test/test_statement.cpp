#include <string_view>
#include <ostream>
#include <vector>
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
static_assert(std::input_iterator<row_iterator>);
static_assert(std::ranges::random_access_range<row>);
static_assert(std::ranges::input_range<row_range>);
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

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 52, 0) && defined(SQLITE_ENABLE_CARRAY)

namespace {

    int  g_carray_destroy_count = 0;
    void * g_carray_destroy_arg = nullptr;

    void carray_test_destroy(void * p) noexcept
    {
        ++g_carray_destroy_count;
        g_carray_destroy_arg = p;
    }

    //Bind `arr` to "k IN carray(?1)" using the requested overload and return the match count.
    //overload: 0 = carray_bind (TRANSIENT), 1 = carray_bind_reference (STATIC), 2 = carray_bind with destructor
    template<class T>
    int carray_count(database & db, thinsqlitepp::span<T> arr, int overload)
    {
        auto stmt = statement::create(db, "SELECT count(*) FROM carray_t WHERE k IN carray(?1)");
        switch (overload)
        {
            case 0:  stmt->carray_bind(1, arr); break;
            case 1:  stmt->carray_bind_reference(1, arr); break;
            default: stmt->carray_bind(1, arr, &carray_test_destroy); break;
        }
        REQUIRE(stmt->step());
        return stmt->template column_value<int>(0);
    }
}

TEST_CASE( "statement carray_bind" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    auto reset_table = [&](const char * values) {
        db->exec("DROP TABLE IF EXISTS carray_t; CREATE TABLE carray_t(k)");
        db->exec(std::string("INSERT INTO carray_t(k) VALUES ") + values);
    };

    //Each block exercises all 3 overloads for one element type T.

    {   //int -> SQLITE_CARRAY_INT32
        reset_table("(10),(20),(30),(40)");
        std::vector<int> a{10, 30, 99};         //two of three present
        for (int ov = 0; ov < 3; ++ov)
            CHECK(carray_count<int>(*db, thinsqlitepp::span<int>(a.data(), a.size()), ov) == 2);
    }

    {   //int64_t -> SQLITE_CARRAY_INT64
        reset_table("(10),(20),(30),(40)");
        std::vector<int64_t> a{20, 40};
        for (int ov = 0; ov < 3; ++ov)
            CHECK(carray_count<int64_t>(*db, thinsqlitepp::span<int64_t>(a.data(), a.size()), ov) == 2);
    }

    {   //double -> SQLITE_CARRAY_DOUBLE
        reset_table("(1.5),(2.5),(3.5)");
        std::vector<double> a{1.5, 3.5, 9.9};
        for (int ov = 0; ov < 3; ++ov)
            CHECK(carray_count<double>(*db, thinsqlitepp::span<double>(a.data(), a.size()), ov) == 2);
    }

    {   //char * -> SQLITE_CARRAY_TEXT
        reset_table("('aa'),('bb'),('cc')");
        char s1[] = "aa";
        char s2[] = "cc";
        std::vector<char *> a{s1, s2};
        for (int ov = 0; ov < 3; ++ov)
            CHECK(carray_count<char *>(*db, thinsqlitepp::span<char *>(a.data(), a.size()), ov) == 2);
    }

#if __cpp_char8_t >= 201811
    {   //char8_t * -> SQLITE_CARRAY_TEXT
        reset_table("('aa'),('bb'),('cc')");
        char8_t s1[] = u8"aa";
        char8_t s2[] = u8"cc";
        std::vector<char8_t *> a{s1, s2};
        for (int ov = 0; ov < 3; ++ov)
            CHECK(carray_count<char8_t *>(*db, thinsqlitepp::span<char8_t *>(a.data(), a.size()), ov) == 2);
    }
#endif

    {   //iovec -> SQLITE_CARRAY_BLOB
        reset_table("(x'0102'),(x'0304'),(x'0506')");
        unsigned char b1[] = {1, 2};
        unsigned char b2[] = {5, 6};
        thinsqlitepp::iovec v1{b1, sizeof(b1)};
        thinsqlitepp::iovec v2{b2, sizeof(b2)};
        std::vector<thinsqlitepp::iovec> a{v1, v2};
        for (int ov = 0; ov < 3; ++ov)
            CHECK(carray_count<thinsqlitepp::iovec>(*db, thinsqlitepp::span<thinsqlitepp::iovec>(a.data(), a.size()), ov) == 2);
    }
}

TEST_CASE( "statement carray_bind custom destructor" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("DROP TABLE IF EXISTS carray_t; CREATE TABLE carray_t(k)");
    db->exec("INSERT INTO carray_t(k) VALUES (10),(20),(30),(40)");

    g_carray_destroy_count = 0;
    g_carray_destroy_arg = nullptr;

    std::vector<int> a{10, 30};
    {
        auto stmt = statement::create(*db, "SELECT count(*) FROM carray_t WHERE k IN carray(?1)");
        stmt->carray_bind(1, thinsqlitepp::span<int>(a.data(), a.size()), &carray_test_destroy, a.data());
        REQUIRE(stmt->step());
        CHECK(stmt->column_value<int>(0) == 2);

        //Destructor must not have run yet: the binding is still alive.
        CHECK(g_carray_destroy_count == 0);
    }   //statement finalized here -> SQLite releases the binding and invokes the destructor

    CHECK(g_carray_destroy_count == 1);
    //SQLite invokes the destructor with the array data pointer (pDel == aData).
    CHECK(g_carray_destroy_arg == static_cast<void *>(a.data()));
}

#endif

TEST_SUITE_END();
