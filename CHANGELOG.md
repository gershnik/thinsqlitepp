# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

### Fixed
- CMake install now puts the *.cmake files and .cppm module under `share/thinsqlitepp` rather than `/lib/{.../}thinsqlitepp` as it
  should always have done, since they aren't architecture dependent. This should be completely transparent to clients 
  unless you hardcode the paths into the install location for some reason.

## [1.8] - 2026-06-09

### Added
- **Experimental** support for using this library as a module. See README for more details.
- `statement::carray_bind` overloads wrapping `sqlite3_carray_bind_v2`
- `database::set_clientdata` overloads and `database::get_clientdata` wrapping `sqlite3_set_clientdata`/`sqlite3_get_clientdata`
- `database::status64` wrapping `sqlite3_db_status64`
- `database::setlk_timeout` wrapping `sqlite3_setlk_timeout`
- `database::set_errmsg` wrapping `sqlite3_set_errmsg`
- `keywords` class providing safe, STL-like interface to `sqlite3_keyword_count`, `sqlite3_keyword_name` and `sqlite3_keyword_check`
- `error::offset` wrapping `sqlite3_error_offset`
- `thinsqlitepp::status` wrapping `sqlite3_status` or `sqlite3_status64`
- clang-cl is now supported on Windows

### Fixed
- `statement::bind(int, std::unique_ptr<T>)` now actually compiles
- `value::get<T *>` now actually compiles
- `context::result(std::unique_ptr<T>)` now actually compiles
- `vtab::create_module(..., std::unique_ptr<>)` now actually compiles
- The library headers are now compiling cleanly with `-Wconversion -Wsign-conversion` on clang

## [1.7] - 2026-05-26

### Fixed
- `sqlite_allocator` now actually compiles when used.
- Typo in `THINSQLITEPP_ENABLE_EXPERIMENTAL`. Both old (wrong) and new (correct) spellings are
  now accepted.
- Removed bogus `noexcept` from `database::overload_function`
- `sqlite_version::from_parts` now validates its input correctly
- `statement_parser` now handles correctly input characters above 128. Previously these could 
  be wrongly interpreted as whitespace under certain conditions.
- Harmless UB in `value::dup`
- Correct SQLite version availability for `value::dup`
- `row_iterator` and `row_range` no longer incorrectly pretend to be _forward_ ones. They
  are _input_ iterator and range, respectively.
- `blob::write(Range)` is no longer wrongly const and no longer takes the range by value.
- `vtab::rollback` now properly passes exceptions to SQLite

### Changed
- Various "destructor" SQLite callbacks now consistently require `noexcept`. This might require
  client code to adjust by declaring passed lambdas/function pointer `noexcept` too (and
  make them `noexcept` if they aren't - this would be a pre-existing latent client bug)

## [1.6] - 2025-05-09

### Added
- Support for database and global configuration options added in recent versions of SQLite
### Fixed
- Missing headers on GCC 15
## Changed
- Test CMake targets are no longer included in default build

## [1.5] - 2025-02-12

### Fixed
- CMake install is no longer broken

## [1.4] - 2024-10-04

### Added
- `blob` type to wrap `sqlite3_blob` and related methods
- `backup` type to wrap `sqlite3_backup` and related methods
- `snapshot` type to wrap `sqlite3_snapshot` and related methods
- Wrappers for `xxx_auto_extension` methods
- Wrapper for `sqlite3_db_name`
- Wrappers for `sqlite3_serialize` and `sqlite3_deserialize`
- Wrapper for `sqlite3_update_hook`
- Wrappers for `sqlite3_preupdate_xxx` API
- Wrappers for `sqlite3_wal_xxx` API
- `sqlite_version` type to wrap versioning calls and macros

### Changed
- `database::exec` now supports multiple simplified callback variants.

### Fixed
- Bogus warnings with GCC in C++17 mode
- Incorrect exception raised in some cases where an SQLite API returns an error but no error is set on database
  connection.

## [1.3] - 2024-09-27

### Added
- Wrappers for all SQLite virtual table-related interfaces and base class for virtual table implementations.
  See [Implementing Virtual Tables](https://gershnik.github.io/thinsqlitepp/vtab-guide.html) for more details
- `statement::bind_reference` overloads that accept callback to be invoked on dereference
- `value::get` overload wrapping `sqlite3_value_pointer`
- `sqlite_allocated` empty base class that makes derived classes use operators new/delete based on SQLite
  memory APIs
- `sqlite_allocator<T>` allocator that uses SQLite memory APIs
- `THINSQLITEPP_BUILDING_EXTENSION` configuration macro to slightly simplify usage of this library in an
  extension

### Fixed
- `exception::error` is now `const`
- Passing oversized data to the API now properly reports SQLITE_TOOBIG exception.
- `database::load_extension` now supports nullptr entry point name

## [1.2] - 2024-09-10

### Added
- Documentation for public API
- `row_range` range class to simplify use of `row_iterator`
- Compile type requirements for `database::exec` callback
- Overloads of `context::result_reference` that accept destructor function callback

### Changed
- Defining SQLite configuration macros `SQLITE_OMIT_LOAD_EXTENSION`, `SQLITE_OMIT_PROGRESS_CALLBACK` and `SQLITE_OMIT_DECLTYPE`
to compile this library is no longer needed.
- Parameters to `database::exec` overloads are now uniformly `std::basic_string_view`. This is a backward compatible change

### Fixed
- `row_iterator` and `row` now properly implement range concepts
- Made handle::c_ptr(T *) an inline friend as it should have been 
- Return type of the collator for `database::create_collation`

## [1.1] - 2023-07-30

### Added
- Ability to install library locally

### Changed
- Modernized and cleaned up CMake configuration

### Fixed
- Warnings on various versions of gcc and clang

## [1.0] - 2022-05-18

### Added
- Initial version

[1.0]: https://github.com/gershnik/thinsqlitepp/releases/v1.0
[1.1]: https://github.com/gershnik/thinsqlitepp/releases/v1.1
[1.2]: https://github.com/gershnik/thinsqlitepp/releases/v1.2
[1.3]: https://github.com/gershnik/thinsqlitepp/releases/v1.3
[1.4]: https://github.com/gershnik/thinsqlitepp/releases/v1.4
[1.5]: https://github.com/gershnik/thinsqlitepp/releases/v1.5
[1.6]: https://github.com/gershnik/thinsqlitepp/releases/v1.6
[1.7]: https://github.com/gershnik/thinsqlitepp/releases/v1.7
[1.8]: https://github.com/gershnik/thinsqlitepp/releases/v1.8
