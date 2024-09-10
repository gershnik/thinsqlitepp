#  ThinSQLite++

A thin, safe and convenient modern C++ wrapper for SQLite API.

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![License](https://img.shields.io/badge/license-BSD-brightgreen.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Tests](https://github.com/gershnik/thinsqlitepp/actions/workflows/test.yml/badge.svg)](https://github.com/gershnik/thinsqlitepp/actions/workflows/test.yml)


Using SQLite C API from C++ can be quite tedious and error prone. While the API is generally clean and object-oriented, various aspects of it, notably resource management and error handling can be very tricky to get right. There is also plenty of `void *` and `...` that make it easy to make a mistake as well as other annoyances. The purpose of this library is to provide a C++ wrapper that alleviates all these problems without introducing any overhead.

<!-- TOC depthfrom:2 -->

- [Requirements](#requirements)
- [Documentation](#documentation)
- [Goals](#goals)
- [Non Goals](#non-goals)
- [Example](#example)
- [Integration](#integration)
    - [CMake](#cmake)
        - [CMake via FetchContent](#cmake-via-fetchcontent)
        - [CMake from downloaded sources](#cmake-from-downloaded-sources)
    - [Installing on your system](#installing-on-your-system)
        - [Basic use](#basic-use)
        - [CMake package](#cmake-package)
        - [Via pkg-config](#via-pkg-config)
    - [Use directly](#use-directly)
- [Configuration](#configuration)
- [Implementation choices](#implementation-choices)
    - [Errors as exceptions](#errors-as-exceptions)
    - [Fake classes](#fake-classes)
    - [Header only](#header-only)
    - [Thread Safety](#thread-safety)

<!-- /TOC -->

## Requirements

* C++17 or greater
* SQLite 3.7.15 or greater

## Documentation

Full reference documentation is available at https://gershnik.github.io/thinsqlitepp/

## Goals

1. **0 overhead**. Using the library should introduce no overhead compared to an equivalent and *correct* C API use when compiled with optimizations turned on. In particular it should not introduce any memory allocations where an equivalent *correct* C code wouldn't.
2. **No new concepts** Using the library should not require from the developer to learn new high level concepts compared to plain SQLite.   
3. **Depend only on public API** The library shouldn't depend on any implementation details of SQLite beyond what is documented in the public API or forms necessary logical consequence of it.
4. **RAII for resource management** Using the library should free the developer from manually managing various `close`, `finalize` etc. methods
5. **Error safety** Correct error handling with SQLite C API is notoriously hard. While there is an overall strategy it follows there are many exceptions (pun not intended) and special cases. Getting additional information about errors correctly is also quite tricky. The library should wrap all of this in one simple and coherent approach that does the right thing and frees the developer from dealing with it.
6. **Mix and match** It should be possible to mix usage of C++ and plain C API in any combination.
7. **Wrapper transparency** You should be able to convert freely from C API pointers to C++ wrappers and back. The translation should ideally be identity preserving: if you create a C++ wrapper of a C pointer and later observer the C pointer in the code, you should be able to get the original wrapper back from it with 0 overhead.
8. **Type safety** No `void *` or `...` if possible.
9. **Const and noexcept safety** Logically non mutating methods should be `const`. Functions that do not throw - `noexcept` 
10. **Simplify overloaded APIs** Some SQLite APIs are badly overloaded. They have a large number of parameters and do different things depending on which ones are specified and which ones are left out. When possible these should be separated into distinct calls.

## Non Goals

1. Support each and every every SQLite interface. There is no reason to do waste time supporting deprecated interfaces, for example. If these are needed by client code it can always access them via C API
2. Provide higher level abstractions not directly exposed by SQLite, e.g. cursors or type safe construction of SQL statements.
3. Extend **Serialized** threading mode (see https://sqlite.org/threadsafe.html) to the wrapper. Serialized mode is a design mistake (equivalent to Java's original synchronized collections) and trying to extend it to the wrapper library without violating 0 overhead principle is impossible. The library should work with a database in this mode but not provide Serialized guarantees to its own calls.
4. Support C++ older than C++17
5. Support UTF16 SQLite interfaces. 

## Example

Here is a small example that demonstrates many of the features of the library.

```cpp

try {

    auto db = database::open("database.db",
                                SQLITE_OPEN_READWRITE |
                                SQLITE_OPEN_NOMUTEX |
                                SQLITE_OPEN_PRIVATECACHE);


    db->config<SQLITE_DBCONFIG_ENABLE_FKEY>(1, nullptr);

    db->exec("PRAGMA journal_mode=WAL");

    auto st = statement::create(*db, "SELECT name, age FROM mytable");
    while (st->step()) {
        auto name = st->column_value<std::string_view>(0);
        auto age = st->column_value<int>(1);
    }

    st->reset();

    for (auto r: row_range(st)) {
        auto name = r[0].value<std::string_view>();
        auto age = r[0].value<int>();
    }

    db->exec("SELECT name, age FROM mytable", [](int statement_idx, row r) noexcept {
        auto name = r[0].value<std::string_view>();
        auto age = r[0].value<int>();
        return true;
    });

    auto func = [] (context * ctxt, int arg_count, value ** args) noexcept {
        ctxt->result(42);
    };

    db->create_function("myfunction", 0, SQLITE_UTF8, &func, nullptr);

    db->exec("SELECT myfunction();", [](int statement_idx, row r) noexcept {
        assert(r[0].value<int>() == 42);
        return true;
    });
    
} catch (thinsqlitepp::exception & ex) {
    int err = ex.primary_error_code();
    int ext = ex.extended_error_code();
    int sys = ex.system_error_code();
    std::cout << ex.what() << '\n';
}

```

## Integration

### CMake

#### CMake via FetchContent

With modern CMake you can easily integrate ThinSQLite++ as follows:
```cmake
include(FetchContent)
FetchContent_Declare(thinsqlitepp
    GIT_REPOSITORY git@github.com:gershnik/thinsqlitepp.git
    GIT_TAG <desired tag like v1.0>
    GIT_SHALLOW TRUE
)
...
FetchContent_MakeAvailable(thinsqlitepp)
...
target_link_libraries(mytarget
PRIVATE
  thinsqlitepp::thinsqlitepp
)
``` 

#### CMake from downloaded sources

Alternatively you can download the library from [Releases](https://github.com/gershnik/thinsqlitepp/releases) 
page, unpack it somewhere and do this

```cmake
add_subdirectory(PATH_WHERE_YOU_UNPACKED_IT_TO, thinsqlitepp)
...
target_link_libraries(mytarget
PRIVATE
  thinsqlitepp::thinsqlitepp
)
```

### Installing on your system

You can also build and install this library on your system using CMake.

1. download the library from [Releases](https://github.com/gershnik/thinsqlitepp/releases) 
page, unpack it into SOME_PATH
2. On command line:
```bash
cd SOME_PATH
cmake -S . -B build 
cmake --build build

#install to /usr/local
sudo cmake --install build
#or for a different prefix
#cmake --install build --prefix /usr
```

Once the library has been installed it can be used int the following ways:

#### Basic use 

Set the include directory to `<prefix>/include` where `<prefix>` is the install prefix from above.

#### CMake package

```cmake
find_package(thinsqlitepp)

target_link_libraries(mytarget
PRIVATE
  thinsqlitepp::thinsqlitepp
)
```

#### Via `pkg-config`

Add the output of `pkg-config --cflags thinsqlitepp` to your compiler flags.

Note that the default installation prefix `/usr/local` might not be in the list of places your
`pkg-config` looks into. If so you might need to do:
```bash
export PKG_CONFIG_PATH=/usr/local/share/pkgconfig
```
before running `pkg-config`

### Use directly

You can also simply download the headers of this repository from [Releases](https://github.com/gershnik/thinsqlitepp/releases) page 
(named `thinsqlitepp-X.Y.tar.gz`), unpack it somewhere and add its `inc` to your include path.


## Configuration

Whichever method you use in order to use ThinSQLite++ your compiler needs to be set to C++17 mode or higher. 
ThinSQLite++ should compile cleanly even on a highest warnings level. 


## Implementation choices

### Errors as exceptions

All errors are reported via C++ exceptions. There is no attempt to produce a parallel error code based interface (like `std::filesystem` does) or expected/outcome based one. This is a deliberate choice. Duplicating the entire library and maintaining it in such condition is a huge drain on developer resources. Exceptions, with all their issues, are by far simpler and easier way to write code and are perfectly fine for vast majority of users. If you are in one of the environments which cannot use exceptions for some reason - you can use the C API or wait until deterministic exceptions make their way into C++ standard.

### Fake classes

C++ "wrapper" classes aren't real. Instead each `sqlite3_foo` C object pointer is treated as a pointer to a "fake" C++ class  `sqlitepp::foo`. Member functions cast the `this` pointer back and invoke the corresponding C API. Why do that instead of the usual "class containing a pointer" approach? Because it produces less overhead and allows identity preserving conversions. A traditional approach can be done it 2 ways. In the first, wrapper class is a non-copyable, move-only entity (like `unique_ptr`). This requires passing pointers to it around  - and adds double indirection. It is true that inlining can often eliminate overhead here but not always. Most problematically, now you cannot go back from C pointer to the C++ object that owns it. This gets ugly in a C callback. In the second approach you end up with 2 classes: non owning wrapper that is passed around by value and an "owner" (like `unique_ptr`) for it. This can be made to work as well as the fake classes approach but at the price of having way more code and introducing two new non standard concepts that library user needs to learn. 

Is fake classes approach legal from C++ standard point of view? I don't know but I wouldn't be surprised if not. Probably casting from pointer to Foo to pointer to an unrelated Bar and back is undefined behavior even if both are simple aggregates. Having said that, there isn't a conceivable platform or compiler where this would fail to work. In fact, many C libraries rely on just that for their working. So at the end simple and portable is better than complex and standard conforming (at least from my point of view).

### Header only

Currently the library is supplied as header-only. The purpose of this library is to be a `thin` wrapper so vast majority of it is by design inline. There are only a couple of places where using non-inline implementation would be even mildly beneficial (an `exception::what` virtual function and one or two longer functions) to library user and dealing with all the issues surrounding providing a library seems to be not worth it. Having said that, the library headers are structured in such a way as to allow making a separate compilation possible. If there is a need in the future it should be relatively simple to add this.

### Thread Safety

As mentioned in [Non Goals](#non-goals) there is no attempt to extend Serialized SQLite mode to this library. The problem is that even if each SQLite call itself is protected by a mutex internally calling them one after another might require additional locking, if you need their results to be consistent. This is most notable when you want to extract Database error code or message after a failed call.
Adding such locking is a pure overhead for sane users who don't use Serialized mode (even checking for `nullptr` mutex has a cost) and brings no benefit to unfortunate people who do use it. Correct C code that uses such mode still needs to lock externally in all the places where this library would have done it.



