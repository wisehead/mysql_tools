#############################################################
#   File Name: run.sh
#     Autohor: Hui Chen (c) 2022
#        Mail: alex.chenhui@gmail.com
# Create Time: 2022/12/27-20:37:49
#############################################################
#!/bin/sh 
mysqldump -h$MASTER_IP -uroot -p$MYSQL_ROOT_PASSWORD --master-data=2 --force --all-databases >/tmp/stonedb-slave-db_bak$(date '+%Y%m%d').sql

mysqldump -h127.0.0.1 -uroot -pmysql123 --master-data=2 --force --all-databases >test.sql
mysqldump -h127.0.0.1 -uroot -pmysql123 --master-data=2  --all-databases >test.sql

mysqldump -h127.0.0.1 -uroot -pmysql123 --no-data --force --all-databases >ddl.sql
mysqldump -h127.0.0.1 -uroot -pmysql123 --no-data --all-databases >ddl.sql

mysql -uroot -pmysql123 -e "show databases"|grep -Ev "Database|information_schema|mysql|db|mysql" >1.txt
grep -Ev "performance_schema|information_schema|mysql|db|mysql|sys" 1.txt |xargs mysqldump -h127.0.0.1 -uroot -pmysql123 --master-data=2 --databases > mysql_dump.sql

mysql -uroot -pmysql123 -e "show databases"|grep -Ev "performance_schema|information_schema|mysql|db|mysql|sys" 1.txt |xargs mysqldump -h127.0.0.1 -uroot -pmysql123 --master-data=2 --skip-add-locks --databases > test.sql

mysql -uroot -pmysql123 -e "show databases" >1.txt
grep -Ev "Database|performance_schema|information_schema|mysql|db|mysql|sys" 1.txt  |xargs mysqldump -h127.0.0.1 -uroot -pmysql123 --master-data=2 --skip-add-locks --databases > test.sql
