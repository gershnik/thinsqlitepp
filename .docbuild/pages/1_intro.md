# An Introduction To ThinSQLite++ {#intro}

@note
This page closely follows [An Introduction To The SQLite C/C++ Interface](https://www.sqlite.org/cintro.html). Since ThinSQLite++ is a thin and exact wrapper of SQLite's C API all the conceptual parts are exactly the same. The difference is in safety and convenience.

[TOC]

## Summary

The following two objects and their methods comprise the essential elements of the ThinSQLite++ interface:

* @refmylib{database} → The database connection object.
* @refmylib{statement} → The prepared statement object.
* @refmylib{database::open} → Open a connection to a new or existing SQLite database. Creates @refmylib{database} instances.
* @refmylib{statement::create} → Compile SQL text into byte-code that will do the work of querying or updating the database. Creates @refmylib{statement} instances.
* @ref statement_bind "statement::bind" → Store application data into [parameters](https://www.sqlite.org/lang_expr.html#varparam) of the original SQL.
* @refmylib{statement::step} → Advance a @refmylib{statement} to the next result row or to completion.
* @refmylib{statement::column_value} → Column values in the current result row for a @refmylib{statement}
* @refmylib{row_range} → An STL forward range that wraps @refmylib{statement::step} loop to access statement results in a standard C++ range fashion.
* @ref database_exec "database::exec" → A set of wrapper functions that does @refmylib{statement::create}, @refmylib{statement::step}, etc. for a string of one or more SQL statements.
* @refmylib{row} → An STL random-access container of cells. Yielded by @refmylib{row_range} and @ref database_exec "database::exec"
* @refmylib{cell} → A cell contained in a @refmylib{row} that wraps @ref statement_column_info "statement::column_" family of functions for a given column.
* @refmylib{exception} → An exception thrown on any SQLite errors

## Introduction

ThinSQLite++ has many APIs. However, most of the APIs are optional and very specialized and can be ignored by beginners. The core API is small, simple, and easy to learn. This article summarizes the core API.

## Core Objects And Interfaces

The principal task of an SQL database engine is to evaluate SQL statements of SQL. To accomplish this, the developer needs two objects:

* The database connection object: @refmylib{database}
* The prepared statement object: @refmylib{statement}

Strictly speaking, the prepared statement object is not required since the convenience wrapper interface, @ref database_exec "database::exec", can be used and this convenience wrapper encapsulates and hides the prepared statement object. Nevertheless, an understanding of prepared statements is needed to make full use of ThinSQLite++.

The database connection and prepared statement objects are controlled by a small set of interfaces listed below.

* @refmylib{database::open}
* @refmylib{statement::create}
* @refmylib{statement::step}
* @refmylib{statement::column_value}

Here is a summary of what the core interfaces do:

* @refmylib{database::open}
  
  This static method opens a connection to an SQLite database file and returns an std::unique_ptr to the database connection object. This is often the first ThinSQLite++ API call that an application makes and is a prerequisite for most other ThinSQLite++ APIs. This routine is the only way to construct the database connection object.

* @refmylib{statement::create}
  
  This static method converts SQL text into a prepared statement object and returns an std::unique_ptr to that object. This interface requires a database connection reference created by a prior call to @refmylib{database::open} and a text string containing the SQL statement to be prepared. This API does not actually evaluate the SQL statement. It merely prepares the SQL statement for evaluation.
  
  Think of each SQL statement as a small computer program. The purpose of @refmylib{statement::create} is to compile that program into object code. The prepared statement is the object code. The @refmylib{statement::step} interface then runs the object code to get a result.

* @refmylib{statement::step}
  
  This method is used to evaluate a prepared statement that has been previously created by the @refmylib{statement::create} interface. The statement is evaluated up to the point where the first row of results are available. To advance to the second row of results, invoke @refmylib{statement::step} again. Continue invoking @refmylib{statement::step} until it returns `false` indicating that the statement is complete. Statements that do not return results (ex: INSERT, UPDATE, or DELETE statements) run to completion on a single call to @refmylib{statement::step}.

* @refmylib{statement::column_value}
  
  This template method returns a single column from the current row of a result set for a prepared statement that is being evaluated by @refmylib{statement::step}. Each time @refmylib{statement::step} stops with a new result set row, this method can be called multiple times to find the values of all columns in that row.

  You indicate the desired return value type via a template parameter to @refmylib{statement::column_value}. Possible types are:
  * int
  * int64_t
  * double
  * std::string_view
  * std::u8string_view (if `char8_t` is supported by your compiler/library)
  * @refmylib{blob_view} (a span of bytes)


## Typical Usage Of Core Objects and Methods

An application will typically use @refmylib{database::open} to create a single database connection during initialization. Note that @refmylib{database::open} can be used to either open existing database files or to create and open new database files. While many applications use only a single database connection, there is no reason why an application cannot call @refmylib{database::open} multiple times in order to open multiple database connections - either to the same database or to different databases. Sometimes a multi-threaded application will create separate database connections for each thread. Note that a single database connection can access two or more databases using the ATTACH SQL command, so it is not necessary to have a separate database connection for each database file.

To run an SQL statement, the application follows these steps:

1. Create a prepared statement using @refmylib{statement::create}.
2. Evaluate the prepared statement by @refmylib{statement::step} one or more times.
3. For queries, extract results by calling @refmylib{statement::column_value} in between two calls to @refmylib{statement::step}.
4. Handle (or decide not to) any @refmylib{exception}s thrown while doing the above

The foregoing is all one really needs to know in order to use SQLite effectively. All the rest is optimization and detail.

## Convenience Wrappers Around Core Methods

A @refmylib{row_range} is a convenience wrapper that exposes @refmylib{statement::step} loop as STL forward range. It yields @refmylib{row} objects which, in turn, model a random access container of @refmylib{cell}s. Using these wrappers you can handle data extraction as STL iteration, using range-for loops and STL algorithms.

The @ref database_exec "database::exec" methods are convenience wrappers that parse **multiple** SQL statements and execute @refmylib{statement::create} and @refmylib{statement::step} loop with a single function call. An optional callback function passed into @ref database_exec "database::exec" is used to process each @refmylib{row} of the result set. 

It is important to realize that neither @refmylib{row_range} nor @ref database_exec "database::exec" do anything that cannot be accomplished using the core methods. In fact, these wrappers are implemented purely in terms of the core routines.

## Binding Parameters and Reusing Prepared Statements

In prior discussion, it was assumed that each SQL statement is prepared once, evaluated, then destroyed. However, SQLite allows the same prepared statement to be evaluated multiple times. This is accomplished using the following:

* @refmylib{statement::reset} or @refmylib{auto_reset} RAII wrapper
* @ref statement_bind "statement::bind"

After a prepared statement has been evaluated by one or more calls to @refmylib{statement::step}, it can be reset in order to be evaluated again by a call to @refmylib{statement::reset}. Think of @refmylib{statement::reset} as rewinding the prepared statement program back to the beginning. Using @refmylib{statement::reset} on an existing prepared statement rather than creating a new prepared statement avoids unnecessary calls to @refmylib{statement::create}. For many SQL statements, the time needed to run @refmylib{statement::create} equals or exceeds the time needed by @refmylib{statement::step}. So avoiding calls to @refmylib{statement::create} can give a significant performance improvement.

As with any "destruction" action ensuring that @refmylib{statement::reset} is called on all code paths before reuse can be tricky and best dealt with RAII. The @refmylib{auto_reset} performs @refmylib{statement::reset} on destruction and can greatly simply managing statement reuse.

It is not commonly useful to evaluate the exact same SQL statement more than once. More often, one wants to evaluate similar statements. For example, you might want to evaluate an INSERT statement multiple times with different values. Or you might want to evaluate the same query multiple times using a different key in the WHERE clause. To accommodate this, SQLite allows SQL statements to contain [parameters](https://www.sqlite.org/lang_expr.html#varparam) which are "bound" to values prior to being evaluated. These values can later be changed and the same prepared statement can be evaluated a second time using the new values.

SQLite allows a [parameter](https://www.sqlite.org/lang_expr.html#varparam) wherever a string literal, blob literal, numeric constant, or NULL is allowed in queries or data modification statements. (DQL or DML) (Parameters may not be used for column or table names, or as values for constraints or default values. (DDL)) A [parameter](https://www.sqlite.org/lang_expr.html#varparam) takes one of the following forms:

* `?`
* `?NNN`
* `:AAA`
* `$AAA`
* `@AAA`

In the examples above, `NNN` is an integer value and `AAA` is an identifier. A parameter initially has a value of NULL. Prior to calling @refmylib{statement::step} for the first time or immediately after @refmylib{statement::reset}, the application can invoke one of the @ref statement_bind "statement::bind" methods to attach values to the parameters. Each call to @ref statement_bind "statement::bind" overrides prior bindings on the same parameter.

An application is allowed to prepare multiple SQL statements in advance and evaluate them as needed. There is no arbitrary limit to the number of outstanding prepared statements. Some applications call @refmylib{statement::create} multiple times at start-up to create all of the prepared statements they will ever need. Other applications keep a cache of the most recently used prepared statements and then reuse prepared statements out of the cache when available. Another approach is to only reuse prepared statements when they are inside of a loop.

## Configuring SQLite

The default configuration for SQLite works great for most applications. But sometimes developers want to tweak the setup to try to squeeze out a little more performance, or take advantage of some obscure feature.

The @refmylib{config} interface is used to make global, process-wide configuration changes for SQLite. The @refmylib{config} interface must be called before any database connections are created. The @refmylib{config} interface allows the programmer to do things like:

* Adjust how SQLite does [memory allocation](https://www.sqlite.org/malloc.html), including setting up alternative memory allocators appropriate for safety-critical real-time embedded systems and application-defined memory allocators.
* Set up a process-wide [error log](https://www.sqlite.org/errlog.html).
* Specify an application-defined page cache.
* Adjust the use of mutexes so that they are appropriate for various [threading models](https://www.sqlite.org/threadsafe.html), or substitute an application-defined mutex system.

After process-wide configuration is complete and database connections have been created, individual database connections can be configured using calls to @refmylib{database::limit} and @refmylib{database::config}.

## Extending SQLite

ThinSQLite++ includes interfaces that can be used to extend SQLite functionality. Such routines include:

* @refmylib{database::create_collation}
* @ref database_create_function "database::create_function"
* @refmylib{database::create_module}

The @refmylib{database::create_collation} methods is used to create new collating sequences for sorting text.

The @refmylib{database::create_function} methods create new SQL functions - either scalar or aggregate. The new function implementation typically makes use of the @refmylib{context} and  @refmylib{value} objects and the following additional interfaces:

* @refmylib{context::aggregate_context}
* @refmylib{context::result}
* @refmylib{context::user_data}

The @refmylib{database::create_module} can be used to create Virtual Table interfaces. More information can be found in @ref vtab-guide.

Shared libraries or DLLs can be used as @ref extension "loadable extensions" to SQLite.

## Other Interfaces

This article only mentions the most important and most commonly used ThinSQLite++ interfaces. The ThinSQLite++ library includes many other APIs implementing useful features that are not described here. Refer to the [list of topics](topics.html), content of the [thinsqlitepp namespace](namespacethinsqlitepp.html) or the [list of classes](annotated.html) for complete and authoritative information about all ThinSQLite++ interfaces.

