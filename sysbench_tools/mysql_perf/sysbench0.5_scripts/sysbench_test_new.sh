THREAD_NUM=$1
EXECUTE_TIME=$2
DISKTYPE="ssd nvram"
SQLMODE="insert update_key"
FILESYSTEM=$3
for disk in $DISKTYPE
	do
	for sql in $SQLMODE
		do
		echo "[`date '+%m%d-%H:%M:%S'`] test on ${disk} while condition is ${sql}..............................."
		#kill mysql
		echo "[`date '+%m%d-%H:%M:%S'`] kill mysql..............................................................."
		killall mysqld

		sleep 2
		echo "[`date '+%m%d-%H:%M:%S'`] clear data................................................................"
		#clear data
		rm -rf /ssd/mysql/var/*;rm -rf /root/mysql/var/*;rm -rf /nvram/mysql/var/*;rm -rf /home/mysql_log/*
		CASE_DIR="case_${disk}_${sql}_${THREAD_NUM}_${EXECUTE_TIME}_${FILESYSTEM}"
		rm -rf ${CASE_DIR}
		mkdir ${CASE_DIR}
		#init mysql and start mysql
		echo "[`date '+%m%d-%H:%M:%S'`] init mysql and start mysql................................................"
		/root/mysql/bin/mysql_install_db --defaults-file=/root/mysql/etc/my.cnf.nvme_${disk} --basedir=/root/mysql >> /dev/null
		
		nohup /root/mysql/libexec/mysqld --defaults-file=/root/mysql/etc/my.cnf.nvme_${disk} --user=root >> /dev/null &	
		sleep 5
		
		echo "[`date '+%m%d-%H:%M:%S'`] start sysbench test [${sql}]..........................................."
		#start nmon monitor
		./start-monitor.sh ${CASE_DIR} ${CASE_DIR}
		#start sysbench test
		SYSBENCH="sysbench  --test=oltp --num-threads=$THREAD_NUM --max-requests=0 --max-time=$EXECUTE_TIME --db-driver=mysql --oltp-table-size=1500000 --oltp-nontrx-mode=${sql} --oltp-test-mode=nontrx --mysql-table-engine=innodb --mysql-db=test --mysql-user=root --mysql-socket=/root/mysql/tmp/mysql.sock"
		SYSBENCH_PREPAR="${SYSBENCH} prepare"
		${SYSBENCH_PREPAR}
		SYSBENCH_RUN="${SYSBENCH} run"
		${SYSBENCH_RUN} | tee case_${disk}_${sql}_${THREAD_NUM}_${EXECUTE_TIME}_${FILESYSTEM}/case_${disk}_${sql}_${THREAD_NUM}_${EXECUTE_TIME}_${FILESYSTEM}.data
		SYSBENCH_CLEANUP="${SYSBENCH} cleanup"
		${SYSBENCH_CLEANUP}
		#stop nmon moitor
		./stop-monitor.sh ${CASE_DIR} ${CASE_DIR}

		echo "[`date '+%m%d-%H:%M:%S'`] sysbench test done [${sql}].............................................."
		done

	done
