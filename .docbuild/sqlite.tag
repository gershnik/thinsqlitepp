<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile>

  <!-- Objects -->
  <compound kind="struct"><name>sqlite3</name><filename>c3ref/sqlite3.html</filename></compound>
  <compound kind="struct"><name>sqlite3_context</name><filename>c3ref/context.html</filename></compound>
  <compound kind="struct"><name>sqlite3_int64</name><filename>c3ref/int64.html</filename></compound>
  <compound kind="struct"><name>sqlite3_stmt</name><filename>c3ref/stmt.html</filename></compound>
  <compound kind="struct"><name>sqlite3_value</name><filename>c3ref/value.html</filename></compound>
  <compound kind="struct"><name>sqlite3_mem_methods</name><filename>c3ref/mem_methods.html</filename></compound>
  <compound kind="struct"><name>sqlite3_module</name><filename>c3ref/module.html</filename></compound>
  <compound kind="struct"><name>sqlite3_mutex</name><filename>c3ref/mutex.html</filename></compound>
  <compound kind="struct"><name>sqlite3_mutex_methods</name><filename>c3ref/mutex_methods.html</filename></compound>
  <compound kind="struct"><name>sqlite3_pcache_methods2</name><filename>c3ref/pcache_methods2.html</filename></compound>

  <!-- Macro Groups -->
  <compound kind="enum"><name>SQLITE_CONFIG_</name><filename>c3ref/c_config_covering_index_scan.html</filename></compound>
  <compound kind="enum"><name>SQLITE_DBCONFIG_</name><filename>c3ref/c_dbconfig_defensive.html</filename></compound>
  <compound kind="enum"><name>SQLITE_PREPARE_</name><filename>c3ref/c_prepare_normalize.html</filename></compound>

  <!-- Function Groups -->
  <compound kind="page"><name>sqlite3_column_</name><filename>c3ref/column_blob</filename></compound>
  <compound kind="page"><name>sqlite3_bind_</name><filename>c3ref/bind_blob</filename></compound>
  <compound kind="page"><name>sqlite3_value_</name><filename>c3ref/value_blob</filename></compound>

  <!-- Functions and Constants -->

  <compound kind="file">
    <name>sqlite3.h</name>

    <member kind="define"><name>SQLITE_BUSY</name><anchorfile>rescode.html</anchorfile><anchor>busy</anchor></member>
    <member kind="define"><name>SQLITE_DONE</name><anchorfile>rescode.html</anchorfile><anchor>done</anchor></member>
    <member kind="define"><name>SQLITE_ERROR</name><anchorfile>rescode.html</anchorfile><anchor>error</anchor></member>
    <member kind="define"><name>SQLITE_ROW</name><anchorfile>rescode.html</anchorfile><anchor>row</anchor></member>
    <member kind="define"><name>SQLITE_STATIC</name><anchorfile>c3ref/c_static.html</anchorfile></member>
    <member kind="define"><name>SQLITE_TRANSIENT</name><anchorfile>c3ref/c_static.html</anchorfile></member>

    <member kind="function"><name>sqlite3_aggregate_context</name><anchorfile>c3ref/aggregate_context.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_blob</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_double</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_int</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_int64</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_null</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_parameter_count</name><anchorfile>c3ref/bind_parameter_count.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_parameter_index</name><anchorfile>c3ref/bind_parameter_index.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_parameter_name</name><anchorfile>c3ref/bind_parameter_name.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_pointer</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_text</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_value</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_bind_zeroblob</name><anchorfile>c3ref/bind_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_busy_handler</name><anchorfile>c3ref/busy_handler.html</anchorfile></member>
    <member kind="function"><name>sqlite3_busy_timeout</name><anchorfile>c3ref/busy_timeout.html</anchorfile></member>
    <member kind="function"><name>sqlite3_changes</name><anchorfile>c3ref/changes.html</anchorfile></member>
    <member kind="function"><name>sqlite3_clear_bindings</name><anchorfile>c3ref/clear_bindings.html</anchorfile></member>
    <member kind="function"><name>sqlite3_close_v2</name><anchorfile>c3ref/close.html</anchorfile></member>
    <member kind="function"><name>sqlite3_collation_needed</name><anchorfile>c3ref/collation_needed.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_count</name><anchorfile>c3ref/column_count.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_database_name</name><anchorfile>c3ref/column_database_name.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_decltype</name><anchorfile>c3ref/column_decltype.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_origin_name</name><anchorfile>c3ref/column_database_name.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_table_name</name><anchorfile>c3ref/column_database_name.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_name</name><anchorfile>c3ref/column_name.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_type</name><anchorfile>c3ref/column_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_column_value</name><anchorfile>c3ref/column_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_commit_hook</name><anchorfile>c3ref/commit_hook.html</anchorfile></member>
    <member kind="function"><name>sqlite3_config</name><anchorfile>c3ref/config.html</anchorfile></member>
    <member kind="function"><name>sqlite3_context_db_handle</name><anchorfile>c3ref/context_db_handle.html</anchorfile></member>
    <member kind="function"><name>sqlite3_create_collation_v2</name><anchorfile>c3ref/create_collation.html</anchorfile></member>
    <member kind="function"><name>sqlite3_create_function_v2</name><anchorfile>c3ref/create_function.html</anchorfile></member>
    <member kind="function"><name>sqlite3_create_module_v2</name><anchorfile>c3ref/create_module.html</anchorfile></member>
    <member kind="function"><name>sqlite3_create_window_function</name><anchorfile>c3ref/create_function.html</anchorfile></member>
    <member kind="function"><name>sqlite3_data_count</name><anchorfile>c3ref/data_count.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_cacheflush</name><anchorfile>c3ref/db_cacheflush.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_config</name><anchorfile>c3ref/db_config.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_filename</name><anchorfile>c3ref/db_filename.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_handle</name><anchorfile>c3ref/db_handle.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_mutex</name><anchorfile>c3ref/db_mutex.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_readonly</name><anchorfile>c3ref/db_readonly.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_release_memory</name><anchorfile>c3ref/db_release_memory.html</anchorfile></member>
    <member kind="function"><name>sqlite3_db_status</name><anchorfile>c3ref/db_status.html</anchorfile></member>
    <member kind="function"><name>sqlite3_declare_vtab</name><anchorfile>c3ref/declare_vtab.html</anchorfile></member>
    <member kind="function"><name>sqlite3_drop_modules</name><anchorfile>c3ref/drop_modules.html</anchorfile></member>
    <member kind="function"><name>sqlite3_enable_load_extension</name><anchorfile>c3ref/enable_load_extension.html</anchorfile></member>
    <member kind="function"><name>sqlite3_errmsg</name><anchorfile>c3ref/errcode.html</anchorfile></member>
    <member kind="function"><name>sqlite3_errstr</name><anchorfile>c3ref/errcode.html</anchorfile></member>
    <member kind="function"><name>sqlite3_exec</name><anchorfile>c3ref/exec.html</anchorfile></member>
    <member kind="function"><name>sqlite3_expanded_sql</name><anchorfile>c3ref/expanded_sql.html</anchorfile></member>
    <member kind="function"><name>sqlite3_extended_errcode</name><anchorfile>c3ref/errcode.html</anchorfile></member>
    <member kind="function"><name>sqlite3_extended_result_codes</name><anchorfile>c3ref/extended_result_codes.html</anchorfile></member>
    <member kind="function"><name>sqlite3_file_control</name><anchorfile>c3ref/file_control.html</anchorfile></member>
    <member kind="function"><name>sqlite3_finalize</name><anchorfile>c3ref/finalize.html</anchorfile></member>
    <member kind="function"><name>sqlite3_free</name><anchorfile>c3ref/free.html</anchorfile></member>
    <member kind="function"><name>sqlite3_get_autocommit</name><anchorfile>c3ref/get_autocommit.html</anchorfile></member>
    <member kind="function"><name>sqlite3_get_auxdata</name><anchorfile>c3ref/get_auxdata.html</anchorfile></member>
    <member kind="function"><name>sqlite3_initialize</name><anchorfile>c3ref/initialize.html</anchorfile></member>
    <member kind="function"><name>sqlite3_interrupt</name><anchorfile>c3ref/interrupt.html</anchorfile></member>
    <member kind="function"><name>sqlite3_is_interrupted</name><anchorfile>c3ref/interrupt.html</anchorfile></member>
    <member kind="function"><name>sqlite3_last_insert_rowid</name><anchorfile>c3ref/last_insert_rowid.html</anchorfile></member>
    <member kind="function"><name>sqlite3_limit</name><anchorfile>c3ref/limit.html</anchorfile></member>
    <member kind="function"><name>sqlite3_load_extension</name><anchorfile>c3ref/load_extension.html</anchorfile></member>
    <member kind="function"><name>sqlite3_malloc</name><anchorfile>c3ref/free.html</anchorfile></member>
    <member kind="function"><name>sqlite3_malloc64</name><anchorfile>c3ref/free.html</anchorfile></member>
    <member kind="function"><name>sqlite3_mutex_alloc</name><anchorfile>c3ref/mutex_alloc.html</anchorfile></member>
    <member kind="function"><name>sqlite3_mutex_enter</name><anchorfile>c3ref/mutex_alloc.html</anchorfile></member>
    <member kind="function"><name>sqlite3_mutex_leave</name><anchorfile>c3ref/mutex_alloc.html</anchorfile></member>
    <member kind="function"><name>sqlite3_mutex_try</name><anchorfile>c3ref/mutex_alloc.html</anchorfile></member>
    <member kind="function"><name>sqlite3_next_stmt</name><anchorfile>c3ref/next_stmt.html</anchorfile></member>
    <member kind="function"><name>sqlite3_open_v2</name><anchorfile>c3ref/open.html</anchorfile></member>
    <member kind="function"><name>sqlite3_overload_function</name><anchorfile>c3ref/overload_function.html</anchorfile></member>
    <member kind="function"><name>sqlite3_prepare_v2</name><anchorfile>c3ref/prepare.html</anchorfile></member>
    <member kind="function"><name>sqlite3_prepare_v3</name><anchorfile>c3ref/prepare.html</anchorfile></member>
    <member kind="function"><name>sqlite3_progress_handler</name><anchorfile>c3ref/progress_handler.html</anchorfile></member>
    <member kind="function"><name>sqlite3_reset</name><anchorfile>c3ref/reset.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_blob</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_double</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_error</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_error_code</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_error_nomem</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_error_toobig</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_int</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_int64</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_null</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_subtype</name><anchorfile>c3ref/result_subtype.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_text</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_value</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_result_zeroblob</name><anchorfile>c3ref/result_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_rollback_hook</name><anchorfile>c3ref/commit_hook.html</anchorfile></member>
    <member kind="function"><name>sqlite3_set_auxdata</name><anchorfile>c3ref/get_auxdata.html</anchorfile></member>
    <member kind="function"><name>sqlite3_set_last_insert_rowid</name><anchorfile>c3ref/set_last_insert_rowid.html</anchorfile></member>
    <member kind="function"><name>sqlite3_shutdown</name><anchorfile>c3ref/initialize.html</anchorfile></member>
    <member kind="function"><name>sqlite3_sql</name><anchorfile>c3ref/expanded_sql.html</anchorfile></member>
    <member kind="function"><name>sqlite3_step</name><anchorfile>c3ref/step.html</anchorfile></member>
    <member kind="function"><name>sqlite3_stmt_busy</name><anchorfile>c3ref/stmt_busy.html</anchorfile></member>
    <member kind="function"><name>sqlite3_stmt_isexplain</name><anchorfile>c3ref/stmt_isexplain.html</anchorfile></member>
    <member kind="function"><name>sqlite3_stmt_readonly</name><anchorfile>c3ref/stmt_readonly.html</anchorfile></member>
    <member kind="function"><name>sqlite3_system_errno</name><anchorfile>c3ref/system_errno.html</anchorfile></member>
    <member kind="function"><name>sqlite3_table_column_metadata</name><anchorfile>c3ref/table_column_metadata.html</anchorfile></member>
    <member kind="function"><name>sqlite3_total_changes</name><anchorfile>c3ref/total_changes.html</anchorfile></member>
    <member kind="function"><name>sqlite3_txn_state</name><anchorfile>c3ref/txn_state.html</anchorfile></member>
    <member kind="function"><name>sqlite3_user_data</name><anchorfile>c3ref/user_data.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_dup</name><anchorfile>c3ref/value_dup.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_free</name><anchorfile>c3ref/value_dup.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_frombind</name><anchorfile>c3ref/value_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_nochange</name><anchorfile>c3ref/value_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_numeric_type</name><anchorfile>c3ref/value_blob.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_subtype</name><anchorfile>c3ref/value_subtype.html</anchorfile></member>
    <member kind="function"><name>sqlite3_value_type</name><anchorfile>c3ref/value_blob.html</anchorfile></member>
    

  </compound>

</tagfile>
