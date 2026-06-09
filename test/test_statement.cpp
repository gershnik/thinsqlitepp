#include <string_view>
#include <ostream>
#include <vector>
#include <memory>
#include <doctest.h>
#include "mock_sqlite.hpp"

#if !SQLITEPP_USE_MODULES
#include <thinsqlitepp/statement.hpp>
#include <thinsqlitepp/database.hpp>
#include <thinsqlitepp/context.hpp>
#endif

#if __cpp_lib_ranges >= 201911L
    #include <ranges>
#endif

#if SQLITEPP_USE_MODULES
#define SQLITEPP_SQLITE_VERSION(x, y, z) ((x) * 1000000 + (y) * 1000 + (z))
import thinsqlitepp;
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

namespace {

    //Callback bookkeeping for the bind_reference(..., unref) and bind_pointer destructor overloads.
    int          g_bind_cb_count = 0;
    const void * g_bind_cb_arg   = nullptr;

    void bind_text_unref(const char * p) noexcept       { ++g_bind_cb_count; g_bind_cb_arg = p; }
    void bind_blob_unref(const std::byte * p) noexcept  { ++g_bind_cb_count; g_bind_cb_arg = p; }
#if __cpp_char8_t >= 201811
    void bind_u8text_unref(const char8_t * p) noexcept  { ++g_bind_cb_count; g_bind_cb_arg = p; }
#endif

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)
    int g_ptr_destroy_count = 0;
    void ptr_int_destroy(int * p) noexcept { ++g_ptr_destroy_count; g_bind_cb_arg = p; }
#endif

    //A type whose live instance count we can observe, for the unique_ptr ownership overload.
    struct counted { static int alive; counted() { ++alive; } ~counted() { --alive; } };
    int counted::alive = 0;
}

TEST_CASE( "statement bind values" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    //Each block binds a value to "SELECT ?1" and reads it straight back.

    {   //null
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, nullptr);
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_NULL);
    }
    {   //int
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, 42);
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_INTEGER);
        CHECK(stmt->column_value<int>(0) == 42);
    }
    {   //int64_t
        const int64_t big = int64_t(1) << 40;
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, big);
        REQUIRE(stmt->step());
        CHECK(stmt->column_value<int64_t>(0) == big);
    }
    {   //double
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, 3.5);
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_FLOAT);
        CHECK(stmt->column_value<double>(0) == 3.5);
    }
    {   //std::string_view (text, by value)
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, std::string_view("hello"));
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_TEXT);
        CHECK(stmt->column_value<std::string_view>(0) == "hello");
    }
#if __cpp_char8_t >= 201811
    {   //std::u8string_view (text, by value)
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, std::u8string_view(u8"world"));
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_TEXT);
        CHECK(stmt->column_value<std::u8string_view>(0) == u8"world");
    }
#endif
    {   //blob_view (by value)
        const std::byte data[] { std::byte(1), std::byte(2), std::byte(3) };
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, blob_view(data));
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_BLOB);
        auto out = stmt->column_value<blob_view>(0);
        REQUIRE(out.size() == 3);
        CHECK(out[0] == std::byte(1));
        CHECK(out[2] == std::byte(3));
    }
    {   //zero_blob
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, zero_blob(4));
        REQUIRE(stmt->step());
        CHECK(stmt->column_type(0) == SQLITE_BLOB);
        auto out = stmt->column_value<blob_view>(0);
        REQUIRE(out.size() == 4);
        CHECK(out[0] == std::byte(0));
        CHECK(out[3] == std::byte(0));
    }
    {   //const value & (read a value from one statement, bind into another)
        auto src = statement::create(*db, "SELECT 123");
        REQUIRE(src->step());
        const value & v = src->raw_column_value(0);

        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind(1, v);
        REQUIRE(stmt->step());
        CHECK(stmt->column_value<int>(0) == 123);
    }
}

TEST_CASE( "statement bind_reference" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    {   //std::string_view (text, by reference / SQLITE_STATIC)
        std::string str = "reftext";
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind_reference(1, std::string_view(str));
        REQUIRE(stmt->step());
        CHECK(stmt->column_value<std::string_view>(0) == "reftext");
    }

    {   //std::string_view with custom unref
        g_bind_cb_count = 0;
        g_bind_cb_arg = nullptr;
        std::string str = "unreftext";
        {
            auto stmt = statement::create(*db, "SELECT ?1");
            stmt->bind_reference(1, std::string_view(str), &bind_text_unref);
            REQUIRE(stmt->step());
            CHECK(stmt->column_value<std::string_view>(0) == "unreftext");
            CHECK(g_bind_cb_count == 0);     //still referenced
        }                                    //finalize releases the reference
        CHECK(g_bind_cb_count == 1);
        CHECK(g_bind_cb_arg == str.data());
    }

#if __cpp_char8_t >= 201811
    {   //std::u8string_view (by reference)
        std::u8string str = u8"refu8";
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind_reference(1, std::u8string_view(str));
        REQUIRE(stmt->step());
        CHECK(stmt->column_value<std::u8string_view>(0) == u8"refu8");
    }

    {   //std::u8string_view with custom unref
        g_bind_cb_count = 0;
        g_bind_cb_arg = nullptr;
        std::u8string str = u8"unrefu8";
        {
            auto stmt = statement::create(*db, "SELECT ?1");
            stmt->bind_reference(1, std::u8string_view(str), &bind_u8text_unref);
            REQUIRE(stmt->step());
            CHECK(stmt->column_value<std::u8string_view>(0) == u8"unrefu8");
            CHECK(g_bind_cb_count == 0);
        }
        CHECK(g_bind_cb_count == 1);
        CHECK(g_bind_cb_arg == str.data());
    }
#endif

    {   //blob_view (by reference / SQLITE_STATIC)
        const std::byte data[] { std::byte(4), std::byte(5) };
        auto stmt = statement::create(*db, "SELECT ?1");
        stmt->bind_reference(1, blob_view(data));
        REQUIRE(stmt->step());
        auto out = stmt->column_value<blob_view>(0);
        REQUIRE(out.size() == 2);
        CHECK(out[0] == std::byte(4));
    }

    {   //blob_view with custom unref
        g_bind_cb_count = 0;
        g_bind_cb_arg = nullptr;
        const std::byte data[] { std::byte(6), std::byte(7) };
        {
            auto stmt = statement::create(*db, "SELECT ?1");
            stmt->bind_reference(1, blob_view(data), &bind_blob_unref);
            REQUIRE(stmt->step());
            auto out = stmt->column_value<blob_view>(0);
            REQUIRE(out.size() == 2);
            CHECK(out[1] == std::byte(7));
            CHECK(g_bind_cb_count == 0);
        }
        CHECK(g_bind_cb_count == 1);
        CHECK(g_bind_cb_arg == data);
    }
}


#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 20, 0)

TEST_CASE( "statement bind pointer" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    //A scalar SQL function that dereferences a bound "int_ptr" pointer argument.
    auto deref = [](context * ctxt, int, value ** vals) noexcept {
        int * p = vals[0]->get<int *>("int_ptr");
        ctxt->result(p ? *p : -1);
    };
    db->create_function("deref_int", 1, SQLITE_UTF8, &deref, nullptr);

    {   //bind(int, T *, const char *, void(*)(T *)) - raw pointer with custom destructor
        g_ptr_destroy_count = 0;
        g_bind_cb_arg = nullptr;
        int payload = 777;
        {
            auto stmt = statement::create(*db, "SELECT deref_int(?1)");
            stmt->bind(1, &payload, "int_ptr", &ptr_int_destroy);
            REQUIRE(stmt->step());
            CHECK(stmt->column_value<int>(0) == 777);   //pointer round-trips through the function
            CHECK(g_ptr_destroy_count == 0);            //destructor not called while bound
        }                                               //finalize invokes the destructor
        CHECK(g_ptr_destroy_count == 1);
        CHECK(g_bind_cb_arg == &payload);
    }

    {   //bind(int, std::unique_ptr<T>) - ownership transfer; object freed on finalize
        counted::alive = 0;
        {
            auto stmt = statement::create(*db, "SELECT ?1");
            stmt->bind(1, std::make_unique<counted>());
            CHECK(counted::alive == 1);                 //SQLite now owns it
            REQUIRE(stmt->step());
        }                                               //finalize deletes the managed object
        CHECK(counted::alive == 0);
    }
}

#endif

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
