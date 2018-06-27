SYSBENCH="sysbench  --test=oltp --num-threads=16 --max-requests=0 --max-time=1800 --db-driver=mysql --oltp-table-size=100000 --oltp-nontrx-mode=update_key --oltp-test-mode=nontrx --mysql-table-engine=innodb --mysql-db=test --mysql-user=mysql --mysql-socket=/home/mysql/mysql/tmp/mysql.sock"
rm -rf case_ext2_ssd_update_key_16_1800_100000
mkdir case_ext2_ssd_update_key_16_1800_100000
SYSBENCH_RUN="${SYSBENCH} run"
sh start-monitor.sh case_ext2_ssd_update_key_16_1800_100000 case_ext2_ssd_update_key_16_1800_100000
${SYSBENCH_RUN} |  tee case_ext2_ssd_update_key_16_1800_100000/case_ext2_ssd_update_key_16_1800_100000.data
sh stop-monitor.sh case_ext2_ssd_update_key_16_1800_100000 case_ext2_ssd_update_key_16_1800_100000
