#############################################################
#   File Name: create_table.sh
#     Autohor: Hui Chen (c) 2017
#        Mail: chenhui13@baidu.com
# Create Time: 2017/09/26-18:01:57
#############################################################
#!/bin/sh 
#for i in `cat table_list`;
int=1
#mysql -uroot -proot -e "use hi_db";
while(( $int<=1024 ))
do 
    #mysql -uroot -proot -e "BLE_SCHEMA='ensemble' and INDEX_NAME='PRIMARY' and TABLE_NAME='"$i"'" > tables_info/$i.info;
    echo $int
    mysql -uroot -proot -e "use hi_db;CREATE TABLE sbtest$int ( id int(10) unsigned NOT NULL AUTO_INCREMENT, k int(10) unsigned NOT NULL DEFAULT '0', c varchar(120) NOT NULL DEFAULT '', pad char(60) NOT NULL DEFAULT '', PRIMARY KEY (id), KEY k_1 (k)) ENGINE=InnoDB AUTO_INCREMENT=14972318 DEFAULT CHARSET=utf8 MAX_ROWS=1000000 KEY_BLOCK_SIZE=8"
    let "int++"
done
