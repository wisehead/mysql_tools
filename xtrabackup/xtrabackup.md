#一、xtrabackup_backup_func

backup mode

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

