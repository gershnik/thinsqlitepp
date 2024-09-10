#include <doctest.h>

#include <thinsqlitepp/context.hpp>

#include <type_traits>
#include <ostream>

using namespace thinsqlitepp;
using namespace std;
using namespace std::literals;

namespace {
    template<class LHS, class RHS>
    bool equalRanges(const LHS & lhs, const RHS & rhs)
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
}

TEST_SUITE_BEGIN("context");

TEST_CASE( "type properties" ) {

    CHECK(is_class_v<context>);
    CHECK(is_final_v<context>);
    CHECK(is_empty_v<context>);
    CHECK(!is_polymorphic_v<context>);
    
    CHECK(!is_default_constructible_v<context>);
    CHECK(!is_copy_constructible_v<context>);
    CHECK(!is_copy_assignable_v<context>);
    CHECK(!is_move_assignable_v<context>);
    CHECK(!is_destructible_v<context>);
    CHECK(!is_swappable_v<context>);
}

TEST_CASE( "destructor matching" ) {

    context * ctxt = nullptr;
    {
        void foo(const char *) noexcept;
        using x = decltype(ctxt->result_reference(""sv, foo));
        static_assert(std::is_same_v<x, void>);
    }
    {
        auto foo = [](const char *) noexcept {};
        using x = decltype(ctxt->result_reference(""sv, foo));
        static_assert(std::is_same_v<x, void>);
    }
    
}

#include <thinsqlitepp/database.hpp>

TEST_CASE( "result" ) {
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    static void (*impl)(context * ctxt) noexcept;

    auto func = [] (context * ctxt, int, value **) noexcept {
        impl(ctxt);
    };

    db->create_function("haha", 0, SQLITE_UTF8, &func, nullptr);

    // NULL
    impl = [](context * ctxt) noexcept {
        ctxt->result(nullptr);
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_NULL);
        return true;
    });

    // INTEGER

    impl = [](context * ctxt) noexcept {
        ctxt->result(5);
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_INTEGER);
        CHECK(r[0].value<int>() == 5);
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result(int64_t(55));
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_INTEGER);
        CHECK(r[0].value<int>() == 55);
        return true;
    });

    // FLOAT

    impl = [](context * ctxt) noexcept {
        ctxt->result(12.);
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_FLOAT);
        CHECK(r[0].value<double>() == 12.);
        return true;
    });

    // TEXT
    impl = [](context * ctxt) noexcept {
        ctxt->result("abc");
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_TEXT);
        CHECK(r[0].value<std::string_view>() == "abc");
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result(std::string_view{});
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_TEXT);
        CHECK(r[0].value<std::string_view>() == "");
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference("abc");
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_TEXT);
        CHECK(r[0].value<std::string_view>() == "abc");
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference(std::string_view{});
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_TEXT);
        CHECK(r[0].value<std::string_view>() == "");
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference("abc", [](const char * ptr) noexcept {
            if (ptr)
                CHECK(std::string_view(ptr) == "abc");
            else
                CHECK(false);
        });
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_TEXT);
        CHECK(r[0].value<std::string_view>() == "abc");
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference(std::string_view{}, [](const char * ptr) noexcept {
            CHECK(ptr == nullptr);
        });
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_TEXT);
        CHECK(r[0].value<std::string_view>() == "");
        return true;
    });

    // BLOB

    static auto bytes = std::array{std::byte{1}, std::byte{2}, std::byte{3}};

    impl = [](context * ctxt) noexcept {
        ctxt->result(blob_view(bytes));
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_BLOB);
        CHECK(equalRanges(r[0].value<blob_view>(), bytes));
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result(blob_view{});
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_BLOB);
        CHECK(equalRanges(r[0].value<blob_view>(), blob_view{}));
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference(blob_view(bytes));
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_BLOB);
        CHECK(equalRanges(r[0].value<blob_view>(), bytes));
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference(blob_view{});
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_BLOB);
        CHECK(equalRanges(r[0].value<blob_view>(), blob_view{}));
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference(blob_view(bytes), [](const std::byte * ptr) noexcept {
            CHECK(ptr == bytes.data());
        });
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_BLOB);
        CHECK(equalRanges(r[0].value<blob_view>(), bytes));
        return true;
    });

    impl = [](context * ctxt) noexcept {
        ctxt->result_reference(blob_view{}, [](const std::byte * ptr) noexcept {
            CHECK(ptr == nullptr);
        });
    };

    db->exec("SELECT haha();", [](int, row r) noexcept {
        CHECK(r[0].type() == SQLITE_BLOB);
        CHECK(equalRanges(r[0].value<blob_view>(), blob_view{}));
        return true;
    });
}

TEST_SUITE_END();


