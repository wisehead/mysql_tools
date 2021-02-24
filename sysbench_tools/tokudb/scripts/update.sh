#!/bin/bash

table_size=1000000
table_count=1
user=chenhui
password=1234
host=127.0.0.1
port=10299
threads=32
test_time=300

./sysbench --test=lua/update_index.lua --mysql-table-engine=tokudb --mysql-db=hi \
--oltp-table-size=$table_size --oltp-tables-count=$table_count --mysql-user=$user --mysql-password=$password \
--report-interval=5 --mysql-port=$port --num-threads=$threads --mysql-host=$host --max-time=$test_time --max-requests=0  \
--db-driver=mysql run > out.txt

