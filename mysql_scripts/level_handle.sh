./bin/mysqldump --defaults-file=etc/user.root.cnf level > level.sql

./bin/mysql --defaults-file=etc/user.root.cnf -e "select TABLE_NAME from INFORMATION_SCHEMA.TABLES where TABLE_SCHEMA='level'" > table_list

sed -i '1,1d' table_list

for i in `cat table_list`;do ./bin/mysql --defaults-file=etc/user.root.cnf -e "select COLUMN_NAME from INFORMATION_SCHEMA.STATISTICS where TABLE_SCHEMA='level' and INDEX_NAME='PRIMARY' and TABLE_NAME='"$i"'" > tables_info/$i.info;done

for i in `cat table_list`;do sed -i '1,1d' tables_info/$i.info;done

for i in `cat table_list`;do ./bin/mysql --defaults-file=etc/user.root.cnf -D level -e "alter table "$i" DROP PRIMARY KEY";done

for i in `cat table_list`;do echo $i;./bin/mysql --defaults-file=etc/user.root.cnf -D level -e "ALTER TABLE "$i" ADD column INCREMENT_ID bigint(20) unsigned NOT NULL AUTO_INCREMENT PRIMARY KEY first";done

for j in `cat table_list`;do l=`cat tables_info/$j.info|wc -l`;if [[ $l -eq 0 ]];then continue;fi;temp='';for k in `cat tables_info/$j.info`;do temp=$temp','$k;done;echo "ALTER TABLE $j ADD UNIQUE (${temp#*,});";done > add_unique_key.sql

./bin/mysql --defaults-file=etc/user.root.cnf -D level < add_unique_key.sql