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

This page describes the second approach.

## Basics

In order to implement a virtual table the bare minimum you need to do is:

- Create a class derived from @refmylib{vtab} template
- Provide one or more required constructors
- Declare a nested cursor class that derives from @refmylib{vtab::cursor} and
  inherits its constructors

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

    class cursor : public vtab::cursor {
    public:
        //inherit base constructor
        using vtab::cursor::cursor;
    };
};
```

And here is how to load and use it:

```cpp

auto db = database::open(...);
my_table::create_module(*db, "my_table_module");

db->exec("SELECT a_column FROM my_table_module", [] (int, row r) {
    std::cout << r[0].value<std::string_view>() << '\n';
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

## Destruction

Normally you can rely on your virtual table class destructor to perform any necessary cleanup. However, if you have 
a need for custom _create_ and/or _connect_ constructors you might want to differentiate between actions taken when
the table is destroyed vs. a simple disconnect. To do so you can define one or both of the following functions:

```cpp
static void destroy(std::unique_ptr<your_class> obj) noexcept;
static void disconnect(std::unique_ptr<your_class> obj) noexcept;
```

These correspond to SQLite @ref xDestroy and @ref xDisconnect methods.

Note that both functions are static and they a given a unique pointer to your class instance. When they are invoked
SQLite is done with your class instance and you now have the ownership of the object. Thus the instance will be 
destroyed@footnote{2} once these methods complete.


## Implementing cursor

In order to actually expose your virtual table data you need to re-define the following base class methods
in your `cursor` class. 

* **filter()**
  ```cpp
  void filter(int index_num, [optional index_data_type index_data,] int argc, value ** argv)
  ```
  The optional `data` argument is described [later](#filtering) on this page.
  It "resets" the cursor to start a new iteration and it is always
  called even for a single iteration. Do not make the mistake of only initializing cursor iteration
  sequence in the constructor. It needs to be re-initialized every time `filter` is called. This
  method corresponds to @ref xFilter SQLite call. The base class implementation: 
  @refmylib{vtab::cursor::filter} does essentially nothing.
* **eof() and next()**
  ```cpp
  bool eof() const noexcept;
  void next();
  ```
  These correspond to @ref xEof and @ref xNext SQLite methods and perform the actual iteration.
  Note that `eof()` must be declared `noexcept`. The base class implementation of @refmylib{vtab::cursor::eof} 
  always returns `true` and @refmylib{vtab::cursor::next} throws an exception.
* **column() and rowid()**
  ```cpp
  void column(context & ctxt, int idx) const;
  int64_t rowid() const;
  ```
  These retrieve values for the table row currently pointed to by the cursor. They are never called 
  once `eof()` returns `false`. They correspond to `xColumn` and `xRowid` SQLite calls. The base
  implementations of @refmylib{vtab::cursor::column} and @refmylib{vtab::cursor::rowid} throw exceptions.
* Optionally define your own constructor if you have any one-time initialization to perform. 
  ```cpp
  cursor(your_table_type * owner): vtab::cursor(owner) 
  { ... }
  ```
  Note that this is rarely necessary since inside your cursor implementation you can always access your 
  enclosing virtual table class instance using @refmylib{vtab::cursor::owner} call to get any data the
  owning table might contain. 

To illustrate how these methods can be used, here is a very minimalistic and incomplete virtual table
that exposes a vector of integers as an SQLite eponymous table

```cpp

class my_table : public vtab<my_table> {
public:
    using constructor_data_type = std::vector<int> *;

    class cursor : public vtab::cursor {
    public:
        //inherit base constructor
        using vtab::cursor::cursor;

        void filter(int /*index_num*/, int /*argc*/, value ** /*argv*/) {
            _begin = owner()->_vec->data();
            _end = _begin + owner()->_vec->size();
            _current = _begin;
        }

        bool eof() const noexcept
            { return _current == _end; }
        
        void next()
            { ++_current; }

        int64_t rowid() const
            { return _current - _begin; }

        void column(context & ctxt, int idx) const {
            ctxt.result(*_current);
        }

    private:
        const int * _begin = nullptr;
        const int * _current = nullptr;
        const int * _end = nullptr;
    };
public:
    my_table(connect_t, database * db, std::vector<int> * vec, int argc, const char * const * argv):
        _vec(vec) {
        db->declare_vtab("CREATE TABLE this_name_is_ignored (value INTEGER)");
    }

private:
    std::vector<int> * _vec;
};

```

Which can be used as follows:

```cpp

std::vector vec{74, 42, 50};
my_table::create_module(*db, "my_table_module", &vec);

db->exec("SELECT value FROM my_table_module", [] (int, row r) {
    std::cout << r[0].value<int>() << '\n';
    return true;
});

```

This should print:
```
74
42
50
```

### Filtering

The full purpose of the `filter()` call is to attempt to filter the returned data as appropriate for the
query `WHERE` (and possibly `ORDER BY`) clauses _in your code_ rather than have SQLite obtain all the values 
and do the filtering itself. The idea is that for some queries, at least, you can do it much more efficiently.

For example in the vector table above the matching of `WHERE value = 42` can be done much more efficiently
in our implementation.

To perform your own filtering you need to:

- re-define **best_index()** in your table class (note that this is a _table's_ method and not cursor's)
  ```cpp
  bool best_index(index_info<index_data_type> & info)	const
  ```

- use the data generated by `best_index()` in your `cursor::filter()` call


`best_index()` is equivalent in functionality to @ref xBestIndex SQLite call. The base implementation in 
@refmylib{vtab::best_index} does essentially nothing.

The @refmylib{index_info} class wraps @ref sqlite3_index_info and has the same functionality. It is an 
input-output parameter used to convey information to `best_index` about what query conditions are in effect 
and return information back to SQLite for query planner.

The `index_data_type` typedef determines what kind of custom data can be stored in @refmylib{index_info}.
(This data is mapped to poorly typed `idxStr` member of @ref sqlite3_index_info). The data is conveyed unchanged
to your `cursor::filter()` call.

If `index_data_type` is `void` (which is the default in @refmylib{vtab::index_data_type}) the setters 
@refmylib{index_info::set_index_data} are disabled and the `cursor::filter()` call you need to implement looks like this:

```cpp
void filter(int idx, int argc, value ** argv)
```

In your table class you can redefine `index_data_type` to be a _pointer_ to some class. If you do that, 
you will be able to use @refmylib{index_info::set_index_data} methods and your `cursor::filter()` call will 
have to look like this:

```cpp
void filter(int idx, index_data_type data, int argc, value ** argv)
```

Refer to @ref xBestIndex for full details on the semantics of various fields of @refmylib{index_info} and
how to manipulate indices.

Here is a simple example that exposes a `std::map` as a virtual table and provides custom indexing
on the map's key column:

```cpp
class my_table : public vtab<my_table> {
public:
    using map_type = std::map<int, std::string>;
    using map_range = std::pair<map_type::iterator, map_type::iterator>;
    using map_indexer = map_range (map_type &, map_type::iterator, map_type::iterator, map_type::key_type);

    //variable length data to pass between best_index() and filter()
    struct comparisons_array : public sqlite_allocated {
        size_t count = 0;
        map_indexer * indexer[1];
    };
    
    //allocate variable length index data
    static comparisons_array * allocate_index_data(size_t entry_count) {
        size_t alloc_size = offsetof(comparisons_array, indexer) + sizeof(comparisons_array) * entry_count;
        auto ret = (comparisons_array *)comparisons_array::operator new(alloc_size);
        ret->count = 0;
        return ret;
    }
    
    //convert SQLite constraint operation to map range lookup
    static map_indexer * find_indexer(int op) {
        struct comparator {
            bool operator()(const map_type::value_type & lhs, map_type::key_type rhs) const
                { return comp(lhs.first, rhs); }
            bool operator()(map_type::key_type lhs, const map_type::value_type & rhs) const
                { return comp(lhs, rhs.first); }
            map_type::key_compare comp;
        };
        switch (op) {
            case SQLITE_INDEX_CONSTRAINT_EQ:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return std::equal_range(begin, end, val, comparator{map.key_comp()});
                };
            case SQLITE_INDEX_CONSTRAINT_GT:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{std::upper_bound(begin, end, val, comparator{map.key_comp()}), end};
                };
            case SQLITE_INDEX_CONSTRAINT_LE:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{begin, std::upper_bound(begin, end, val, comparator{map.key_comp()})};
                };
            case SQLITE_INDEX_CONSTRAINT_LT:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{begin, std::lower_bound(begin, end, val, comparator{map.key_comp()})};
                };
            case SQLITE_INDEX_CONSTRAINT_GE:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{std::lower_bound(begin, end, val, comparator{map.key_comp()}), end};
                };
        }
        return nullptr;
    }

public:
    
    using constructor_data_type = map_type *;
    using index_data_type =  comparisons_array *;
    
    my_table(connect_t, database * db, map_type * map, int argc, const char * const * argv):
        _map(map) {
        db->declare_vtab(R"_(
         CREATE TABLE this_name_is_ignored (
                         key INTEGER PRIMARY KEY,
                         value TEXT)
        )_");
    }

    bool best_index(index_info<index_data_type> & info) const {
        auto constraints = info.constraints();
        auto usages = info.constraints_usage();
        auto orderbys = info.orderbys();

        //first let's check if already provide the required order, if any
        bool properly_ordered = true;
        for (auto order: orderbys) {
            if (order.iColumn != 0 && order.iColumn != -1)
                properly_ordered = false;
            else if (order.desc)
                properly_ordered = false;
        }
        info.set_order_by_consumed(properly_ordered);


        //next check if we can handle the constraints        
        if (!constraints.empty()) {
            std::unique_ptr<comparisons_array> comparisons{allocate_index_data(constraints.size())};
            for (size_t i = 0; i < constraints.size(); ++i) {
                auto & constraint = constraints[i];
                auto & usage = usages[i];

                //skip unused constraints or constraints not on key/rowid column
                if (!constraint.usable || (constraint.iColumn != 0 && constraint.iColumn != -1))
                    continue;
            
                if (auto indexer = find_indexer(constraint.op)) {
                    //if we can handle the operation using map lookup add the
                    //indexer to the data
                    comparisons->indexer[comparisons->count] = indexer;
                    //set argvIndex so the value to compare to is given to filter()
                    usage.argvIndex = int(++comparisons->count);
                    //and tell SQLite not to check this condition itself
                    usage.omit = true;
                }
            }
            if (comparisons->count) {
                //we don't use index_number but still let's set it to non-default value
                info.set_index_number(1);
                //move our collected indexers data into info
                info.set_index_data(std::move(comparisons));
                //set the cost to 0 - we can handle it best
                info.set_estimated_cost(0);
                return true;
            } 
        }
        
        //if we have no indexers to use set a high but not infinite cost and let SQLite do the filtering
        info.set_estimated_cost((double)2147483647);
        return true;
    }

    class cursor : public vtab::cursor {
    public:
        //inherit base constructor
        using vtab::cursor::cursor;

        void filter(int index_num, comparisons_array * index_data, int argc, value ** argv) {
            
            auto & map = *(owner()->_map);

            //initialize our range to the full map
            _current = map.begin();
            _end = map.end();

            //if we have indexers available, restrict the range
            //to satisfy all of them
            if (index_data) {
                for (size_t i = 0; i < index_data->count; ++i) {
                    auto val = argv[i]->get<map_type::key_type>();
                    std::tie(_current, _end) = index_data->indexer[i](map, _current, _end, val);
                }
            }
        }

        bool eof() const noexcept
            { return _current == _end; }
        
        void next() 
            { ++_current; }

        int64_t rowid() const
            { return int64_t(_current->first); }

        void column(context & ctxt, int idx) const {
            if (idx == 0)
                ctxt.result(_current->first);
            else
                ctxt.result(_current->second);
        }

    private:
        map_type::iterator _current;
        map_type::iterator _end;
    };

private:
    map_type * _map;
};

```

Which can be used as follows:

```cpp
std::map<int, std::string> map{
    {74, "a"},
    {42, "b"},
    {50, "c"},
    {80, "d"}
};
my_table::create_module(*db, "my_table_module", &map);
db->exec("SELECT key, value FROM my_table_module WHERE key > 70 AND key < 80", [] (int, row r) {
    std::cout << r[0].value<int>() << ": " << r[1].value<std::string_view>() << '\n';
    return true;
});
```

This should print:
```
74: a
```

## Other optional methods

Beyond the basic method described above you can also define many additional optional methods in your
virtual table class. Their signatures and SQLite equivalents are given below. Refer to SQLite functions'
documentation for further details.


| Method                                                                    |  SQLite Equivalent |
|---------------------------------------------------------------------------|--------------------|
| @code{.cpp} int64_t update(int argc, value ** argv) @endcode              | @ref xUpdate
| @code{.cpp} int find_function(int n_arg, const char * name, void (**func)(context*,int,value**) noexcept, void ** user_data) const noexcept @endcode | @ref xFindFunction
| @code{.cpp} void begin() @endcode                                         | @ref xBegin
| @code{.cpp} void sync() @endcode                                          | @ref xSync
| @code{.cpp} void commit() @endcode                                        | @ref xCommit
| @code{.cpp} void rollback() @endcode                                      | @ref xRollback
| @code{.cpp} void rename(const char * name) @endcode                       | @ref xRename
| @code{.cpp} void savepoint(int n) @endcode                                | @ref xSavepoint
| @code{.cpp} void release(int n) @endcode                                  | @ref xRelease
| @code{.cpp} void rollback_to(int n) @endcode                              | @ref xRollbackTo
| @code{.cpp} static bool shadow_name(const char * name) noexcept @endcode  | @ref xShadowName (since SQLite 3.26)
| @code{.cpp} allocated_string integrity(const char * schema, const char * table, int flags) @endcode | @ref xIntegrity (since SQLite 3.44)

## Exceptions

Unless explicitly required to be `noexcept` in the documentation all the virtual table and cursor functions you define 
can and should throw exceptions if they cannot fulfill their contract. The exceptions will be handled, converted to
an error return from SQLite callback and properly set `zErrMsg` field. You can throw thinsqlitepp::exception to report
a specific desired SQLite error code or anything derived from std::exception. In the latter case the return code will 
be @ref SQLITE_ERROR and the error message whatever your exception's @ref std::exception::what() "what()" method returns.

Below is the list of all functions that must be `noexcept`

* **destroy**
* **disconnect**
* **cursor::eof**
* **find_function**
* **shadow_name**

## Footnotes

@footnotedef{1} You can certainly add any other constructors to your class but these won't be recognized or used<br>
@footnotedef{2} Unless of course if, for some reason, you decide to stash the unique pointer somewhere<br>

