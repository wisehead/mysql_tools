1.check_master

```
function check_master() {
  mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show databases;"
  if [ "$?" -ne 0 ]; then 
   echo "主库连接不通，请检查主库状态或者用户名密码是否正确"
   exit 1; 
  fi
}
mysql -h127.0.0.1 -P3306 -uroot -pxxx -e "show databases"
```

2.check_slave

```
function check_slave() {
  mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "show databases;"
  if [ "$?" -ne 0 ]; then
   echo "从库连接不通，请检查从库状态或者用户名密码是否正确"
   exit 1;
  fi
}

mysql -h127.0.0.1 -P3307 -uroot -pxxx -e "show databases"
```

3.检查主库GTID

```
#检查主库是否开启GTID模式
function check_whether_GTID() {
  gtid_mode_status=$(mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show variables like 'gtid_mode';" | grep -E 'ON' | wc -l)
  e_gtid_status=$(mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show variables like 'enforce_gtid_consistency';" | grep -E 'ON' | wc -l)
  if [[ $gtid_mode_status -eq 0 ]] || [[ $e_gtid_status -eq 0 ]]; then
	echo "在主库执行发生错误，或者是主库未开启GTID，请先在主库开启GTID模式"
	exit 0
  fi
}

mysql -h127.0.0.1 -P3306 -uroot -pxxx -e "show variables like 'gtid_mode';" | grep -E 'ON' | wc -l
mysql -h127.0.0.1 -P3306 -uroot -pxxx -e "show variables like 'enforce_gtid_consistency';" | grep -E 'ON' | wc -l

1）将以下参数加入从库的参数文件
	# enable GTID
	gtid_mode = on
	enforce_gtid_consistency = 1
```

4.主库创建同步账号

```
create user 'repl'@'%' identified by 'xxx';
grant replication slave on *.* to 'repl'@'%';
```

5.配置从库my.cnf

```
slave_type_conversions='ALL_NON_LOSSY,ALL_LOSSY'
```

6.change master

```
#与主库建立主从关系，并检查主从状态
function change_sync_master() {
  mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "change master to master_host='$MASTER_IP',master_port=$MASTER_PORT,master_user='$REPLICATION_USER',master_password='$REPLICATION_USER_PAPASSWORD',master_auto_position=1;start slave;"

  slave_status=$(mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "show slave status\G" | grep -E 'Slave.*Running: Yes' | wc -l)
  if [ $slave_status -lt 2 ]; then
    echo "StoneDB Master Replication setting fail"
  else
    echo "StoneDB Master Replication setting successfully"
  fi
}

mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "change master to master_host='$MASTER_IP',master_port=$MASTER_PORT,master_user='$REPLICATION_USER',master_password='$REPLICATION_USER_PAPASSWORD',master_auto_position=1;start slave;"
mysql -h127.0.0.1 -P3307 -uroot -pxxx -e "change master to master_host='127.0.0.1',master_port=3306,master_user='repl',master_password='xxx',master_auto_position=1;start slave;"

change master to master_host='127.0.0.1',master_port=3306,master_user='repl',master_password='xxx',master_auto_position=1;

start slave;
```

7.error

```
Last_IO_Error: Fatal error: The slave I/O thread stops because master and slave have equal MySQL server ids; these ids must be different for replication to work (or the --replicate-same-server-id option must be used on slave but this does not always make sense; please check the manual before using it).
修改  my.cnf server_id改一下，重启。
```
