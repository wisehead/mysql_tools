THREAD_NUM=$1
EXECUTE_TIME=$2
TABLE_SIZE=$3
SQL_MODE=$4

touch sysbench_prepare_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "SYSBENCH=\"sysbench  --test=oltp --num-threads=$THREAD_NUM --max-requests=0 --max-time=$EXECUTE_TIME --db-driver=mysql --oltp-table-size=${TABLE_SIZE} --oltp-nontrx-mode=${SQL_MODE} --oltp-test-mode=nontrx --mysql-table-engine=innodb --mysql-db=test --mysql-user=mysql --mysql-socket=/home/mysql/mysql/tmp/mysql.sock\"" >> sysbench_prepare_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh

echo "SYSBENCH_PREPARE=\"\${SYSBENCH} prepare\"" >> sysbench_prepare_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh
echo "\${SYSBENCH_PREPARE}" >> sysbench_prepare_${THREAD_NUM}_${EXECUTE_TIME}_${TABLE_SIZE}_${SQL_MODE}.sh
