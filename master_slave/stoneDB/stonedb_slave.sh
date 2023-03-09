#!/bin/bash

echo "启动该脚本前需要检查以下环境："
echo "1.主从库网络是否联通"
echo "2.从库备份目录磁盘空间是否充足"
echo "3.主从库是否正常启动"
echo "4.主从库是否开启GTID模式"
echo "5.主库是否创建复制用户"
read -p "以上条件是否满足？ 输入y，继续，输入n，退出.:" run_status

if [ "$run_status" != "y" ]; then
  exit -1
fi


set -x

read -p "1、请输入StoneDB从库的安装目录:" INSTALL_PATH
read -p "2、请输入主库IP:" MASTER_IP
read -p "3、请输入主库数据库端口:" MASTER_PORT
read -p "4、请输入主库用户:" MASTER_USER
read -p "5、请输入主库用户密码:" MASTER_USER_PASSWORD
read -p "6、请输入StoneDB从库用户:" SLAVE_USER
read -p "7、请输入StoneDB从库用户密码:" SLAVE_USER_PASSWORD
read -p "8、请输入主库的复制用户:" REPLICATION_USER
read -p "9、请输入主库的复制用户密码:" REPLICATION_USER_PAPASSWORD
read -p "10、请输入导出目录:" EXPORT_PATH


#检查主库
function check_master() {
  mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show databases;"
  if [ "$?" -ne 0 ]; then 
   echo "主库连接不通，请检查主库状态或者用户名密码是否正确"
   exit 1; 
  fi
}

#检查从库
function check_slave() {
  mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "show databases;"
  if [ "$?" -ne 0 ]; then
   echo "从库连接不通，请检查从库状态或者用户名密码是否正确"
   exit 1;
  fi
}

#检查主库是否开启GTID模式
function check_whether_GTID() {
  gtid_mode_status=$(mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show variables like 'gtid_mode';" | grep -E 'ON' | wc -l)
  e_gtid_status=$(mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show variables like 'enforce_gtid_consistency';" | grep -E 'ON' | wc -l)
  if [[ $gtid_mode_status -eq 0 ]] || [[ $e_gtid_status -eq 0 ]]; then
	echo "在主库执行发生错误，或者是主库未开启GTID，请先在主库开启GTID模式"
	exit 0
  fi
}

#从库安装半同步插件，首次执行时放开，如需重复执行需要注释该调用
function install_plug() {
  rpl_status=$(mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "select plugin_name,plugin_status from information_schema.plugins where plugin_name like '%rpl_semi_sync_slave%';"| grep -E '*rpl_semi_sync_slave*' | wc -l)
  if [ $rpl_status -eq 0 ];then
	mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "INSTALL PLUGIN rpl_semi_sync_slave SONAME 'semisync_slave.so';"
  fi
}

#主从配置参数，首次执行时放开，如需重复执行需要注释该调用
function change_my_cnf() {
cat <<"EOF" >>/tmp/stonedb-slave
# enable GTID
gtid_mode = on
enforce_gtid_consistency = 1
#Enhanced semi-synchronization
loose-plugin-load = "rpl_semi_sync_slave=semisync_slave.so"
loose-rpl-semi-sync-slave-enabled = 1
loose_rpl_semi_sync_master_timeout=1000
loose-rpl_semi_sync_master_wait_point=AFTER_SYNC
binlog-ignore-db=information_schema
binlog-ignore-db=performance_schema
binlog-ignore-db=sys
sync_binlog=1
innodb_flush_log_at_trx_commit=1
slave-parallel-type=LOGICAL_CLOCK
slave_parallel_workers=8
master_info_repository=TABLE
relay_log_info_repository=TABLE
relay_log_recovery=ON
log_slave_updates=ON
read_only = 1
slave_type_conversions='ALL_NON_LOSSY,ALL_LOSSY'
sql_mode='STRICT_TRANS_TABLES,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,MANDATORY_TIANMU'
EOF

  # because my.cnf may mount from docker outside, sed may error of ` Device or resource busy`
  cp $INSTALL_PATH/my.cnf $INSTALL_PATH/my.cnf2
  sed -i 's/server-id = 1/server-id = 10360/g' $INSTALL_PATH/my.cnf2
  sed -i "/server-id = 10360/r /tmp/stonedb-slave" $INSTALL_PATH/my.cnf2
  cat $INSTALL_PATH/my.cnf2 > $INSTALL_PATH/my.cnf
  rm -rf $INSTALL_PATH/my.cnf2

  cat $INSTALL_PATH/my.cnf

  chown -R mysql:mysql $INSTALL_PATH

  ls -al $INSTALL_PATH/

  echo "restart\n"
  $INSTALL_PATH/mysql_server restart
}

#主库导出数据，默认导出除系统库以外的其他所有数据库
function export_all_database_to_slave() { 
  mysql -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD -e "show databases" > $EXPORT_PATH/$(date '+%Y%m%d').txt
  grep -Ev "^Database$|^performance_schema$|^information_schema$|^mysql$|^sys$" $EXPORT_PATH/$(date '+%Y%m%d').txt  |xargs mysqldump -h$MASTER_IP -u$MASTER_USER -p$MASTER_USER_PASSWORD --set-gtid-purged=on --single-transaction --skip-add-locks --log-error=/tmp/export_$(date '+%Y%m%d').log --add-drop-database --databases > $EXPORT_PATH/stonedb-slave-db_bak$(date '+%Y%m%d').sql
  if [ "$?" -ne 0 ]; then
   error_status=$(cat /tmp/export_$(date '+%Y%m%d').log | grep "Got errno 28" | wc -l)
    if [[ $error_status -eq 1 ]]; then
        echo "磁盘空间爆满"
        exit 0
    fi
   exit 1;
  fi
}

#转换导出的备份文件
#1.增加 MANDATORY_TIANMU 配置，会对导入的数据表的引擎强制转换为 TIANMU
#2.在 SET @@GLOBAL.GTID_PURGED 前增加 reset master，防止GTID报错
#3.decimal 精度大于18的转换为18
function export_data_format_conversion() {
  sed -i "s/SQL_MODE='/SQL_MODE='MANDATORY_TIANMU,/g" $EXPORT_PATH/stonedb-slave-db_bak$(date '+%Y%m%d').sql
  sed -i "s/SET @@GLOBAL.GTID_PURGED=/reset master; SET @@GLOBAL.GTID_PURGED=/g" $EXPORT_PATH/stonedb-slave-db_bak$(date '+%Y%m%d').sql
  sed -i 's/decimal(19\|decimal(2[0-9]\|decimal(3[0-9]\|decimal(4[0-9]\|decimal(5[0-9]\|decimal(6[0-9]\|DECIMAL(19\|DECIMAL(2[0-9]\|DECIMAL(3[0-9]\|DECIMAL(4[0-9]\|DECIMAL(5[0-9]\|DECIMAL(6[0-9]/decimal(18/g' $EXPORT_PATH/stonedb-slave-db_bak$(date '+%Y%m%d').sql
}

#导入数据
function import_all_database_to_slave() {
  mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD -e "stop slave;reset slave all;reset master;"
  mysql -h127.0.0.1 -u$SLAVE_USER -p$SLAVE_USER_PASSWORD < $EXPORT_PATH/stonedb-slave-db_bak$(date '+%Y%m%d').sql
}

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

check_master
check_slave
check_whether_GTID
#install_plug
#change_my_cnf
export_all_database_to_slave
export_data_format_conversion
import_all_database_to_slave
change_sync_master
