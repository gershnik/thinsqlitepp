#include <ostream>
#include <string_view>
#include <vector>
#include <algorithm>
#include <iterator>
#include <doctest.h>
#include "mock_sqlite.hpp"

#if !SQLITEPP_USE_MODULES
#include <thinsqlitepp/global.hpp>
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

#if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 24, 0)

#if __cpp_lib_ranges >= 201911L
static_assert(std::random_access_iterator<keywords::const_iterator>);
static_assert(std::ranges::random_access_range<keywords>);
static_assert(std::ranges::sized_range<keywords>);
#endif

TEST_SUITE_BEGIN("keywords");

TEST_CASE( "size and empty" ) {
    keywords kw;

    CHECK(kw.size() > 0);
    CHECK(!kw.empty());

    //size() agrees with iterator distance
    CHECK(kw.size() == std::distance(kw.begin(), kw.end()));
    CHECK(kw.size() == std::distance(kw.cbegin(), kw.cend()));

    CHECK(kw.begin() == kw.cbegin());
    CHECK(kw.end() == kw.cend());
}

TEST_CASE( "iteration and content" ) {
    keywords kw;

    std::vector<std::string_view> all;
    for (auto k : kw) {
        CHECK(!k.empty());
        all.push_back(k);
    }
    REQUIRE(int(all.size()) == kw.size());

    //operator[] matches what iteration produces
    for (int i = 0; i < kw.size(); ++i)
        CHECK(kw[i] == all[size_t(i)]);

    //every listed keyword is recognized by check()
    for (auto k : all)
        CHECK(keywords::check(k));

    //a well known keyword is present
    CHECK(std::find(all.begin(), all.end(), std::string_view("SELECT")) != all.end());
}

TEST_CASE( "check" ) {
    CHECK(keywords::check("SELECT"));
    CHECK(keywords::check("select"));   //case insensitive
    CHECK(keywords::check("Table"));
    CHECK(keywords::check("INSERT"));

    CHECK(!keywords::check("zzzznotakeyword"));
    CHECK(!keywords::check(""));
}

TEST_CASE( "random access iterator" ) {
    keywords kw;
    REQUIRE(kw.size() >= 4);

    const auto first = kw.begin();
    const auto last  = kw.end();

    //arithmetic
    CHECK(first + kw.size() == last);
    CHECK(last - first == kw.size());
    CHECK(last - kw.size() == first);
    CHECK((1 + first) == (first + 1));   //symmetric operator+

    //subscript
    CHECK(first[0] == *first);
    CHECK(first[1] == *(first + 1));

    //pre/post increment and decrement
    auto it = first;
    auto post = it++;
    CHECK(post == first);
    CHECK(it == first + 1);
    ++it;
    CHECK(it == first + 2);
    auto postdec = it--;
    CHECK(postdec == first + 2);
    CHECK(it == first + 1);
    --it;
    CHECK(it == first);

    //compound assignment
    it += 3;
    CHECK(it == first + 3);
    it -= 2;
    CHECK(it == first + 1);

    //ordering
    CHECK(first < last);
    CHECK(first <= first);
    CHECK(last  > first);
    CHECK(last  >= last);
    CHECK(first == kw.begin());
    CHECK(first != last);
}

TEST_CASE( "reverse iteration" ) {
    keywords kw;

    std::vector<std::string_view> forward(kw.begin(), kw.end());
    std::vector<std::string_view> reverse(kw.rbegin(), kw.rend());
    REQUIRE(reverse.size() == forward.size());

    std::reverse(forward.begin(), forward.end());
    CHECK(reverse == forward);

    //crbegin/crend produce the same as rbegin/rend
    std::vector<std::string_view> creverse(kw.crbegin(), kw.crend());
    CHECK(creverse == reverse);
}

TEST_SUITE_END();

#endif
