#############################################################
#   File Name: create_dir.sh
#     Autohor: Hui Chen (c) 2023
#        Mail: chenhui13@xxx.com
# Create Time: 2023/03/01-16:49:08
#############################################################
#!/bin/sh 

rm -fr data
rm -fr binlog
rm -fr log
rm -fr tmp
rm -rf redolog
rm -rf undolog

mkdir data
mkdir binlog
mkdir log
mkdir tmp
mkdir redolog
mkdir undolog

#/data/chenhui/install/bin/mysqld --defaults-file=/data/chenhui/install/my.cnf --initialize --user=mysql
