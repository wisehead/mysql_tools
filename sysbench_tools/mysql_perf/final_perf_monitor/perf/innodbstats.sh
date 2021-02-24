#!/bin/bash
PWD=$(cd `dirname $0`;pwd)

#####
# usage:
# innodbstats.sh interval host port user passwd


SLEEP_TIME=$1

if [ -n "$5" ];then
    pwd_flag="-p$5"
fi

while true
do
	mysql -h $2 -P $3 -u$4 $pwd_flag -e "SHOW ENGINE INNODB STATUS \G"
	sleep $SLEEP_TIME
done
