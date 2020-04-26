#一、main

```cpp
main
--capture_tool_command(argc, argv);
----make_argv
--mysql_server_init(-1, NULL, NULL)
----my_init
------my_thread_global_init
--------pthread_mutexattr_init(&my_fast_mutexattr);
--------my_create_thread_local_key
--------mysql_mutex_init(key_THR_LOCK_malloc, &THR_LOCK_malloc, MY_MUTEX_INIT_FAST);
------my_thread_init
----mysql_client_plugin_init
--handle_options(argc, argv, &client_defaults, &server_defaults);
--xb_init
----xb_mysql_connect//??
----get_mysql_vars//??
----parse_show_engine_innodb_status//
--xtrabackup_backup_func
```

#二、xtrabackup_backup_func

backup mode
##2.1 简化版
```cpp
xtrabackup_backup_func
--recv_find_max_checkpoint
--log_group_header_read
--// 初始化datasink，datasink用来完成读/写的多样化（比如读出后转化为xbstream格式）
--xtrabackup_init_datasinks();
--// 将lsn转换为Page NO.
--xtrabackup_choose_lsn_offset(checkpoint_lsn_start);
--// 先拷贝Redo日志到checkpoint_lsn_start，checkpoint_lsn_start是Checkpoint记录点
--// 说明checkpoint_lsn_start之前的日志记录修改的Page都已经持久化
--xtrabackup_copy_logfile(checkpoint_lsn_start, FALSE)
--// 1. 创建日志拷贝线程
--os_thread_create(log_copying_thread, NULL, &log_copying_thread_id);
--// 2. 初始化所有表的MDL锁，详细分析见后文
--mdl_lock_init();
--// 3. 创建N个数据拷贝线程
--os_thread_create(data_copy_thread_func, data_threads + i, &data_threads[i].id);
--// 等待所有数据拷贝线程退出
--// 4. 这里会执行LOCK TABLES FOR BACKUP / FLUSH TABLES WITH READ LOCK，
    // 记录Binlog的位置，和已经写入的LSN的值。在备份恢复时，会恢复到这个时间点
--// 5. 拷贝非InnoDB文件，包括frm、MYD、MYI等
--backup_start()
--// 6. 停止日志拷贝线程
--os_event_set(log_copying_stop);
--// 7. 释放全局读锁
--backup_finish()
--// 8. 释放所有表的MDL锁
--mdl_unlock_all()
```

##2.2 详细版

```cpp
xtrabackup_backup_func
--has_tokudb_plugin
----query = "SELECT COUNT(*) FROM information_schema.plugins WHERE plugin_name='TokuDB'"
----xb_mysql_query//?
----mysql_fetch_row
--innodb_init_param
----init_tmpdir
----xb_init_log_block_size
--xb_normalize_init_values
----SysTablespace::normalize
--srv_general_init//Initializes the synchronization primitives, memory system, and the thread, local storage.
----sync_check_init
----recv_sys_var_init
----os_thread_init
----trx_pool_init
----que_init
----row_mysql_init
--xb_fil_io_init
----os_aio_init
----fil_init//Initializes the tablespace memory cache.
----fsp_init
--recv_sys_create
--recv_sys_init
--open_or_create_log_file
--recv_find_max_checkpoint
--log_group_header_read//get checkpoint_lsn
--xtrabackup_init_datasinks
----ds_create
--dst_log_file = ds_open(ds_redo, XB_LOG_FILENAME, &stat_info);//open log file
--// 将lsn转换为Page NO.
--xtrabackup_choose_lsn_offset(checkpoint_lsn_start);
--xtrabackup_copy_logfile//从checkpoint_lsn_start开始拷贝redo log
----log_group_read_log_seg
------fil_io
----xtrabackup_scan_log_recs
--/* Populate fil_system with tablespaces to copy */
--xb_load_tablespaces
```











