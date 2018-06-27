FILESYSTEM=$1
#init DISK filesystem
killall mysqld

sh init_disk.sh $FILESYSTEM

mkdir /ssd/mysql
mkdir /ssd/mysql/mysql
mkdir /ssd/mysql/mysql/var
mkdir /nvram/mysql
mkdir /nvram/mysql/mysql
mkdir /nvram/mysql/mysql/var

#start generate test file

DISKTYPE="ssd nvram"
SQLMODE="insert update_key"

for DISK in $DISKTYPE
	do
	for SQL in $SQLMODE
	do
	rm -rf test_case_${DISK}_${SQL}

	mkdir test_case_${DISK}_${SQL}
	
	cp start_mysql.sh test_case_${DISK}_${SQL}/

	cp clear_data.sh test_case_${DISK}_${SQL}/

	cp make_dir.sh  test_case_${DISK}_${SQL}/

	cp sysbench_test.sh test_case_${DISK}_${SQL}/

	cp start-monitor.sh test_case_${DISK}_${SQL}/
	
	cp stop-monitor.sh test_case_${DISK}_${SQL}/

	cd test_case_${DISK}_${SQL}

	touch test_case_${DISK}_${SQL}.sh
	
	echo "THREAD_NUM=\$1" >> test_case_${DISK}_${SQL}.sh

	echo "EXECUTE_TIME=\$2" >> test_case_${DISK}_${SQL}.sh

	echo "killall mysqld" >> test_case_${DISK}_${SQL}.sh

	echo "sh clear_data.sh" >> test_case_${DISK}_${SQL}.sh
		
	echo "CASE_DIR=\"case_${DISK}_${SQL}_\${THREAD_NUM}_\${EXECUTE_TIME}_${FILESYSTEM}\"" >> test_case_${DISK}_${SQL}.sh

	echo "sh make_dir.sh \${CASE_DIR}" >> test_case_${DISK}_${SQL}.sh

	echo "sh start_mysql.sh ${DISK}" >> test_case_${DISK}_${SQL}.sh

	echo "sleep 5" >> test_case_${DISK}_${SQL}.sh

	echo "sh start-monitor.sh \${CASE_DIR} \${CASE_DIR}" >> test_case_${DISK}_${SQL}.sh
	echo "sh sysbench_test.sh \${THREAD_NUM} \${EXECUTE_TIME} ${SQL} \${CASE_DIR}" >> test_case_${DISK}_${SQL}.sh
	echo "sh stop-monitor.sh \${CASE_DIR} \${CASE_DIR}" >> test_case_${DISK}_${SQL}.sh
	
	cd ..

	done
done
	

		
