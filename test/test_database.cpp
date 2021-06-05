#include "catch.hpp"
#include "mock_sqlite.hpp"

#include <thinsqlitepp/database.hpp>

#include <type_traits>

using namespace thinsqlitepp;
using namespace std;
using namespace Catch;

TEST_CASE( "database type properties", "[database]") {

    CHECK(is_class_v<database>);
    CHECK(is_final_v<database>);
    CHECK(is_empty_v<database>);
    CHECK(!is_polymorphic_v<database>);
    
    CHECK(!is_default_constructible_v<database>);
    CHECK(!is_copy_constructible_v<database>);
    CHECK(!is_copy_assignable_v<database>);
    CHECK(!is_move_assignable_v<database>);
    CHECK(is_destructible_v<database>);
    CHECK(!is_swappable_v<database>);
}

#include <thinsqlitepp/context.hpp>
#include <thinsqlitepp/value.hpp>


class ErrorCodeMatcher : public MatcherBase<::thinsqlitepp::exception>
{
public:
    ErrorCodeMatcher(int error_code) : _error_code(error_code) {}

    bool match(::thinsqlitepp::exception const & in) const override
    {
        return in.primary_error_code() == _error_code;
    }

    std::string describe() const override
    {
        return "error code equals: " + std::to_string(_error_code);
    }
private:
    int _error_code;
};

ErrorCodeMatcher HasErrorCode(int val) {
    return ErrorCodeMatcher(val);
}

class sqlitepp_test_fixture
{
private:
    mock_cleanup _cleanup;
};



TEST_CASE_METHOD(sqlitepp_test_fixture,  "database creation", "[database]") {
 
    set_mock_sqlite3_open_v2([] (const char *filename, sqlite3 **ppDb, int flags, const char *zVfs) {
       
        REQUIRE_THAT(filename, Equals("foo.db"));
        REQUIRE(flags == (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX));
        REQUIRE(zVfs == nullptr);
        return real_sqlite3_open_v2(filename, ppDb, flags, zVfs);
    });
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    bool closed = false;
    set_mock_sqlite3_close_v2([&closed] (sqlite3 *db) {
       
        closed = true;
        return real_sqlite3_close_v2(db);
    });
    
    db.reset();
    REQUIRE(closed);
    
    int errcode = 0;
    set_mock_sqlite3_open_v2([&errcode] (const char *filename, sqlite3 **ppDb, int flags, const char *zVfs) {
       
        REQUIRE_THAT(filename, Equals("foo.db"));
        REQUIRE(flags == 0);
        REQUIRE(zVfs == nullptr);
        errcode = real_sqlite3_open_v2(filename, ppDb, flags, zVfs);
        REQUIRE(errcode == SQLITE_MISUSE);
        return errcode;
    });
    try
    {
        db = database::open("foo.db", 0);
        CHECK(false);
    }
    catch(::thinsqlitepp::exception & ex)
    {
        CHECK(ex.extended_error_code() == errcode);
        CHECK(ex.primary_error_code() == errcode);
        CHECK(ex.system_error_code() == 0);
    }
    
    db.reset();
    REQUIRE(closed);
    
    int sys_errcode = 0;
    set_mock_sqlite3_open_v2([&errcode, &sys_errcode] (const char *filename, sqlite3 **ppDb, int flags, const char *zVfs) {
       
        REQUIRE_THAT(filename, Equals("/foo/nosuch.db"));
        REQUIRE(flags == (SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX));
        REQUIRE(zVfs == nullptr);
        errcode = real_sqlite3_open_v2(filename, ppDb, flags, zVfs);
#if SQLITE_VERSION_NUMBER >= 3012000
        sys_errcode = sqlite3_system_errno(*ppDb);
#else
        sys_errcode = 0;
#endif
        REQUIRE(errcode == SQLITE_CANTOPEN);
        return errcode;
    });
    try
    {
        db = database::open("/foo/nosuch.db", SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
        CHECK(false);
    }
    catch(::thinsqlitepp::exception & ex)
    {
        CHECK(ex.extended_error_code() == errcode);
        CHECK(ex.primary_error_code() == errcode);
        CHECK(ex.system_error_code() == sys_errcode);
        CHECK_THAT(ex.what(), Equals("unable to open database file"));
    }
    
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "busy handler", "[database]") {
    
    auto db1 = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    int count_invoked;
    bool do_abort = false;
    
    auto h = [&] (int c) noexcept {
        if (!do_abort)
            return false;
        count_invoked = c;
        if (c == 0)
            return true;
        db1->exec("END TRANSACTION"); //not noexcept!
        return true;
    };
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    db1->exec("BEGIN EXCLUSIVE TRANSACTION");
    
    set_mock_sqlite3_busy_handler([&] (sqlite3 * dbx, int(*handler)(void*,int), void *data) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(data == &h);
        return real_sqlite3_busy_handler(dbx, handler, data);
    });
    db->busy_handler(&h);
    
    try
    {
        db->exec("BEGIN EXCLUSIVE TRANSACTION");
    }
    catch(::thinsqlitepp::exception & ex)
    {
        CHECK(ex.primary_error_code() == SQLITE_BUSY);
    }
    
    do_abort = true;
    db->exec("BEGIN EXCLUSIVE TRANSACTION");
    
    CHECK(count_invoked == 1);
    
    set_mock_sqlite3_busy_handler([&] (sqlite3 *dbx, int(*handler)(void*,int), void *data) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(handler == nullptr);
        REQUIRE(data == nullptr);
        return real_sqlite3_busy_handler(dbx, handler, data);
    });
    db->busy_handler(nullptr);
    
    clear_mock_sqlite3_busy_handler();
    //checking that this compiles
    db->busy_handler([](int *, int) noexcept {
        
        return 0;
        
    }, (int*)nullptr);
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "busy timeout", "[database]") {
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    set_mock_sqlite3_busy_timeout([&] (sqlite3 *dbx, int ms) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(ms == 5);
        return real_sqlite3_busy_timeout(dbx, ms);
    });
    db->busy_timeout(5);
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "changes", "[database]") {
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(name TEXT PRIMARY key)");
    
    set_mock_sqlite3_changes([&] (sqlite3 *dbx) {
        
        REQUIRE(dbx == db->c_ptr());
        return real_sqlite3_changes(dbx);
    });
    db->exec("INSERT INTO foo(name) VALUES ('abc')");
    CHECK(db->changes() == 1);
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "commit hook", "[database]") {
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(name TEXT PRIMARY key)");
    
    auto hook = [] () noexcept -> bool {
        
        return true; //convert to rollback
    };
    
    set_mock_sqlite3_commit_hook([&] (sqlite3 *dbx, int(*handler)(void*), void *data) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(data == &hook);
        return real_sqlite3_commit_hook(dbx, handler, data);
    });
    
    db->commit_hook(&hook);
    REQUIRE_THROWS_MATCHES(db->exec("INSERT INTO foo(name) VALUES ('abc')"), ::thinsqlitepp::exception, HasErrorCode(SQLITE_CONSTRAINT));
    
    set_mock_sqlite3_commit_hook([&] (sqlite3 *dbx, int(*handler)(void*), void *data) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(handler == nullptr);
        REQUIRE(data == nullptr);
        return real_sqlite3_commit_hook(dbx, handler, data);
    });
    db->commit_hook(nullptr);
    REQUIRE_NOTHROW(db->exec("INSERT INTO foo(name) VALUES ('abc')"));
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "rollback hook", "[database]") {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(name TEXT PRIMARY key)");
    
    bool called = false;
    auto hook = [&] () noexcept -> void {
        
        called = true;
    };
    set_mock_sqlite3_rollback_hook([&] (sqlite3 *dbx, void(*handler)(void*), void *data) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(data == &hook);
        return real_sqlite3_rollback_hook(dbx, handler, data);
    });
    db->rollback_hook(&hook);
    db->exec("BEGIN TRANSACTION;ROLLBACK");
    CHECK(called);
    set_mock_sqlite3_rollback_hook([&] (sqlite3 *dbx, void(*handler)(void*), void *data) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(handler == nullptr);
        REQUIRE(data == nullptr);
        return real_sqlite3_rollback_hook(dbx, handler, data);
    });
    db->rollback_hook(nullptr);
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "create collation", "[database]") {
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(name TEXT PRIMARY key)");
    db->exec("INSERT INTO foo(name) VALUES ('abc')");
    
    auto collator = [&](const thinsqlitepp::span<const std::byte> & lhs, const thinsqlitepp::span<const std::byte> & rhs) noexcept -> int {
      
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    };
    
    set_mock_sqlite3_create_collation_v2([&](sqlite3 *dbx, const char * name, int flags,
                                             void * data,
                                             int(*compare)(void*,int,const void*,int,const void*),
                                             void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(data == &collator);
        return real_sqlite3_create_collation_v2(dbx, name, flags, data, compare, destroy);
    });
    
    db->create_collation("haha", SQLITE_UTF8, &collator);
    
    db->exec("SELECT * FROM foo WHERE name COLLATE 'haha' = 'abc'");
    
    struct collator2_t
    {
        int operator()(const thinsqlitepp::span<const std::byte> & lhs, const thinsqlitepp::span<const std::byte> & rhs) const noexcept
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }
    };
    
    std::unique_ptr<collator2_t> collator2(new collator2_t);
    auto ptr = collator2.get();
    set_mock_sqlite3_create_collation_v2([&](sqlite3 *dbx, const char * name, int flags,
                                             void * data,
                                             int(*compare)(void*,int,const void*,int,const void*),
                                             void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(data == ptr);
        return real_sqlite3_create_collation_v2(dbx, name, flags, data, compare, destroy);
    });
    db->create_collation("haha", SQLITE_UTF8, collator2.release(), [] (collator2_t * p) noexcept { delete p; });
    
    db->exec("SELECT * FROM foo WHERE name COLLATE 'haha' = 'abc'");
    
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "create function", "[database]") {
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->exec("DROP TABLE IF EXISTS foo; CREATE TABLE foo(name TEXT PRIMARY key)");
    db->exec("INSERT INTO foo(name) VALUES ('abc')");
    
    auto func = [] (context * ctxt, int, value **) noexcept {
        ctxt->result(17);
    };
    
    set_mock_sqlite3_create_function_v2([&](sqlite3 *dbx, const char * name, int count, int flags,
                                            void * data,
                                            void (*eval)(sqlite3_context*,int,sqlite3_value**),
                                            void (*step)(sqlite3_context*,int,sqlite3_value**),
                                            void (*done)(sqlite3_context*),
                                            void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(name == "haha"s);
        REQUIRE(flags == SQLITE_UTF8);
        REQUIRE(data == &func);
        REQUIRE(func != nullptr);
        REQUIRE(step == nullptr);
        REQUIRE(done == nullptr);
        REQUIRE(destroy == nullptr);
        return real_sqlite3_create_function_v2(dbx, name, count, flags, data, eval, step, done, destroy);
    });
    db->create_function("haha", 1, SQLITE_UTF8, &func, nullptr);
    
    db->exec("SELECT haha(3)");
    
    set_mock_sqlite3_create_function_v2([&](sqlite3 *dbx, const char * name, int count, int flags,
                                            void * data,
                                            void (*func)(sqlite3_context*,int,sqlite3_value**),
                                            void (*step)(sqlite3_context*,int,sqlite3_value**),
                                            void (*done)(sqlite3_context*),
                                            void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(name == "haha"s);
        REQUIRE(flags == SQLITE_UTF8);
        REQUIRE(data == nullptr);
        REQUIRE(func == nullptr);
        REQUIRE(step == nullptr);
        REQUIRE(done == nullptr);
        REQUIRE(destroy == nullptr);
        return real_sqlite3_create_function_v2(dbx, name, count, flags, data, func, step, done, destroy);
    });
    db->create_function("haha", 1, SQLITE_UTF8, nullptr);
    
    struct aggregate
    {
        void step(context *, int, value **) noexcept {
            
            ++_value;
            
        };
        
        void last(context * ctxt) noexcept {
            ctxt->result(17 + _value);
            _value = 0;
        };
        
        void step() {}
        void last() {}
        
        int _value = 0;
        
    } aggr;
    
    set_mock_sqlite3_create_function_v2([&](sqlite3 *dbx, const char * name, int count, int flags,
                                            void * data,
                                            void (*func)(sqlite3_context*,int,sqlite3_value**),
                                            void (*step)(sqlite3_context*,int,sqlite3_value**),
                                            void (*done)(sqlite3_context*),
                                            void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(name == "hoho"s);
        REQUIRE(flags == SQLITE_UTF8);
        REQUIRE(data == &aggr);
        REQUIRE(func == nullptr);
        REQUIRE(step != nullptr);
        REQUIRE(done != nullptr);
        REQUIRE(destroy == nullptr);
        return real_sqlite3_create_function_v2(dbx, name, count, flags, data, func, step, done, destroy);
    });
    db->create_function("hoho", 1, SQLITE_UTF8, &aggr, nullptr);
    
    db->exec("SELECT hoho(name) FROM foo");
    
    set_mock_sqlite3_create_function_v2([&](sqlite3 *dbx, const char * name, int count, int flags,
                                            void * data,
                                            void (*func)(sqlite3_context*,int,sqlite3_value**),
                                            void (*step)(sqlite3_context*,int,sqlite3_value**),
                                            void (*done)(sqlite3_context*),
                                            void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(name == "hoho"s);
        REQUIRE(flags == SQLITE_UTF8);
        REQUIRE(data == nullptr);
        REQUIRE(func == nullptr);
        REQUIRE(step == nullptr);
        REQUIRE(done == nullptr);
        REQUIRE(destroy == nullptr);
        return real_sqlite3_create_function_v2(dbx, name, count, flags, data, func, step, done, destroy);
    });
    db->create_function("hoho", 1, SQLITE_UTF8, nullptr);
    
#if SQLITE_VERSION_NUMBER >= 3025000
    struct window
    {
        void step(context *, int, value **) noexcept {
            ++_value;
        };
        
        void last(context * ctxt) noexcept {
            ctxt->result(17 + _value);
            _value = 0;
        };
        
        void current(context * ctxt) noexcept {
            ctxt->result(17 + _value);
        };
        
        void inverse(context *, int, value **) noexcept {
            ++_value;
        };
        
        int _value = 0;
        
    } wnd;
    
    set_mock_sqlite3_create_window_function([&](sqlite3 *dbx, const char * name, int count, int flags,
                                                void * data,
                                                void (*step)(sqlite3_context*,int,sqlite3_value**),
                                                void (*done)(sqlite3_context*),
                                                void (*value)(sqlite3_context*),
                                                void (*inverse)(sqlite3_context*,int,sqlite3_value**),
                                                void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(name == "hehe"s);
        REQUIRE(flags == SQLITE_UTF8);
        REQUIRE(data == &wnd);
        REQUIRE(step != nullptr);
        REQUIRE(done != nullptr);
        REQUIRE(value != nullptr);
        REQUIRE(inverse != nullptr);
        REQUIRE(destroy == nullptr);
        return real_sqlite3_create_window_function(dbx, name, count, flags, data, step, done, value, inverse, destroy);
    });
    
    db->create_window_function("hehe", 1, SQLITE_UTF8, &wnd, nullptr);
    
    db->exec("SELECT hehe(name) OVER (ORDER BY name ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) FROM foo");
    
    set_mock_sqlite3_create_window_function([&](sqlite3 *dbx, const char * name, int count, int flags,
                                                void * data,
                                                void (*step)(sqlite3_context*,int,sqlite3_value**),
                                                void (*done)(sqlite3_context*),
                                                void (*value)(sqlite3_context*),
                                                void (*inverse)(sqlite3_context*,int,sqlite3_value**),
                                                void(*destroy)(void*)){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(name == "hehe"s);
        REQUIRE(flags == SQLITE_UTF8);
        REQUIRE(data == nullptr);
        REQUIRE(step == nullptr);
        REQUIRE(done == nullptr);
        REQUIRE(value == nullptr);
        REQUIRE(inverse == nullptr);
        REQUIRE(destroy == nullptr);
        return real_sqlite3_create_window_function(dbx, name, count, flags, data, step, done, value, inverse, destroy);
    });
    
    db->create_window_function("hehe", 1, SQLITE_UTF8, nullptr);
#endif
}

#if SQLITE_VERSION_NUMBER >= 3030000

TEST_CASE_METHOD(sqlitepp_test_fixture,  "drop modules", "[database]") {
    
    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    set_mock_sqlite3_drop_modules([&](sqlite3 *dbx, const char ** keepx) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(keepx == nullptr);
        return real_sqlite3_drop_modules(dbx, keepx);
    });
    db->drop_modules();
    set_mock_sqlite3_drop_modules([&](sqlite3 *dbx, const char ** keepx) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(keepx != nullptr);
        REQUIRE(keepx[0] == nullptr);
        return real_sqlite3_drop_modules(dbx, keepx);
    });
    db->drop_modules_except();
    set_mock_sqlite3_drop_modules([&](sqlite3 *dbx, const char ** keepx) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(keepx != nullptr);
        REQUIRE(keepx[0] == "hello"s);
        REQUIRE(keepx[1] == "world"s);
        REQUIRE(keepx[2] == nullptr);
        return real_sqlite3_drop_modules(dbx, keepx);
    });
    db->drop_modules_except("hello", std::string("world"));
    
    const char * keep[] = {"a", "b", "c", nullptr};
    set_mock_sqlite3_drop_modules([&](sqlite3 *dbx, const char ** keepx) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(keepx != nullptr);
        REQUIRE(keepx[0] == "a"s);
        REQUIRE(keepx[1] == "b"s);
        REQUIRE(keepx[2] == "c"s);
        REQUIRE(keepx[3] == nullptr);
        return real_sqlite3_drop_modules(dbx, keepx);
    });
    db->drop_modules_except(keep);
}

#endif

TEST_CASE_METHOD(sqlitepp_test_fixture,  "load extension", "[database]") {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    std::string errx;
    set_mock_sqlite3_load_extension([&](sqlite3 *dbx, const char * file, const char * proc, char ** err) {
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(file == "hghf"s);
        REQUIRE(proc == "zzz"s);
        REQUIRE(err != nullptr);
        auto ret = real_sqlite3_load_extension(dbx, file, proc, err);
        errx = *err;
        return ret;
    });
    try
    {
        db->load_extension("hghf", "zzz");
        REQUIRE(false);
    }
    catch(::thinsqlitepp::exception & ex)
    {
        CHECK(ex.extended_error_code() == SQLITE_ERROR);
        CHECK(ex.what() == errx);
    }
    
}

TEST_CASE_METHOD(sqlitepp_test_fixture,  "progress handler", "[database]") {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    
    
    auto func = [] () noexcept {
        return false;
    };
    
    set_mock_sqlite3_progress_handler([&](sqlite3 *dbx, int step_count, int(*handler)(void*), void*data){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(step_count == 16);
        REQUIRE(handler != nullptr);
        REQUIRE(data == &func);
        return real_sqlite3_progress_handler(dbx, step_count, handler, data);
    });
    
    db->progress_handler(16, &func);
    
    set_mock_sqlite3_progress_handler([&](sqlite3 *dbx, int step_count, int(*handler)(void*), void*data){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(step_count == 16);
        REQUIRE(handler == nullptr);
        REQUIRE(data == nullptr);
        return real_sqlite3_progress_handler(dbx, step_count, handler, data);
    });
    db->progress_handler(16, nullptr);
    
    auto func1 = [] (nullptr_t) noexcept {
        return 0;
    };
    
    set_mock_sqlite3_progress_handler([&](sqlite3 *dbx, int step_count, int(*handler)(void*), void*data){
        
        REQUIRE(dbx == db->c_ptr());
        REQUIRE(step_count == 16);
        REQUIRE(handler != nullptr);
        REQUIRE(data == nullptr);
        return real_sqlite3_progress_handler(dbx, step_count, handler, data);
    });
    db->progress_handler(16, func1, nullptr);
    
}
