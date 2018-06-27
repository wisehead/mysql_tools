THREAD_NUM=$1
EXECUTE_TIME=$2
SQL=$3
CASE_DIR=$4

SYSBENCH="sysbench  --test=oltp --num-threads=$THREAD_NUM --max-requests=0 --max-time=$EXECUTE_TIME --db-driver=mysql --oltp-table-size=1500000 --oltp-nontrx-mode=${SQL} --oltp-test-mode=nontrx --mysql-table-engine=innodb --mysql-db=test --mysql-user=root --mysql-socket=/root/mysql/tmp/mysql.sock"
echo "[`date '+%m%d-%H:%M:%S'`] sysbench_${THREAD_NUM}_${EXECUTE_TIME}_${SQL} prepare............................."
SYSBENCH_PREPAR="${SYSBENCH} prepare"
${SYSBENCH_PREPAR}
echo "[`date '+%m%d-%H:%M:%S'`] sysbench_${THREAD_NUM}_${EXECUTE_TIME}_${SQL} run................................."
SYSBENCH_RUN="${SYSBENCH} run"
${SYSBENCH_RUN} | tee ${CASE_DIR}/${CASE_DIR}.data
echo "[`date '+%m%d-%H:%M:%S'`] sysbench_${THREAD_NUM}_${EXECUTE_TIME}_${SQL} cleanup............................."
SYSBENCH_CLEANUP="${SYSBENCH} cleanup"
${SYSBENCH_CLEANUP}
