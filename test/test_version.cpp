#include <doctest.h>
#include "mock_sqlite.hpp"

#include <thinsqlitepp/version.hpp>

#include <string.h>

using namespace thinsqlitepp;

static_assert(sqlite_version::compile_time().value() == SQLITE_VERSION_NUMBER);

static_assert(sqlite_version(1) < sqlite_version(2));
static_assert(!(sqlite_version(2) < sqlite_version(2)));
static_assert(!(sqlite_version(3) < sqlite_version(2)));
static_assert(sqlite_version(1) <= sqlite_version(2));
static_assert(sqlite_version(2) <= sqlite_version(2));
static_assert(!(sqlite_version(3) <= sqlite_version(2)));
static_assert(sqlite_version(2) > sqlite_version(1));
static_assert(!(sqlite_version(2) > sqlite_version(2)));
static_assert(!(sqlite_version(2) > sqlite_version(3)));
static_assert(sqlite_version(2) >= sqlite_version(1));
static_assert(sqlite_version(2) >= sqlite_version(2));
static_assert(!(sqlite_version(2) >= sqlite_version(3)));

static_assert(sqlite_version(1) == sqlite_version(1));
static_assert(!(sqlite_version(1) == sqlite_version(2)));
static_assert(!(sqlite_version(1) != sqlite_version(1)));
static_assert(sqlite_version(1) != sqlite_version(2));

static_assert(sqlite_version::from_parts(3, 1, 0) < sqlite_version::from_parts(3, 2, 10));

TEST_SUITE_BEGIN("version");

TEST_CASE("runtime")
{
    CHECK(sqlite_version::compile_time() == sqlite_version::runtime());
    CHECK(strcmp(sqlite_version::compile_time_str(), sqlite_version::runtime_str()) == 0);
    CHECK(strcmp(sqlite_version::compile_time_sourceid(), sqlite_version::runtime_sourceid()) == 0);
}

TEST_SUITE_END();
