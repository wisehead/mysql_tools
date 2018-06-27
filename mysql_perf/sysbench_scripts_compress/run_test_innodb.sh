#!/usr/bin/env bash
set -e
#set -x

PERF_TEST=
src_dir=`pwd`/..
sb_zip=${src_dir}/sysbench.zip
sb_dir=${src_dir}/sysbench-1.0.6
install_dir=/usr/local/share/sysbench
mysql_dir=/mnt/sfx-card-root/mysql_5623
mysql_bin=${mysql_dir}/bin/mysql

usage="Usage: ./run_test.sh [options]\n
Run Sysbench benchmark tests to demonstrate CSSZLIB advantage in MySQL transparent page compression.\n
The following tests will be executed in order.\n
\t1.    Sysbench OLTP Write-Only test(CSSZLIB)\n
\t2.    Sysbench OLTP Read-Only test(CSSZLIB)\n
\t3.    Sysbench OLTP Read-Write test(CSSZLIB)\n
\t4.    Sysbench OLTP Write-Only test(No Compression)\n
\t5.    Sysbench OLTP Read-Only test(No Compression)\n
\t6.    Sysbench OLTP Read-Write test(No Compression)\n
\t7.    Sysbench OLTP Write-Only test(Standard ZLIB)\n
\t8.    Sysbench OLTP Read-Only test with Standard ZLIB enabled\n
\t9.    Sysbench OLTP Read-Write test with Standard ZLIB enabled\n
Options:\n
\t--help        - Display how to use this script\n
\t--host        - host name or ip of MySQL Server, default to localhost.\n
\t--port        - port number of MySQL Server, default to 3306.\n
\t--rootpwd     - MySQL Server root password, default to empty string.\n
\t--tables      - number of tables to be created in Sysbench tests, default to 4.\n
\t--table-size  - size of each table in MB, default to 1024 MB.\n
\t--threads     - number of threads to be used in Sysbench tests, default to 16.\n
\t--time        - time in seconds that each Sysbench test wil run, default to 60 seconds.\n
\t--performance - use the configurations in my.cnf.performance if given.\n
"
HOSTNAME=localhost
PORT=3306
NUM_TABLES=25
TABLE_SIZE=2048
NUM_THREADS=16
TIME=300
#ROOTPWD="ypqRz_Y!6cp8zb^fY69M*ux!N!GW*C"
ROOTPWD="Av4h%ryp1!h=ZFe*^17)ht^e7^TMTs"

while test $# -gt 0
do
    case $1 in
        --help) echo -e $usage; exit 0 ;;
        --host) shift; HOSTNAME=$1 ;;
        --port) shift; PORT=$1 ;;
        --rootpwd) shift; ROOTPWD=$1 ;;
        --tables) shift; NUM_TABLES=$1 ;;
        --table-size) shift; TABLE_SIZE=$1 ;;
        --threads) shift; NUM_THREADS=$1 ;;
        --time) shift; TIME=$1 ;;
        --performance) PERF_TEST=$1 ;;
    esac
    shift
done

echo -e "Hostname: ${HOSTNAME}
Port: ${PORT}
Number of tables: ${NUM_TABLES}
Each table size: ${TABLE_SIZE} MB
Number of threads: ${NUM_THREADS}
Time: ${TIME} seconds
"
#TESTS="write read mix"
TESTS="write read mix"
COMPRESSORS="zlib nocompr"

function print_time {
    echo "["`date +%H:%M:%S`"]:$1"

}

run_test()
{
testname=$1
mode=$2

/usr/local/bin/sysbench --db-driver=mysql --mysql-host=$HOSTNAME --mysql-port=$PORT \
  --mysql-user=sbtest --mysql-password=sbtest $testname --tables=$NUM_TABLES --threads=$NUM_THREADS \
  --table-size=$((TABLE_SIZE*1024*1024/200)) --time=$TIME $mode
}

#if [ ! -d $sb_dir ]; then
if [  -d $sb_dir ]; then
    rm -rf sb_zip
    wget -O $sb_zip https://github.com/akopytov/sysbench/archive/1.0.6.zip
    unzip $sb_zip -d $src_dir
fi

if [ ! -d $install_dir ]; then
    (cd $sb_dir && ./autogen.sh && ./configure --with-mysql-includes=${mysql_dir}/include  --with-mysql-libs=${mysql_dir}/lib && make && make install)
fi

#service mysql restart
print_time "sleep 60s: BEGIN..."
sleep 60
print_time "sleep 60s: END..."

${mysql_bin} -h ${HOSTNAME} -P ${PORT} -uroot -p${ROOTPWD} -e "create database if not exists sbtest; grant all on sbtest.* to sbtest@'%' identified by 'sbtest'; flush privileges;"

for comp in $COMPRESSORS; do
    if [ $comp = 'zlib' ]; then
        cp oltp_common_innodb_zlib.lua $install_dir/oltp_common.lua
        echo ""
        echo "ZLIB:"
    else
        cp oltp_common_innodb_nocompr.lua $install_dir/oltp_common.lua
        echo ""
        echo "No Compression:"
    fi
    
    print_time "$comp test: BEGIN..."

    for test in $TESTS; do
        if [ $test = 'write' ]; then
            echo "Write-Only Test:"
            testname=${install_dir}/oltp_write_only.lua
        elif [ $test = 'read' ]; then
            echo "Read-Only Test:"
            testname=${install_dir}/oltp_read_only.lua
        else
            echo "Read-Write Test:"
            testname=${install_dir}/oltp_read_write.lua
        fi

        if test -z $PERF_TEST; then
            cp my.cnf.sanitycheck /etc/my.cnf
        else
            cp my.cnf.performance /etc/my.cnf
        fi

        print_time "begin cleanup data: START..."
        run_test $testname cleanup > /dev/null
        print_time "begin cleanup data: OVER..."

        print_time "prepare data: START..."
        run_test $testname prepare > /dev/null
        print_time "prepare data: OVER..."

        print_time "run [$test] test: START..."
        output=`echo -e "$(run_test $testname run)" | grep transactions`
        [[ $output =~ [[:digit:]]+\.[[:digit:]]+ ]] && echo "Transactions: ${BASH_REMATCH} per sec"
        print_time "run [$test] test: OVER..."

        print_time "count data size:"
        #${mysql_bin} -h $HOSTNAME -P ${PORT} -uroot -p${ROOTPWD} -e "select 'Database Size:' as 'db', \
        #concat(round(IFNULL(sum(ALLOCATED_SIZE)/1024/1024, 0), 2), ' MB') as length from information_schema.innodb_sys_tablespaces \
        #where name like '%sbtest%';" 2>/dev/null | grep "Database Size"

        ${mysql_bin} -h $HOSTNAME -P ${PORT} -uroot -p${ROOTPWD} -e "SELECT 'Database Size:' as 'db', \
        concat(round(IFNULL((sum(DATA_LENGTH)+sum(INDEX_LENGTH))/1024/1024, 0), 2), 'MB') as length \
        FROM information_schema.TABLES where TABLE_SCHEMA='sbtest';" 2>/dev/null | grep "Database Size"

        print_time "finish cleanup data: START..."
        run_test $testname cleanup > /dev/null
        print_time "finish cleanup data: OVER..."

        echo ""
        print_time "sleep 60s: ..."
	sleep 60
    done
done
