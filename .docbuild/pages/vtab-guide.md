# Implementing Virtual Tables  {#vtab-guide}

> [!note]
> If you are not familiar with what SQLite virtual tables are and how they work 
> the best place to start is [The Virtual Table Mechanism Of SQLite](https://www.sqlite.org/vtab.html)
> page. 

[TOC]

## Introduction

There are two ways you can implement a virtual table using ThinSQLite++:
- Manually, where you directly implement @ref sqlite3_module calls using ThinSQLite++ for the implementation.
All the necessary supporting calls such as @refmylib{database::create_module}, @refmylib{database::declare_vtab},
@refmylib{database::vtab_config}, etc. are provided by the library.
- Using the provided @refmylib{vtab} base class.

There is nothing much to add about the manual approach. Simply follow [The Virtual Table Mechanism Of SQLite](https://www.sqlite.org/vtab.html)
page, implement the necessary calls, taking care of properly handling any errors (note that you cannot emit C++ exceptions from SQLite callbacks),
memory ownership and typing.

This page describes the seconds approach.

## Basics

In order to implement a virtual table the bare minimum you need to do is:

- Create a class derived from @refmylib{vtab} template
- Provide one or more required constructors

Here is an example of a minimal virtual table class that doesn't do anything

```cpp
#include <thinsqlitepp/vtab.hpp>

using namespace thinsqlitepp;

class my_table : public vtab<my_table> {
public:
    my_table(database * db, int argc, const char * const * argv) {
        // tell SQLite what does the virtual table look like in SQL
        db->declare_vtab("CREATE TABLE this_name_is_ignored (a_column TEXT)");
    }
};
```

And here is how to load and use it:

```cpp

auto db = database::open(...);
my_table::create_module(*db, "my_table_module");

db->exec("SELECT a_column FROM my_table_module", [] (int, row r) noexcept {
    std::cout << r[0].value<std::string_view>();
    return true;
});
```

If you run this code nothing should be printed out since your table doesn't yet provide any data.

## Constructors

What kind of constructors you give your virtual table class determines what kind of virtual table
it produces. There are 3 possible constructors@footnote{1}

- **Common** constructor. It has the following form
  ```cpp
  my_table::my_table(database * db, [optional construction data pointer,] int argc, const char * const * argv)
  ```
  If specified this must be the **only** recognized constructor - you cannot also have *create* or *connect* constructors
  together with it.
- **Create** constructor. It has the following form
  ```cpp
  my_table::my_table(create_t, database * db, [optional construction data pointer,] int argc, const char * const * argv)
  ```
  You cannot have it together with *common* constructor. 
- **Connect** constructor. It has the following form
  ```cpp
  my_table::my_table(connect_t, database * db, [optional construction data pointer,] int argc, const char * const * argv)
  ```
  You must have either this constructor or *common* constructor but not both.

@refmylib{vtab::create_t} and @refmylib{vtab::connect_t} are dummy marker types declared in @refmylib{vtab} that are used
to differentiate between the create and connect constructors.

The behavior expectations of these constructors is the same as from SQLite @ref xCreate and @ref xConnect functions.
A common constructor implements the same behavior for both.

The type of virtual table you create is determined from the constructors you create as follows

| Constructors            |  Virtual Table Type | SQLite equivalent
|-------------------------|---------------------|-------------------
| Both Create and Connect | Regular             | Separate @ref xCreate and @ref xConnect
| Common                  | [Eponymous](https://www.sqlite.org/vtab.html#epovtab)  | Same @ref xCreate and @ref xConnect
| Only Connect            | [Eponymous only](https://www.sqlite.org/vtab.html#eponymous_only_virtual_tables) | Null @ref xCreate

### Construction data

Your constructors can accept optional _construction data_ passed from @refmylib{database::create_module}. By default it is disabled.
In order to enable it you need to declare `constructor_data_type` type in your class as a pointer to whatever data you want to pass.
For example:

```cpp

struct my_data {...};

class my_table : public vtab<my_table> {
public:
    using constructor_data_type = my_data *;
public:
    my_table(database * db, my_data * data, int argc, const char * const * argv) {
        ...
    }
};

my_data data;
my_table::create_module(*db, "my_table_module", &data);

```

This will pass the pointer to `data` from the calling code your constructor. Refer to @refmylib{vtab::create_module}
for more details on how to pass data and control its lifetime.

## Implementing cursor

In order to actually expose your virtual table data you need to provide a nested `cursor` class in your virtual 
table implementation.

@anchor fn_1 1: You can certainly add any other constructors to your class but these won't be recognized or used


