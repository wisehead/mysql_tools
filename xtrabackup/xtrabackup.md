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
----os_thread_create(io_handler_thread
------fil_aio_wait
----SysTablespace::check_file_spec
----Tablespace::open_or_create//Open or Create the data files if they do not exist.
------Datafile::open_or_create
--------os_file_create
------fil_space_create//Create a space memory object and put it to the fil_system hash table.
------fil_node_create//Appends a new file to the chain of files of a space. File must be closed.
--------fil_node_create_low
----srv_undo_tablespaces_init/* Add separate undo tablespaces to fil_system */
----xb_load_single_table_tablespaces
--mdl_lock_init
----xb_mysql_connect
----xb_mysql_query(mdl_con, "BEGIN", false, true);
//=======================TokuDB Part=========================
--lock_tokudb_tables
----xb_mysql_query(connection,"SET TOKUDB_CHECKPOINT_LOCK = ON", false);
--os_thread_create(data_copy_thread_func//for innodb
--
```

#3.tokudb hot-backup Process

```
    /*
                        TokuDB Hot-Backup
                        =================
    We'll describe how Xtrabackup support TokuDB Hot-Backup?
    Method Xtrabackup backups up InnoDB engine has specificity. Backups up tokudb works as follow:

        STEP 1. SET TOKUDB_CHECKPOINT_LOCK=ON;
            prevents dirty pages in cachetable flushing to disk
            so log files will rise until STEP 3

        STEP 2. Copy tokudb data files(*.tokudb)
            after set TOKUDB_CHECKPOINT_LOCK on, data files won't change any more, but write operation continues(log files still increase)

        STEP 3. FLUSH TABLES WITH READ LOCK;
            prevents write operation, so log files won't change any more

        STEP 4. Copy tokudb log files(*.tokulog)

        STEP 5. UNLOCK TABLES

        STEP 6. SET TOKUDB_CHECKPOINT_LOCK=OFF
    */
```

#4.tokudb part for backup

```cpp
//=======================TokuDB Part=========================
xtrabackup_backup_func
--has_tokudb_plugin
----query = "SELECT COUNT(*) FROM information_schema.plugins WHERE plugin_name='TokuDB'"
----xb_mysql_query//?
----mysql_fetch_row
...
//other innodb related code.
...
    /*
    === TokuDB Hot-Backup ===
    STEP 1：
    SET TOKUDB_CHECKPOINT_LOCK=ON, Lock TokuDB tables
    */
--lock_tokudb_tables
----xb_mysql_query(connection,"SET TOKUDB_CHECKPOINT_LOCK = ON", false);
    
	/*
	=== TokuDB Hot-Backup ===
	STEP 2：
	Copy tokudb data files(*.tokudb)
	*/
    
--os_thread_create(data_copy_thread_func//for innodb
----data_copy_thread_func
------my_thread_init
------datadir_node_init
------copy_file//?????
------my_thread_end
------os_thread_exit
    /*
    === TokuDB Hot-Backup ===
    STEP 3：
    FLUSH TABLES WITH READ LOCK;

    Backup non-InnoDB files, e.g frm, MYD, MYI ...
    we also backup RocksDB in backu_start()
    and We will execute "FLUSH TABLE WITH READ LOCK" here to prevent DDL */
--backup_start
----backup_files
------datadir_node_init
------datafile_rsync_backup
    /*
    === TokuDB Hot-Backup ===
    STEP 4：
    Copy tokudb log files(*.tokulog)
    */
--os_thread_create(tokudb_log_copying_thread）
----tokudb_log_copying_thread
------my_thread_init
------datadir_node_init
------filename_matches_regex
------copy_file
------my_thread_end();
------os_thread_exit();

```










