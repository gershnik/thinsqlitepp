#include <doctest.h>

#include <type_traits>
#include <memory>
#include <new>
#include <limits>
#include <vector>
#include <list>
#include <string>

#if !SQLITEPP_USE_MODULES
    #include <thinsqlitepp/memory.hpp>
#else
    #include "mock_sqlite.hpp"
    import thinsqlitepp;
#endif

using namespace thinsqlitepp;

namespace {

// ---- Mock helpers -----------------------------------------------------------
//
// alloc_recorder tracks malloc/malloc64/free invocations. Two flavors of
// install: one that hands out a fixed fake buffer (for tests that don't need
// real backing memory), and one that passes through to the real allocator
// (for functional tests that touch the returned memory).

struct alloc_recorder {
    int  alloc_count = 0;
    int  free_count  = 0;
    sqlite3_int64 last_size = 0;
    void * last_alloc = nullptr;
    void * last_free  = nullptr;
};

inline void install_fake_alloc(alloc_recorder & rec, void * fake) {
    set_mock_sqlite3_malloc([&rec, fake](int size) -> void * {
        ++rec.alloc_count;
        rec.last_size  = size;
        rec.last_alloc = fake;
        return fake;
    });
#if SQLITE_VERSION_NUMBER >= 3008007
    set_mock_sqlite3_malloc64([&rec, fake](sqlite3_uint64 size) -> void * {
        ++rec.alloc_count;
        rec.last_size  = sqlite3_int64(size);
        rec.last_alloc = fake;
        return fake;
    });
#endif
    set_mock_sqlite3_free([&rec](void * p) {
        if (!p) return;
        ++rec.free_count;
        rec.last_free = p;
    });
}

inline void install_failing_alloc() {
    set_mock_sqlite3_malloc([](int) -> void * { return nullptr; });
#if SQLITE_VERSION_NUMBER >= 3008007
    set_mock_sqlite3_malloc64([](sqlite3_uint64) -> void * { return nullptr; });
#endif
    // free won't be called on a nullptr, but install a no-op for safety
    set_mock_sqlite3_free([](void *) {});
}

inline void install_passthrough_alloc(alloc_recorder & rec) {
    set_mock_sqlite3_malloc([&rec](int size) -> void * {
        ++rec.alloc_count;
        rec.last_size = size;
        void * p = real_sqlite3_malloc(size);
        if (p) rec.last_alloc = p;
        return p;
    });
#if SQLITE_VERSION_NUMBER >= 3008007
    set_mock_sqlite3_malloc64([&rec](sqlite3_uint64 size) -> void * {
        ++rec.alloc_count;
        rec.last_size = sqlite3_int64(size);
        void * p = real_sqlite3_malloc64(size);
        if (p) rec.last_alloc = p;
        return p;
    });
#endif
    set_mock_sqlite3_free([&rec](void * p) {
        if (!p) return;
        ++rec.free_count;
        rec.last_free = p;
        real_sqlite3_free(p);
    });
}

} // namespace

TEST_SUITE_BEGIN("memory");

// -----------------------------------------------------------------------------
// sqlite_deleter
// -----------------------------------------------------------------------------

TEST_CASE("sqlite_deleter type properties") {
    using namespace std;

    CHECK(is_empty_v<sqlite_deleter<char>>);
    CHECK(is_trivially_default_constructible_v<sqlite_deleter<char>>);
    CHECK(is_trivially_copyable_v<sqlite_deleter<char>>);
    CHECK(is_nothrow_invocable_v<sqlite_deleter<char>, char *>);
    CHECK(is_nothrow_invocable_v<sqlite_deleter<std::byte>, std::byte *>);
}

TEST_CASE("sqlite_deleter invokes sqlite3_free with the given pointer") {
    mock_cleanup _;

    int call_count = 0;
    void * captured = nullptr;
    set_mock_sqlite3_free([&](void * p) {
        ++call_count;
        captured = p;
    });

    char sentinel[1] = { 0 };  // not really allocated, mock will swallow
    sqlite_deleter<char>{}(sentinel);

    CHECK(call_count == 1);
    CHECK(captured == sentinel);
}

TEST_CASE("sqlite_deleter is a no-op when called with nullptr") {
    // The contract of sqlite3_free is that NULL is a no-op. Our deleter
    // forwards unconditionally, so sqlite3_free MUST see the nullptr.
    mock_cleanup _;

    int call_count = 0;
    bool saw_null = false;
    set_mock_sqlite3_free([&](void * p) {
        ++call_count;
        if (p == nullptr) saw_null = true;
    });

    sqlite_deleter<char>{}(nullptr);
    CHECK(call_count == 1);
    CHECK(saw_null);
}

TEST_CASE("sqlite_deleter works with const T") {
    // Internally used as unique_ptr<const char, sqlite_deleter<const char>>.
    // Verify the const_cast inside operator() compiles and that the right
    // pointer is forwarded.
    mock_cleanup _;

    void * captured = nullptr;
    set_mock_sqlite3_free([&](void * p) { captured = p; });

    const char sentinel[1] = { 0 };
    sqlite_deleter<const char>{}(sentinel);
    CHECK(captured == static_cast<const void *>(sentinel));
}

// -----------------------------------------------------------------------------
// allocated_string / allocated_bytes
// -----------------------------------------------------------------------------

TEST_CASE("allocated_string is the documented unique_ptr alias") {
    using namespace std;
    CHECK((is_same_v<allocated_string,
                     unique_ptr<char, sqlite_deleter<char>>>));
}

TEST_CASE("allocated_bytes is the documented unique_ptr alias") {
    using namespace std;
    CHECK((is_same_v<allocated_bytes,
                     unique_ptr<std::byte, sqlite_deleter<std::byte>>>));
}

TEST_CASE("allocated_string destructor calls sqlite3_free") {
    mock_cleanup _;

    int free_count = 0;
    void * captured = nullptr;
    set_mock_sqlite3_free([&](void * p) {
        ++free_count;
        captured = p;
    });

    char buf[1] = { 0 };
    {
        allocated_string s(buf);
        REQUIRE(s);
    } // destructor must invoke sqlite3_free
    CHECK(free_count == 1);
    CHECK(captured == buf);
}

TEST_CASE("allocated_string move transfers ownership; no free until release") {
    mock_cleanup _;

    int free_count = 0;
    set_mock_sqlite3_free([&](void *) { ++free_count; });

    char buf[1] = { 0 };
    allocated_string s(buf);
    REQUIRE(s);

    allocated_string t = std::move(s);
    CHECK(!s);
    REQUIRE(t);
    CHECK(free_count == 0);

    t.reset();
    CHECK(free_count == 1);
}

// -----------------------------------------------------------------------------
// sqlite_allocated
// -----------------------------------------------------------------------------

namespace {
    struct counted : sqlite_allocated {
        static inline int alive = 0;
        int x;
        counted() noexcept : x(42) { ++alive; }
        explicit counted(int v) noexcept : x(v) { ++alive; }
        ~counted() noexcept { --alive; }
    };

    // Sized to host one or several `counted` objects safely.
    alignas(counted) unsigned char fake_storage[sizeof(counted) * 16];
}

TEST_CASE("sqlite_allocated::new invokes SQLite allocator with sizeof(T)") {
    mock_cleanup _;
    REQUIRE(counted::alive == 0);

    alloc_recorder rec;
    install_fake_alloc(rec, fake_storage);

    auto * p = new counted;
    CHECK(rec.alloc_count == 1);
    CHECK(rec.last_size >= sqlite3_int64(sizeof(counted)));
    CHECK(static_cast<void *>(p) == fake_storage);
    CHECK(p->x == 42);
    CHECK(counted::alive == 1);

    delete p;
    CHECK(rec.free_count == 1);
    CHECK(rec.last_free == fake_storage);
    CHECK(counted::alive == 0);
}

TEST_CASE("sqlite_allocated::new(nothrow) returns nullptr on OOM") {
    mock_cleanup _;
    install_failing_alloc();

    auto * p = new (std::nothrow) counted;
    CHECK(p == nullptr);
    CHECK(counted::alive == 0);
}

TEST_CASE("sqlite_allocated::new throws bad_alloc on OOM") {
    mock_cleanup _;
    install_failing_alloc();

    CHECK_THROWS_AS(new counted, std::bad_alloc);
    CHECK(counted::alive == 0);
}

TEST_CASE("sqlite_allocated::new[] / delete[] invoke SQLite allocator") {
    mock_cleanup _;
    REQUIRE(counted::alive == 0);

    alloc_recorder rec;
    install_fake_alloc(rec, fake_storage);

    constexpr int N = 4;
    auto * arr = new counted[N];
    CHECK(rec.alloc_count == 1);
    // operator new[] is asked for at least N * sizeof(counted) plus possibly
    // a small cookie for the array length.
    CHECK(rec.last_size >= sqlite3_int64(N * sizeof(counted)));
    CHECK(counted::alive == N);

    delete[] arr;
    CHECK(rec.free_count == 1);
    CHECK(counted::alive == 0);
}

TEST_CASE("sqlite_allocated::new[](nothrow) returns nullptr on OOM") {
    mock_cleanup _;
    install_failing_alloc();

    auto * arr = new (std::nothrow) counted[4];
    CHECK(arr == nullptr);
    CHECK(counted::alive == 0);
}

// -----------------------------------------------------------------------------
// sqlite_allocator
//
// These would have caught the void* -> T* implicit-conversion bug in
// sqlite_allocator<T>::allocate(). The template body is never instantiated
// anywhere else in the codebase, so the bug went undetected. Simply
// *compiling* the tests below fails against the unfixed library.
// -----------------------------------------------------------------------------

TEST_CASE("sqlite_allocator type properties") {
    using namespace std;
    using A = sqlite_allocator<int>;

    CHECK((is_same_v<A::value_type, int>));
    CHECK(is_default_constructible_v<A>);
    CHECK(is_copy_constructible_v<A>);
    CHECK(is_nothrow_copy_constructible_v<A>);

    sqlite_allocator<char> ac;
    sqlite_allocator<int> ai(ac);   // rebinding constructor
    (void)ai;
}

TEST_CASE("sqlite_allocator equality is universal (stateless)") {
    sqlite_allocator<int> a1;
    sqlite_allocator<int> a2;
    sqlite_allocator<char> ac;

    CHECK(a1 == a2);
    CHECK_FALSE(a1 != a2);
    CHECK(a1 == ac);
    CHECK_FALSE(a1 != ac);
}

TEST_CASE("sqlite_allocator::allocate forwards to sqlite3_malloc(64)") {
    mock_cleanup _;

    alloc_recorder rec;
    install_fake_alloc(rec, fake_storage);

    sqlite_allocator<int> a;
    int * p = a.allocate(4);

    CHECK(rec.alloc_count == 1);
    CHECK(rec.last_size == sqlite3_int64(4 * sizeof(int)));
    CHECK(static_cast<void *>(p) == fake_storage);

    a.deallocate(p, 4);
    CHECK(rec.free_count == 1);
    CHECK(rec.last_free == fake_storage);
}

TEST_CASE("sqlite_allocator::allocate returns the correctly-typed pointer") {
    // This is the specific behavior broken by the void* -> T* bug: even if
    // we ignore the result, the test must compile.
    mock_cleanup _;
    alloc_recorder rec;
    install_fake_alloc(rec, fake_storage);

    sqlite_allocator<int> a;
    int * p = a.allocate(1);
    static_assert(std::is_same_v<decltype(p), int *>);
    a.deallocate(p, 1);

    sqlite_allocator<double> a2;
    double * q = a2.allocate(1);
    static_assert(std::is_same_v<decltype(q), double *>);
    a2.deallocate(q, 1);
}

TEST_CASE("sqlite_allocator::allocate throws bad_alloc on OOM") {
    mock_cleanup _;
    install_failing_alloc();

    sqlite_allocator<char> a;
    CHECK_THROWS_AS(a.allocate(64), std::bad_alloc);
}

TEST_CASE("sqlite_allocator overflow check throws bad_array_new_length") {
    // n * sizeof(T) overflows -- caught before the allocator is touched,
    // so no mock setup is needed (and we verify allocator was NOT called).
    mock_cleanup _;
    alloc_recorder rec;
    install_passthrough_alloc(rec);

    sqlite_allocator<int> a;
    CHECK_THROWS_AS(a.allocate(std::numeric_limits<std::size_t>::max()),
                    std::bad_array_new_length);
    CHECK(rec.alloc_count == 0);
}

TEST_CASE("sqlite_allocator works with std::vector") {
    mock_cleanup _;
    alloc_recorder rec;
    install_passthrough_alloc(rec);

    {
        std::vector<int, sqlite_allocator<int>> v;
        v.reserve(64);
        for (int i = 0; i < 64; ++i)
            v.push_back(i * 3);

        REQUIRE(v.size() == 64);
        for (int i = 0; i < 64; ++i)
            CHECK(v[size_t(i)] == i * 3);
    }

    // At least one alloc happened (reserve) and matching frees ran.
    CHECK(rec.alloc_count >= 1);
    CHECK(rec.free_count == rec.alloc_count);
}

TEST_CASE("sqlite_allocator works with std::list (cross-rebinding)") {
    // std::list rebinds the allocator to its internal node type, exercising
    // the rebinding constructor path. Pre-fix, this was a compile error.
    mock_cleanup _;
    alloc_recorder rec;
    install_passthrough_alloc(rec);

    {
        std::list<int, sqlite_allocator<int>> lst;
        for (int i = 1; i <= 5; ++i)
            lst.push_back(i);

        int sum = 0;
        for (int v : lst) sum += v;
        CHECK(sum == 15);
        CHECK(lst.size() == 5);
    }

    CHECK(rec.alloc_count >= 5);   // one node alloc per push_back
    CHECK(rec.free_count == rec.alloc_count);
}

TEST_CASE("sqlite_allocator works with std::basic_string") {
    mock_cleanup _;
    alloc_recorder rec;
    install_passthrough_alloc(rec);

    {
        using S = std::basic_string<char, std::char_traits<char>,
                                    sqlite_allocator<char>>;
        S s;
        for (int i = 0; i < 64; ++i)
            s.push_back(char('a' + (i % 26)));
        CHECK(s.size() == 64);
        CHECK(s[0] == 'a');
        CHECK(s[25] == 'z');
        CHECK(s[26] == 'a');
    }

    CHECK(rec.free_count == rec.alloc_count);
}

// -----------------------------------------------------------------------------
// Deprecated `deleter<T>` alias
// -----------------------------------------------------------------------------

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4996)
#elif defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

TEST_CASE("legacy `deleter` alias still resolves to sqlite_deleter") {
    CHECK((std::is_same_v<deleter<char>, sqlite_deleter<char>>));
    CHECK((std::is_same_v<deleter<std::byte>, sqlite_deleter<std::byte>>));
}

#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

TEST_SUITE_END();
