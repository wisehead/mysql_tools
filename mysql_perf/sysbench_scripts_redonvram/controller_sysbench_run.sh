THREAD_NUM=$1
EXECUTE_TIME=$2
TABLE_SIZE=$3
SQL_MODE=$4
CASE_DIR=$5

touch sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "SYSBENCH=\"sysbench  --test=oltp --num-threads=$THREAD_NUM --max-requests=0 --max-time=$EXECUTE_TIME --db-driver=mysql --oltp-table-size=${TABLE_SIZE} --oltp-nontrx-mode=${SQL_MODE} --oltp-test-mode=nontrx --mysql-table-engine=innodb --mysql-db=test --mysql-user=mysql --mysql-socket=/home/mysql/mysql/tmp/mysql.sock\"" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "rm -rf ${CASE_DIR}" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "mkdir ${CASE_DIR}" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "SYSBENCH_RUN=\"\${SYSBENCH} run\"" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "sh start-monitor.sh ${CASE_DIR} ${CASE_DIR}" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "\${SYSBENCH_RUN} |  tee ${CASE_DIR}/${CASE_DIR}.data" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "sh stop-monitor.sh ${CASE_DIR} ${CASE_DIR}" >> sysbench_run_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh
