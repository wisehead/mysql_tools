#!/bin/bash
 
abc=('%')
 
echo "=================================================mysql========================================================"
echo "SET SQL_LOG_BIN=0;"
for i in `cat list`
do
database=`echo $i |awk -F"," '{print $3}'`
user=`echo $i |awk -F"," '{print $1}'`
passwd=`echo $i |awk -F"," '{print $2}'`
if [[ -z ${user} ]]
then
user=${database}"_w"
fi
if [[ -z ${passwd} ]]
then
passwd=${database}"_w123"
fi
echo "CREATE DATABASE \`"${database}"\`;"
for j in ${abc[*]}
do
echo "GRANT ALL on \`"${database}"\`.* to '"${user}"'@'"$j"' IDENTIFIED BY '"${passwd}"';"
done
done
 
echo "=================================================dbproxy========================================================"
 
for i in `cat list`
do
database=`echo $i |awk -F"," '{print $3}'`
user=`echo $i |awk -F"," '{print $1}'`
passwd=`echo $i |awk -F"," '{print $2}'`
if [[ -z ${user} ]]
then
user=${database}"_w"
fi
if [[ -z ${passwd} ]]
then
passwd=${database}"_w123"
fi
cat <<EOF 
[DB_User_tsc_0000_${user}]
db_username = ${user}
db_password = ${passwd}
cluster_name = tsc_0000
default_charset = utf8
default_db = ${database}
 
[Product_User_tsc_0000_${user}]
username = ${user}
password = ${passwd}
db_username = ${user}
max_connections = 200
authip_enable = 0
 
EOF
done
