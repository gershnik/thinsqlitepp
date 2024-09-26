# Using ThinSQLite++ in an SQLite extension {#extension}


ThinSQLite++ can be used in SQLite++ [extensions](https://www.sqlite.org/loadext.html). 

To use it in an extension you need to **either**:

* Include `<sqlite3ext.h>` and specify `SQLITE_EXTENSION_INIT3` *before including any of ThinSQLite++ headers*.
  For example:
  ```cpp
  #include <sqlite3ext.h>
  SQLITE_EXTENSION_INIT3

  #include <thinsqlitepp/database.hpp>
  ``` 

* Define `THINSQLITEPP_BUILDING_EXTENSION` to 1 using your build system (or in code _before including
  any ThinSQLite++ headers_). For example:
  ```cpp
  #define THINSQLITEPP_BUILDING_EXTENSION 1
  #include <thinsqlitepp/database.hpp>
  ```

Either way achieves the same effect.

The extension's entry point should look similar to the below:

```cpp

//Use this macro in *one* source file usually where your entry point is
SQLITE_EXTENSION_INIT1

using namespace thinsqlitepp;

extern "C"
#if defined(_WIN32)
__declspec(dllexport)
#elif defined(__GNUC__)
[[gnu::visibility("default")]]
#endif
/* TODO: Change the entry point name so that "extension" is replaced by
** text derived from the shared library filename as follows:  Copy every
** ASCII alphabetic character from the filename after the last "/" through
** the next following ".", converting each character to lowercase, and
** discarding the first three characters if they are "lib".
*/
int sqlite3_extension_init(database * db, char ** pzErrMsg, const sqlite3_api_routines * pApi){
    SQLITE_EXTENSION_INIT2(pApi);

    try{
        /* Insert here calls to
        **     db->create_function(),
        **     db->create_collation(),
        **     db->create_module(), 
        **     etc.
        ** to register the new features that your extension adds.
        */
        return SQLITE_OK;
    }
    catch(exception & ex) {
        *pzErrMsg = const_cast<char *>(ex.error().extract_message().release());
        return ex.extended_error_code();
    }
    catch(std::exception & ex) {
        auto what = ex.what();
        auto len = strlen(what) + 1;
        if (auto message = (const char *)sqlite3_malloc(len)) {
            memcpy(*pzErrMsg, what, len);
        }
        return SQLITE_ERROR;
    }
}

```

Note that the first parameter of the `sqlite3_extension_init` can safely be declared as
`thinsqlitepp::database *` rather than `sqlite3 *`. This is due to the fact that ThinSQLite++
classes are [fake wrappers](https://github.com/gershnik/thinsqlitepp#fake-classes)

