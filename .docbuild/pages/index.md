This is reference documentation for ThinSQLite++ library. For overview, rationale and other links see
[GitHub README](https://github.com/gershnik/thinsqlitepp)

## Basics

The library is header only. In order to use it you need to either include an umbrella header:
```cpp
#include <thinsqlitepp/thinsqlitepp.hpp>
```

or individual headers for the facilities you need. The necessary headers are listed in the reference documentation
of each class and global entities.

All the code in the library is under namespace @ref thinsqlitepp. A directive:
```cpp
using namespace thinsqlitepp;
```
is assumed for all code samples in this documentation.

This library depends only on C++ standard library and SQLite. A header named `sqlite3.h` must be present
in include path for it to compile.

## Memory handling

ThinSQLite++ code does not allocate any memory on its own. Any dynamic memory allocation that happens
during its use is done by SQLite and is exactly the same as would have been if using its C API.

Ownership of allocated pointers is indicated by passing `std::unique_ptr<foo>` while non-ownership
by plain `foo *`. The library uses RAII pervasively and there should be no need to manually manage object 
lifetimes while using it.

## Error handling

All errors reported by SQLite as well as any internal issues this library detects are thrown 
as `thinsqlitepp::exception` exceptions. Since ThinSQLite++ does not allocate memory the C++
standard library facilities it uses should generally not throw. However, it is possible that an 
internal bug in this library might case standard library misuse and result in an
`std::exception` being thrown. Any such instances should be considered fatal errors.

Functions that are guaranteed not to throw are marked `noexcept` in this library API. This includes
any callbacks you provide. The callbacks are usually invoked by SQLite which is a C library that 
does not expect any calls to throw and will likely get into a corrupt state if they do.

## Thread safety

