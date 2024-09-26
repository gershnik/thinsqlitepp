This is reference documentation for ThinSQLite++ library. For overview, rationale and other links see
[GitHub README](https://github.com/gershnik/thinsqlitepp)

[TOC]

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

### Memory handling

ThinSQLite++ code does not allocate any memory on its own. Any dynamic memory allocation that happens
during its use is done by SQLite and is exactly the same as would have been if using its C API.

Ownership of allocated pointers is indicated by passing `std::unique_ptr<foo>` while non-ownership
by plain `foo *`. The library uses RAII pervasively and there should be no need to manually manage object 
lifetimes while using it.

### Error handling

All errors reported by SQLite as well as any internal issues this library detects are thrown 
as `thinsqlitepp::exception` exceptions. Since ThinSQLite++ does not allocate memory the C++
standard library facilities it uses should generally not throw. However, it is possible that an 
internal bug in this library might case standard library misuse and result in an
`std::exception` being thrown. Any such instances should be considered fatal errors.

Functions that are guaranteed not to throw are marked `noexcept` in this library API. **This includes
any callbacks** for class that accept them. The callbacks are usually invoked by SQLite which is a C library that 
does not expect any calls to throw and will likely get into a corrupt state if they do. This means that
lambda callbacks usually need to be declared like this:
```cpp

foo->func_with_callback([](arguments) noexcept {

});
```

If you want to pass a function that is not declared as `noexcept` but is known not to throw you can
either explicitly cast it or wrap in a lambda like above.

### Thread safety

Classes in this library are generally thread-agnostic. Fake wrappers of SQLite types follow whatever
thread-safety is in effect for the underlying SQLite type. Any utility classes provided by this library 
follow the basic thread-safety guarantee: simultaneous reads (e.g. invocations of `const` member functions)
are allowed from multiple threads; simultaneous writes (e.g. invocations of non-`const` member functions) 
require synchronization.

## More info

Start with [An Introduction To ThinSQLite++](intro.md) page, especially if you are not already familiar with SQLite C interface.

If you are familiar with SQLite C interface a list of mappings from SQLite to ThinSQLite++ can be found [here](mapping.md)

Otherwise, browse the [**list of topics**](topics.html), content of the @ref thinsqlitepp "thinsqlitepp namespace" or the [**list of classes**](annotated.html)

## Bugs and suggestions

Please don't hesitate to report any bugs or suggestions via [Github issues](https://github.com/gershnik/thinsqlitepp/issues).
Bug reports are most welcome and all effort will be made to address them promptly.





