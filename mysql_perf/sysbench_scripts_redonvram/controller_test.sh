FILESYSTEM=$1
DISKTYPE=$2
SQLMODE=$3
THREAD_NUM=$4
EXECUTE_TIME=$5
TABLE_SIZE=$6

#init DISK filesystem
rm -rf test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}

mkdir test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}

cp controller_init_disk.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}
cp controller_start_mysql.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}
cp controller_sysbench_prepare.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}
cp controller_sysbench_run.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}
cp controller_sysbench_cleanup.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}
cp start-monitor.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}
cp stop-monitor.sh  test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}

cd test_case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}

CASE_DIR="case_${FILESYSTEM}_${DISKTYPE}_${SQLMODE}_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}"

sh controller_init_disk.sh ${FILESYSTEM}

sh controller_start_mysql.sh ${DISKTYPE}

sh controller_sysbench_prepare.sh ${THREAD_NUM} ${EXECUTE_TIME} ${TABLE_SIZE} ${SQLMODE}

sh controller_sysbench_run.sh ${THREAD_NUM} ${EXECUTE_TIME} ${TABLE_SIZE} ${SQLMODE} ${CASE_DIR}

sh controller_sysbench_cleanup.sh ${THREAD_NUM} ${EXECUTE_TIME} ${TABLE_SIZE} ${SQLMODE}

		
