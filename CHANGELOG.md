# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

### Added
- `blob` type to wrap `sqlite3_blob` and related methods
- `backup` type to wrap `sqlite3_backup` and related methods
- `snapshot` type to wrap `sqlite3_snapshot` and related methods
- Wrappers for `xxx_auto_extension` methods
- Wrapper for `sqlite3_db_name`
- Wrappers for `sqlite3_serialize` and `sqlite3_deserialize`
- Wrapper for `sqlite3_update_hook`
- Wrappers for `sqlite3_preupdate_xxx` API

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
