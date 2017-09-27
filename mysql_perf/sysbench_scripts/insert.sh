#!/bin/bash

table_size=10000000000000000
table_count=1
user=mars_w
password=mars_w
host=10.3.2.37
port=4052
threads=32
test_time=300

./sysbench --test=lua/insert.lua  --mysql-table-engine=innodb \
--oltp-table-size=$table_size --oltp-tables-count=$table_count --mysql-user=$user --mysql-password=$password --oltp-auto-inc=off \
--report-interval=5 --mysql-port=$port --num-threads=$threads --mysql-host=$host --max-time=$test_time --max-requests=0 run > insert.txt
