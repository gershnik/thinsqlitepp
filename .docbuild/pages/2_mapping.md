# Mappings of SQLite API to ThinSQLite++ {#mapping}

This page contains mappings of SQLite objects and functions to ThinSQLite++ and serves as a 
status page of what is or isn't available in this library.

[TOC]



## Legend

* **out of scope**: Functionality that is not implemented and never will be. Common 
  examples are UTF-16 APIs or VFS related API.

* <span style="color:orange"> **not implemented** </span>: Functionality that is currently not 
  implemented but should be and might be added in a future version.

## Types

| Type                              |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3                         | @refmylib{database}
| ::sqlite3_api_routines            | **out of scope**
| ::sqlite3_backup                  | @refmylib{backup}
| ::sqlite3_blob                    | @refmylib{blob}
| ::sqlite3_context                 | @refmylib{context}
| ::sqlite3_file                    | **out of scope**
| ::sqlite3_index_info              | @refmylib{index_info}
| ::sqlite3_int64                   | @ref std::int64_t
| ::sqlite3_uint64                  | @ref std::size_t
| ::sqlite3_io_methods              | **out of scope**
| ::sqlite3_mem_methods             | **out of scope**
| ::sqlite3_module                  | By itself **out of scope**. Functionality provided as part of @refmylib{vtab}
| ::sqlite3_mutex                   | @refmylib{mutex}
| ::sqlite3_mutex_methods           | **out of scope**
| ::sqlite3_pcache                  | **out of scope**
| ::sqlite3_pcache_methods2         | **out of scope**
| ::sqlite3_pcache_page             | **out of scope**
| ::sqlite3_snapshot                | @refmylib{snapshot}
| ::sqlite3_stmt                    | @refmylib{statement}
| ::sqlite3_str                     | **out of scope**
| ::sqlite3_value                   | @refmylib{value}
| ::sqlite3_vfs                     | **out of scope**
| ::sqlite3_vtab                    | @refmylib{vtab}
| ::sqlite3_vtab_cursor             | @refmylib{vtab::cursor}

## Functions

### A-B

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_aggregate_context       | @refmylib{context::aggregate_context}
| ::sqlite3_auto_extension          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_autovacuum_pages        | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_backup_finish           | @refmylib{backup}
| ::sqlite3_backup_init             | @refmylib{backup::init}
| ::sqlite3_backup_pagecount        | @refmylib{backup::pagecount}
| ::sqlite3_backup_remaining        | @refmylib{backup::remaining}
| ::sqlite3_backup_step             | @refmylib{backup::step}
| ::sqlite3_bind_blob               | @refmylib{statement::bind}, @refmylib{statement::bind_reference}
| ::sqlite3_bind_blob64             | @refmylib{statement::bind}, @refmylib{statement::bind_reference}
| ::sqlite3_bind_double             | @refmylib{statement::bind}
| ::sqlite3_bind_int                | @refmylib{statement::bind}
| ::sqlite3_bind_int64              | @refmylib{statement::bind}
| ::sqlite3_bind_null               | @refmylib{statement::bind}
| ::sqlite3_bind_parameter_count    | @refmylib{statement::bind_parameter_count}
| ::sqlite3_bind_parameter_index    | @refmylib{statement::bind_parameter_index}
| ::sqlite3_bind_parameter_name     | @refmylib{statement::bind_parameter_name}
| ::sqlite3_bind_pointer            | @refmylib{statement::bind}
| ::sqlite3_bind_text               | @refmylib{statement::bind}, @refmylib{statement::bind_reference}
| ::sqlite3_bind_text16             | **out of scope**
| ::sqlite3_bind_text64             | @refmylib{statement::bind}
| ::sqlite3_bind_value              | @refmylib{statement::bind}
| ::sqlite3_bind_zeroblob           | @refmylib{statement::bind}
| ::sqlite3_bind_zeroblob64         | @refmylib{statement::bind}
| ::sqlite3_blob_bytes              | @refmylib{blob::bytes}
| ::sqlite3_blob_close              | @refmylib{blob}
| ::sqlite3_blob_open               | @refmylib{database::open_blob}
| ::sqlite3_blob_read               | @refmylib{blob::read}
| ::sqlite3_blob_reopen             | @refmylib{blob::reopen}
| ::sqlite3_blob_write              | @refmylib{blob::write}
| ::sqlite3_busy_handler            | @refmylib{database::busy_handler}
| ::sqlite3_busy_timeout            | @refmylib{database::busy_timeout}

### C

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_cancel_auto_extension   | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_changes                 | @refmylib{database::changes}
| ::sqlite3_changes64               | @refmylib{database::changes}
| ::sqlite3_clear_bindings          | @refmylib{statement::clear_bindings}
| ::sqlite3_close                   | **out of scope**
| ::sqlite3_close_v2                | @refmylib{database}
| ::sqlite3_collation_needed        | @refmylib{database::collation_needed}
| ::sqlite3_collation_needed16      | **out of scope**
| ::sqlite3_column_blob             | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_column_bytes            | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_column_bytes16          | **out of scope**
| ::sqlite3_column_count            | @refmylib{statement::column_count}, 
| ::sqlite3_column_database_name    | @refmylib{statement::column_database_name}, @refmylib{cell::database_name}
| ::sqlite3_column_database_name16  | **out of scope**
| ::sqlite3_column_decltype         | @refmylib{statement::column_declared_type}, @refmylib{cell::declared_type}
| ::sqlite3_column_decltype16       | **out of scope**
| ::sqlite3_column_double           | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_column_int              | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_column_int64            | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_column_name             | @refmylib{statement::column_name}, @refmylib{cell::name}
| ::sqlite3_column_name16           | **out of scope**
| ::sqlite3_column_origin_name      | @refmylib{statement::column_origin_name}, @refmylib{cell::origin_name}
| ::sqlite3_column_origin_name16    | **out of scope**
| ::sqlite3_column_table_name       | @refmylib{statement::column_table_name}, @refmylib{cell::table_name}
| ::sqlite3_column_table_name16     | **out of scope**
| ::sqlite3_column_text             | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_column_text16           | **out of scope**
| ::sqlite3_column_type             | @refmylib{statement::column_type}, @refmylib{cell::type}
| ::sqlite3_column_value            | @refmylib{statement::column_value}, @refmylib{cell::value}
| ::sqlite3_commit_hook             | @refmylib{database::commit_hook}
| ::sqlite3_compileoption_get       | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_compileoption_used      | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_complete                | **out of scope**
| ::sqlite3_complete16              | **out of scope**
| ::sqlite3_config                  | @refmylib{database::config}
| ::sqlite3_context_db_handle       | @refmylib{context::database}
| ::sqlite3_create_collation        | @refmylib{database::create_collation}
| ::sqlite3_create_collation16      | **out of scope**
| ::sqlite3_create_collation_v2     | @refmylib{database::create_collation}
| ::sqlite3_create_filename         | **out of scope**
| ::sqlite3_create_function         | @refmylib{database::create_function}
| ::sqlite3_create_function16       | **out of scope**
| ::sqlite3_create_function_v2      | @refmylib{database::create_function}
| ::sqlite3_create_module           | @refmylib{database::create_module}, @refmylib{vtab::create_module}
| ::sqlite3_create_module_v2        | @refmylib{database::create_module}, @refmylib{vtab::create_module}
| ::sqlite3_create_window_function  | @refmylib{database::create_window_function}

### D-E

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_data_count              | @refmylib{statement::data_count}
| ::sqlite3_database_file_object    | **out of scope**
| ::sqlite3_db_cacheflush           | @refmylib{database::cacheflush}
| ::sqlite3_db_config               | @refmylib{database::config}
| ::sqlite3_db_filename             | @refmylib{database::filename}
| ::sqlite3_db_handle               | @refmylib{statement::database}
| ::sqlite3_db_mutex                | @refmylib{database::mutex}
| ::sqlite3_db_name                 | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_db_readonly             | @refmylib{database::readonly}
| ::sqlite3_db_release_memory       | @refmylib{database::release_memory}
| ::sqlite3_db_status               | @refmylib{database::status()}
| ::sqlite3_declare_vtab            | @refmylib{database::declare_vtab}
| ::sqlite3_deserialize             | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_drop_modules            | @refmylib{database::drop_modules}
| ::sqlite3_enable_load_extension   | @refmylib{database::enable_load_extension}
| ::sqlite3_enable_shared_cache     | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_errcode                 | @refmylib{exception::primary_error_code}, @refmylib{error::primary}
| ::sqlite3_errmsg                  | @refmylib{exception::what}, @refmylib{error::message}
| ::sqlite3_errmsg16                | **out of scope**
| ::sqlite3_error_offset            | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_errstr                  | @refmylib{exception::what}, @refmylib{error::message}
| ::sqlite3_exec                    | @refmylib{database::exec}
| ::sqlite3_expanded_sql            | @refmylib{statement::expanded_sql}
| ::sqlite3_extended_errcode        | @refmylib{exception::extended_error_code}, @refmylib{error::extended}
| ::sqlite3_extended_result_codes   | @refmylib{database::extended_result_codes}


### F-L

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_file_control            | @refmylib{database::file_control}
| ::sqlite3_filename_database       | **out of scope**
| ::sqlite3_filename_journal        | **out of scope**
| ::sqlite3_filename_wal            | **out of scope**
| ::sqlite3_finalize                | @refmylib{statement}
| ::sqlite3_free                    | @refmylib{sqlite_deleter}, @refmylib{sqlite_allocated},  @refmylib{sqlite_allocator}
| ::sqlite3_free_filename           | **out of scope**
| ::sqlite3_free_table              | **out of scope**
| ::sqlite3_get_autocommit          | @refmylib{database::get_autocommit}
| ::sqlite3_get_auxdata             | @refmylib{context::get_auxdata}
| ::sqlite3_get_clientdata          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_get_table               | **out of scope**
| ::sqlite3_hard_heap_limit64       | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_initialize              | @refmylib{initialize}
| ::sqlite3_interrupt               | @refmylib{database::interrupt}
| ::sqlite3_is_interrupted          | @refmylib{database::is_interrupted}
| ::sqlite3_keyword_check           | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_keyword_count           | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_keyword_name            | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_last_insert_rowid       | @refmylib{database::last_insert_rowid}
| ::sqlite3_libversion              | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_libversion_number       | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_limit                   | @refmylib{database::limit}
| ::sqlite3_load_extension          | @refmylib{database::load_extension}
| ::sqlite3_log                     | <span style="color:orange"> **not implemented** </span>


### M-O

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_malloc                  | @refmylib{sqlite_allocated},  @refmylib{sqlite_allocator}
| ::sqlite3_malloc64                | @refmylib{sqlite_allocated},  @refmylib{sqlite_allocator}
| ::sqlite3_memory_highwater        | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_memory_used             | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_mprintf                 | **out of scope**
| ::sqlite3_msize                   | **out of scope**
| ::sqlite3_mutex_alloc             | @refmylib{mutex::alloc}
| ::sqlite3_mutex_enter             | @refmylib{mutex::lock}
| ::sqlite3_mutex_free              | @refmylib{mutex}
| ::sqlite3_mutex_held              | **out of scope**
| ::sqlite3_mutex_leave             | @refmylib{mutex::unlock}
| ::sqlite3_mutex_notheld           | **out of scope**
| ::sqlite3_mutex_try               | @refmylib{mutex::try_lock}
| ::sqlite3_next_stmt               | @refmylib{database::next_statement}
| ::sqlite3_normalized_sql          | **out of scope**
| ::sqlite3_open                    | @refmylib{database::open}
| ::sqlite3_open16                  | **out of scope**
| ::sqlite3_open_v2                 | @refmylib{database::open}
| ::sqlite3_os_end                  | **out of scope**
| ::sqlite3_os_init                 | **out of scope**
| ::sqlite3_overload_function       | @refmylib{database::overload_function}


### P-R

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_prepare                 | @refmylib{statement::create}
| ::sqlite3_prepare16               | **out of scope**
| ::sqlite3_prepare16_v2            | **out of scope**
| ::sqlite3_prepare16_v3            | **out of scope**
| ::sqlite3_prepare_v2              | @refmylib{statement::create}
| ::sqlite3_prepare_v3              | @refmylib{statement::create}
| ::sqlite3_preupdate_blobwrite     | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_preupdate_count         | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_preupdate_depth         | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_preupdate_hook          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_preupdate_new           | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_preupdate_old           | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_progress_handler        | @refmylib{database::progress_handler}
| ::sqlite3_randomness              | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_realloc                 | **out of scope**
| ::sqlite3_realloc64               | **out of scope**
| ::sqlite3_release_memory          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_reset                   | @refmylib{statement::reset}
| ::sqlite3_reset_auto_extension    | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_result_blob             | @refmylib{context::result}, @refmylib{context::result_reference}
| ::sqlite3_result_blob64           | @refmylib{context::result}, @refmylib{context::result_reference}
| ::sqlite3_result_double           | @refmylib{context::result}
| ::sqlite3_result_error            | @refmylib{context::error}
| ::sqlite3_result_error16          | **out of scope**
| ::sqlite3_result_error_code       | @refmylib{context::error}
| ::sqlite3_result_error_nomem      | @refmylib{context::error_nomem}
| ::sqlite3_result_error_toobig     | @refmylib{context::error_toobig}
| ::sqlite3_result_int              | @refmylib{context::result}
| ::sqlite3_result_int64            | @refmylib{context::result}
| ::sqlite3_result_null             | @refmylib{context::result}
| ::sqlite3_result_pointer          | @refmylib{context::result}
| ::sqlite3_result_subtype          | @refmylib{context::result_subtype}
| ::sqlite3_result_text             | @refmylib{context::result}, @refmylib{context::result_reference}
| ::sqlite3_result_text16           | **out of scope**
| ::sqlite3_result_text16be         | **out of scope**
| ::sqlite3_result_text16le         | **out of scope**
| ::sqlite3_result_text64           | @refmylib{context::result}, @refmylib{context::result_reference}
| ::sqlite3_result_value            | @refmylib{context::result}
| ::sqlite3_result_zeroblob         | @refmylib{context::result}
| ::sqlite3_result_zeroblob64       | @refmylib{context::result}
| ::sqlite3_rollback_hook           | @refmylib{database::rollback_hook}


### S

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_serialize               | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_set_authorizer          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_set_auxdata             | @refmylib{context::set_auxdata}
| ::sqlite3_set_clientdata          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_set_last_insert_rowid   | @refmylib{database::set_last_insert_rowid}
| ::sqlite3_shutdown                | @refmylib{shutdown}
| ::sqlite3_sleep                   | **out of scope**
| ::sqlite3_snapshot_cmp            | @refmylib{snapshot::compare} and comparison operators
| ::sqlite3_snapshot_free           | @refmylib{snapshot}
| ::sqlite3_snapshot_get            | @refmylib{database::get_snapshot}
| ::sqlite3_snapshot_open           | @refmylib{database::open_snapshot}
| ::sqlite3_snapshot_recover        | @refmylib{database::recover_snapshot}
| ::sqlite3_snprintf                | **out of scope**
| ::sqlite3_soft_heap_limit64       | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_sourceid                | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_sql                     | @refmylib{statement::sql}
| ::sqlite3_status                  | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_status64                | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_step                    | @refmylib{statement::step}
| ::sqlite3_stmt_busy               | @refmylib{statement::busy}
| ::sqlite3_stmt_explain            | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_stmt_isexplain          | @refmylib{statement::isexplain}
| ::sqlite3_stmt_readonly           | @refmylib{statement::readonly}
| ::sqlite3_stmt_scanstatus         | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_stmt_scanstatus_reset   | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_stmt_scanstatus_v2      | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_stmt_status             | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_str_append              | **out of scope**
| ::sqlite3_str_appendall           | **out of scope**
| ::sqlite3_str_appendchar          | **out of scope**
| ::sqlite3_str_appendf             | **out of scope**
| ::sqlite3_str_errcode             | **out of scope**
| ::sqlite3_str_finish              | **out of scope**
| ::sqlite3_str_length              | **out of scope**
| ::sqlite3_str_new                 | **out of scope**
| ::sqlite3_str_reset               | **out of scope**
| ::sqlite3_str_value               | **out of scope**
| ::sqlite3_str_vappendf            | **out of scope**
| ::sqlite3_strglob                 | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_stricmp                 | **out of scope**
| ::sqlite3_strlike                 | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_strnicmp                | **out of scope**
| ::sqlite3_system_errno            | @refmylib{exception::system_error_code}, @refmylib{error::system}


### T-W

| Function                          |  Mapped To                                                     |
|-----------------------------------|----------------------------------------------------------------|
| ::sqlite3_table_column_metadata   | @refmylib{database::table_column_metadata}
| ::sqlite3_test_control            | **out of scope**
| ::sqlite3_threadsafe              | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_total_changes           | @refmylib{database::total_changes}
| ::sqlite3_total_changes64         | @refmylib{database::total_changes}
| ::sqlite3_trace_v2                | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_txn_state               | @refmylib{database::txn_state}
| ::sqlite3_unlock_notify           | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_update_hook             | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_uri_boolean             | **out of scope**
| ::sqlite3_uri_int64               | **out of scope**
| ::sqlite3_uri_key                 | **out of scope**
| ::sqlite3_uri_parameter           | **out of scope**
| ::sqlite3_user_data               | @refmylib{context::user_data}
| ::sqlite3_value_blob              | @refmylib{value::get}
| ::sqlite3_value_bytes             | @refmylib{value::get}
| ::sqlite3_value_bytes16           | **out of scope**
| ::sqlite3_value_double            | @refmylib{value::get}
| ::sqlite3_value_dup               | @refmylib{value::dup}
| ::sqlite3_value_encoding          | **out of scope**
| ::sqlite3_value_free              | @refmylib{value}
| ::sqlite3_value_frombind          | @refmylib{value::frombind}
| ::sqlite3_value_int               | @refmylib{value::get}
| ::sqlite3_value_int64             | @refmylib{value::get}
| ::sqlite3_value_nochange          | @refmylib{value::nochange}
| ::sqlite3_value_numeric_type      | @refmylib{value::numeric_type}
| ::sqlite3_value_pointer           | @refmylib{value::get}
| ::sqlite3_value_subtype           | @refmylib{value::subtype}
| ::sqlite3_value_text              | @refmylib{value::get}
| ::sqlite3_value_text16            | **out of scope**
| ::sqlite3_value_text16be          | **out of scope**
| ::sqlite3_value_text16le          | **out of scope**
| ::sqlite3_value_type              | @refmylib{value::type}
| ::sqlite3_version                 | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_vfs_find                | **out of scope**
| ::sqlite3_vfs_register            | **out of scope**
| ::sqlite3_vfs_unregister          | **out of scope**
| ::sqlite3_vmprintf                | **out of scope**
| ::sqlite3_vsnprintf               | **out of scope**
| ::sqlite3_vtab_collation          | @refmylib{index_info::collation}
| ::sqlite3_vtab_config             | @refmylib{database::vtab_config}
| ::sqlite3_vtab_distinct           | @refmylib{index_info::distinct}
| ::sqlite3_vtab_in                 | @refmylib{index_info::is_in}
| ::sqlite3_vtab_in_first           | @refmylib{value::in_first}
| ::sqlite3_vtab_in_next            | @refmylib{value::in_next}
| ::sqlite3_vtab_nochange           | @refmylib{context::vtab_nochange}
| ::sqlite3_vtab_on_conflict        | @refmylib{database::vtab_on_conflict}
| ::sqlite3_vtab_rhs_value          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_wal_autocheckpoint      | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_wal_checkpoint          | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_wal_checkpoint_v2       | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_wal_hook                | <span style="color:orange"> **not implemented** </span>
| ::sqlite3_win32_set_directory     | **out of scope**
| ::sqlite3_win32_set_directory16   | **out of scope**
| ::sqlite3_win32_set_directory8    | **out of scope**

