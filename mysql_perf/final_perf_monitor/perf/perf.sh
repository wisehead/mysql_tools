#!/bin/bash

####### COLLECT SYSTEM STATS RELEVANT TO WORKLOAD/ DATABASE
#####
# usage:
# perf.sh start interval port user passwd
# perf.sh stop

function usage() {
	cat <<EOT
usage: sh perf.sh  {start|stop} interval host port user passwd
       (interval sampling period)
EOT
}
if [ $# -lt 1 ];then
	usage
	exit 1
fi
PWD=$(cd `dirname $0`;pwd)

if [ "$1" = "start" ] 
then
    host=$3
    port=$4
    user=$5
    passwd=$6

    if [ "$host" = "" -o "$port" = "" -o "$user" = "" ];then
	usage
	exit 1
    fi
	LOGDIR=${PWD}/../result
	rm -rf $LOGDIR
	mkdir -p $LOGDIR
	iostat -xkt $2 > $LOGDIR/iostat.out &
	vmstat $2 > $LOGDIR/vmstat.out &
	mpstat -P ALL $2 > $LOGDIR/mpstat.out &
	$PWD/innodbstats.sh $2 $host $port $user $passwd > $LOGDIR/innodb_status.out &
	$PWD/mysqlglobalstats.sh $2 $host $port $user $passwd > $LOGDIR/global_0.out &
elif [ "$1" = "stop" ] 
then
	pkill iostat
	pkill vmstat
	pkill mpstat
	pkill innodbstats
	#pkill mysqlglobalstats
	ps -ef|grep mysqlglobalstats|grep -v grep|awk '{print $2}'|xargs kill -9  >/dev/null 2>&1
else
	usage
	exit 1
fi

