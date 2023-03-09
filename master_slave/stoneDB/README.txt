一、脚本文件说明
	1）stonedb_slave.sh 一键部署脚本
	2）stonedb_slave_save_log.sh 一键部署脚本日志版，会记录执行过程中生产的日志，便于出现问题时进行排查，执行该脚本时必须确保该脚本和stonedb_slave.sh在同一目录下，产生的日志文件在/tmp/目录下，以stonedb-slave-log+日期的.log文件


二、使用stonedb_slave.sh脚本一键部署StoneDB从库，主库需要具备以下前置条件：
	1、是否安装半同步插件
	如果主库安装了半同步插件，则从库也需要安装半同步插件，否则从库无需安装半同步插件。
	主库检查是否安装半同步插件命令：
	show status like 'rpl_semi_sync_master_status';
	如果返回ON，说明是半同步模式，从库需要安装半同步插件，"install_plug"即是安装步骤。
	如果返回OFF，说明是异步模式，从库无需安装半同步插件，注释"install_plug"即可。

	2、是否开启GTID模式（建议开启）
	如果主库开启了GTID模式，则从库也需要开启GTID模式，否则从库无需开启GTID模式。
	主库检查是否开启GTID模式命令：
	show variables like 'gtid_mode';
	show variables like 'enforce_gtid_consistency';
	如果返回都为ON，说明是GTID模式，从库需要开启GTID模式。
	如果返回为非ON，说明是非GTID模式，从库无需开启GTID模式。
	从库开启GTID模式步骤：
	1）将以下参数加入从库的参数文件
	# enable GTID
	gtid_mode = on
	enforce_gtid_consistency = 1
	2）重启从库

	3、主库是否需要创建复制用户
	如果现有环境已经是主从复制关系，现在想把StoneDB作为从库加入这个主从复制架构中，则主库无需再创建复制用户。
	如果现有环境是非主从复制关系，则主库需要创建复制用户。
	主库创建复制用户命令：
	create user 'repl'@'%' identified by 'xxx';
	grant replication slave on *.* to 'repl'@'%';
	
	4、从库参数配置（建议手动配置）
	由于StoneDB有参数模板，stonedb_slave.sh也有部分参数，且参数不具备普遍性，建议手动配置从库的参数，stonedb_slave.sh默认也注释了change_my_cnf。
	注：slave_type_conversions='ALL_NON_LOSSY,ALL_LOSSY'，该参数必须在从库StoneDB上有。
    
	5、主库导出
	默认导出除系统库以外的其他所有数据库，用户可根据实际需求修改脚本，只导出指定库。


三、ERROR排查
    若运行脚本过程中发生了错误，则会退出，我们需要根据错误去排查分析原因。
	1、需要查看的日志
	$INSTALL_PATH/log/mysqld.log
	$INSTALL_PATH/log/tianmu.log
	2、ERROR具体报错信息
	$INSTALL_PATH/bin/perror error xxx
	示例如下：
	/stonedb57/install/bin/perror error 1032
    OS error code   0:  Success
    MySQL error code 1032 (ER_KEY_NOT_FOUND): Can't find record in '%-.192s'


四、传参说明
	执行脚本需要传入10个参数，这10个参数分别是
	1）StoneDB从库安装目录 
	2）主库IP 
	3）主库数据库端口
	4）主库用户
	5）主要用户密码
	6）StoneDB从库用户
	7）StoneDB从库用户密码
	8）复制用户
	9）复制用户密码
	10）导出目录
	注：传参中需要输入主库和从库的用户和密码，这个用户必须具备所有库的读写权限。为了防止磁盘空间爆满而未产生告警信息，导出目录与导出日志目录不应该在同一个目录下。