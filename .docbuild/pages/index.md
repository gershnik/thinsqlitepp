This is reference documentation for ThinSLite++ library. For overview, rationale and other links see
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



