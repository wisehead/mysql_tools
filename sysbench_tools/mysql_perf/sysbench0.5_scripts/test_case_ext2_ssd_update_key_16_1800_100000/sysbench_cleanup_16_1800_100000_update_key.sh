SYSBENCH="sysbench  --test=oltp --num-threads=16 --max-requests=0 --max-time=1800 --db-driver=mysql --oltp-table-size=100000 --oltp-nontrx-mode=update_key --oltp-test-mode=nontrx --mysql-table-engine=innodb --mysql-db=test --mysql-user=mysql --mysql-socket=/home/mysql/mysql/tmp/mysql.sock"
SYSBENCH_CLEANUP="${SYSBENCH} cleanup"
${SYSBENCH_CLEANUP}
