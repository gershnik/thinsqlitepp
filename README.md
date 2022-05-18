#  ThinSQLite++

A thin, safe and convenient modern C++ wrapper for SQLite API.

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![License](https://img.shields.io/badge/license-BSD-brightgreen.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Tests](https://github.com/gershnik/thinsqlitepp/actions/workflows/test.yml/badge.svg)](https://github.com/gershnik/thinsqlitepp/actions/workflows/test.yml)


Using SQLite C API from C++ can be quite tedious and error prone. While the API is generally clean and object-oriented, various aspects of it, notably resource management and error handling can be very tricky to get right. There is also plenty of `void *` and `...` that make it easy to make a mistake as well as other annoyances. The purpose of this library is to provide a C++ wrapper that alleviates all these problems without introducing any overhead.

<!-- TOC depthfrom:2 -->

- [Requirements](#requirements)
- [Goals](#goals)
- [Non Goals](#non-goals)
- [Example](#example)
- [Integration](#integration)
    - [CMake](#cmake)
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

## Goals

1. **0 overhead**. Using the library should introduce no overhead compared to an equivalent and *correct* C API use when comipled with optimizations turned on. In particular it should not introduce any memory allocations where an equivalent *correct* C code wouldn't.
2. **No new concepts** Using the library should not require from the developer to learn new high level concepts compared to plain SQLite.   
3. **Depend only on public API** The library shouldn't depend on any implementation details of SQLite beyond what is documented in the public API or forms necessary logical consequence of it.
4. **RAII for resource management** Using the library should free the developer from manually managing various `close`, `finalize` etc. methods
5. **Error safety** Correct error handling with SQLite C API is notoriously hard. While there is an overall strategy it follows there are many exceptions (pun not intended) and special cases. Getting additional information about errors correctly is also quite tricky. The library should wrap all of this in one simple and coherent approach that does the right thing and frees the developer from dealing with it.
6. **Mix and match** It should be possible to mix usage of C++ and plain C API in any combination.
7. **Wrapper transparency** You should be able to convert freely from C API pointers to C++ wrappers and back. The transaltion should ideally be identity preserving: if you create a C++ wrapper of a C pointer and later observer the C pointer in the code, you should be able to get the original wrapper back from it with 0 overhead.
8. **Type safety** No `void *` or `...` if possible.
9. **Const and noexcept safety** Logically non mutating methods should be `const`. Functions that do not throw - `noexcept` 
10. **Simplify overloaded APIs** Some SQLite APIs are badly overloaded. They have a large number of parameters and do different things depending on which ones are specified and which ones are left out. When possible these should be separated into distinct calls.

## Non Goals

1. Support each and every every SQLite interface. There is no reason to do waste time supporting deprecated interfaces, for example. If these are needed by client code it can always access them via C API
2. Provide higher level abstractions not directly exposed by SQLite, e.g. cursors or typesafe construction of SQL statements.
3. Extend **Serialized** threading mode (see https://sqlite.org/threadsafe.html) to the wrapper. Serialized mode is a design mistake (equivalent to Java's original synchronized collections) and trying to extend it to the wrapper library without violating 0 overhead principle is impossible. The library should work with a database in this mode but not provide Serialized guarantees to its own calls.
4. Support C++ older than C++17
5. Support UTF16 SQLite interfaces. 

## Example

Here is a small example of opening a database that demonstrates many of the features of the library.

<details>
<summary>Code</summary>

```cpp

std::filesystem::path dbFolder = ...;

auto db = database::open((dbFolder / "database.db").string(),
                            SQLITE_OPEN_READWRITE |
                            SQLITE_OPEN_NOMUTEX |
                            SQLITE_OPEN_PRIVATECACHE);


db->config<SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION>(1, nullptr);
db->config<SQLITE_DBCONFIG_ENABLE_FKEY>(1, nullptr);

db->exec("PRAGMA journal_mode=WAL");

auto st = statement::create(*db, "PRAGMA user_version");
if (!st->step())
    throw std::runtime_error("No user_version in database");
auto version = st->column_value<int64_t>(0);

st = statement::create(*db, "SELECT value FROM metadata WHERE key = 'schemaHash'");
if (!st->step())
    throw std::runtime_error("No schema hash in database");
std::string schemaHash(st->column_value<std::string_view>(0));

if (schemaHash != g_dbSchemaHash)
    throw std::runtime_error("Database schema mismatch");

```

</details>

## Integration

### CMake

With modern CMake you can easily integrate ThinSQLite++ as follows:
```cmake
include(FetchContent)
FetchContent_Declare(thinsqlitepp
        GIT_REPOSITORY git@github.com:gershnik/thinsqlitepp.git
        GIT_TAG <desired tag like v1.0>
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(thinsqlitepp)
``` 
  
Alternatively you can clone this repository somewhere and do this:

```cmake
add_subdirectory(PATH_WHERE_YOU_DOWNALODED_IT_TO, thinsqlitepp)
```

In either case you can then use `thinsqlitepp` as a library in your project. 

Whichever method you use in order to use ThinSQLite++ your compiler needs to be set in C++17 mode. 
ThinSQLite++ should compile cleanly even on a highest warnings level. 

## Configuration

This library relies on some of the same configuration macros as SQLite itself to enable some optional functionality. Currently the following macros are used:

* `SQLITE_OMIT_LOAD_EXTENSION`
* `SQLITE_OMIT_PROGRESS_CALLBACK`
* `SQLITE_OMIT_DECLTYPE`

If your SQLite is built using any of these you should define them for any code that uses this library too.

Note that macOS built-in SQLite is built with `SQLITE_OMIT_LOAD_EXTENSION` and the relevant functions aren't even present in its `sqlite3.h` header file. Therefore, you **must** define this macro in order to use this library with it.


## Implementation choices

### Errors as exceptions

All errors are reported via C++ exceptions. There is no attempt to produce a parallel error code based interface (like `std::filesystem` does) or expected/outcome based one. This is a delibrate choice. Duplicating the entire library and maintaining it in such condition is a huge drain on developer resources. Exceptions, with all their issues, are by far simpler and easier way to write code and are perfectly fine for vast majority of users. If you are in one of the environments which cannot use exceptions for some reason - you can use the C API or wait until determinsitic exceptions make their way into C++ standard.

### Fake classes

C++ "wrapper" classes aren't real. Instead each `sqlite3_foo` C object pointer is treated as a pointer to a "fake" C++ class  `sqlitepp::foo`. Member functions cast the `this` pointer back and invoke the corresponding C API. Why do that instead of the usual "class containing a pointer" approach? Becuase it produces less overead and allows idenitity preseving conversions. A trational approach can be done it 2 ways. In the first, wrapper class is a non-copyable, move-only entity (like `unique_ptr`). This requires passing pointers to it arround  - and adds double indirection. It is true that inlinining can often eliminate overhead here but not always. Most problematically, now you cannot go back from C pointer to the C++ object that owns it. This gets ugly in a C callback. In the second approach you end up with 2 classes: non owning wrapper that is passed around by value and an "owner" (like `unique_ptr`) for it. This can be made to work as well as the fake classes approach but at the price of having way more code and introducing two new non standard concepts that library user needs to learn. 

Is fake classes approach legal from C++ standard point of view? I don't know but I wouldn't be suprised if not. Probably casting from pointer to Foo to pointer to an unrelated Bar and back is undefined behavior even if both are simple aggregates. Having said that, there isn't a conceivable platform or compiler where this would fail to work. In fact, many C libraries rely on just that for their workimg. So at the end simple and portable is better than complex and standard conforming (at least from my point of view).

### Header only

Currently the library is supplied as header-only. The purpose of this library is to be a `thin` wrapper so vast majority of it is by design inline. There are only a couple of places where using non-inline implementation would be even mildly benefitial (an `exception::what` virtual function and one or two longer functions) to library user and dealing with all the issues surrounding providing a library seems to be not worth it. Having said that, the library headers are structured in such a way as to allow making a separate compilation possible. If there is a need in the future it should be relatively simple to add this.

### Thread Safety

As mentioned in [Non Goals](#non-goals) there is no attempt to extend Serialized SQLite mode to this library. The problem is that even if each SQLite call itself is protected by a mutex internally calling them ine after another might require additional locking, if you need their results to be consistent. This is most notable when you want to extract Database error code or message after a failed call.
Adding such locking is a pure overhead for sane users who don't use Serialized mode (even checking for `nullptr` mutex has a cost) and brings no benefit to unfortunate people who do use it. Correct C code that uses such mode still needs to lock externally in all the places where this library would have done it.



