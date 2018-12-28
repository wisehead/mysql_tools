#!/bin/sh
#author  alex.chenhui@gmail.com 
#date    2015-12-12 11:20

export LD_LIBRARY_PATH=

cur_dir=$(pwd)
case $# in
   2)
                target_path=$1
                port=$2
   ;;
   *)
        echo "Usage:sh $0 target_path port"
        echo "please give 2 parameters"
        exit 1
   ;;
esac

function log()
{    
        local epath=$(pwd)
        local timestamp=$(date +%Y%m%d-%H:%M:%S)
        #echo "#######################################################################"    
        echo "[$timestamp][$epath]$1"  
}
function do_cmd()
{    
        log "[exec] $*   please wait............"
        if [ $debug -eq 1 ];then        
                $@    
        else        
                $@ >/dev/null 2>&1    
        fi
        if [[ $? -ne 0 ]];then
                log "[fail] $@" 
                exit 1 
        else
                log "[succ] $@"
        fi
}

debug=1
bpath=$target_path

sec=$(date +%S)
server_id=$(ping -c 1 `hostname`|head -n 1|awk '{print $3}'|sed -r 's/[().\s]//g')
server_id="${server_id}${sec}"
server_id=$(($server_id%4294967295))

#tar zxvf mysql-5163.tar.gz

src_path="mysql-5163"

ps -ef|grep $bpath/|grep -v grep|awk '{print $2}'|xargs kill -9 >/dev/null 2>&1

files=`ls $bpath`
if [ ! -z "$files" ]; then
        echo "target path $bpath is not empty."
        exit 1
fi

if [ ! -d $bpath ];then
        do_cmd mkdir -p $bpath
fi

function generate_pwd()
{
        #local MATRIX="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~!@#$%^&*()_+="
        local MATRIX="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~!@%^&*()_+="
        local LENGTH="30"
        local PASS=""
        while [ "${n:=1}" -le "$LENGTH" ]
        do
                PASS="$PASS${MATRIX:$(($RANDOM%${#MATRIX})):1}"
                let n+=1
        done
        echo "$PASS"
        exit 0
}
function generate_sql_cnf()
{
        local bpath=$1
        local port=$2
        local server_id=$3
        local sql_file=$4

        local root_pwd=$(generate_pwd)
        local admin_pwd=$(generate_pwd)
    
cat >$sql_file <<_EOF_
SET SQL_LOG_BIN=0;

-- clear mysql-bin.00000* create by mysql_install_db
-- RESET MASTER;

-- clear dirty users
DELETE FROM mysql.user WHERE user='';
DELETE FROM mysql.db   WHERE user='';
DELETE FROM mysql.user WHERE host LIKE '%-%';
DELETE FROM mysql.db   WHERE host LIKE '%-%';

-- change password for root
UPDATE mysql.user SET password=PASSWORD('${root_pwd}') WHERE user='root';
UPDATE mysql.user SET password=PASSWORD('${admin_pwd}') WHERE user='admin';

-- create admin users
GRANT SELECT,RELOAD,PROCESS,SHOW DATABASES,SUPER,LOCK TABLES,REPLICATION CLIENT,TABLE MAINTENANCE,TRUNCATE TABLE ON *.* TO 'admin'@'localhost' IDENTIFIED BY '${admin_pwd}' WITH GRANT OPTION;
GRANT SELECT,RELOAD,PROCESS,SHOW DATABASES,SUPER,LOCK TABLES,REPLICATION CLIENT,TABLE MAINTENANCE,TRUNCATE TABLE ON *.* TO 'admin'@'127.0.0.1' IDENTIFIED BY '${admin_pwd}' WITH GRANT OPTION;
GRANT REPLICATION CLIENT,REPLICATION SLAVE ON *.* TO 'mysqlsync'@'' IDENTIFIED BY 'mysqlsync' WITH GRANT OPTION;
-- GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '${root_pwd}';

-- reset privileges and replication status;
flush privileges;
-- reset master;
reset slave;
_EOF_

log "generate mysqld.cnf  server-id=$server_id readonly=$readonly"
cat >$bpath/etc/mysqld.cnf <<_EOF_
[mysqld]
server-id   = ${server_id}
log-slave-updates = 1
binlog-format            = ROW
_EOF_

log "generate user.root.cnf user=root password=$root_pwd"
cat >$bpath/etc/user.root.cnf <<_EOF_
[client]
user=root
password=$root_pwd
socket=$bpath/tmp/mysql.sock
_EOF_

log "generate user.admin.cnf user=admin password=$admin_pwd"
cat >$bpath/etc/user.admin.cnf <<_EOF_
[client]
user=admin
password=$admin_pwd
socket=$bpath/tmp/mysql.sock
_EOF_

cat >$bpath/etc/my.cnf <<_EOF_
[client]
port                     = $port
socket                   = $bpath/tmp/mysql.sock

[mysqld]
core-file
rpl_databus_checksum_enabled=1
rpl_databus_trace_level=0
rpl_group_id=1
rpl_group_id_act_as_root=1
log_slave_connects = 1

!include $bpath/etc/mysqld.cnf

port                     = $port
socket                   = $bpath/tmp/mysql.sock
pid-file                 = $bpath/var/mysql.pid
basedir                  = $bpath
datadir                  = $bpath/var

# tmp dir settings
tmpdir                   = $bpath/tmp/
slave-load-tmpdir        = $bpath/tmp/ 

# 
language                 = $bpath/share/mysql/english/
character-sets-dir       = $bpath/share/mysql/charsets/

# skip options 
skip-name-resolve
skip-symbolic-links
skip-external-locking
skip-slave-start

#sysdate-is-now

# res settings
back_log                 = 50
max_connections          = 1000
max_connect_errors       = 10000
#open_files_limit         = 10240

connect-timeout          = 5
wait-timeout             = 28800
interactive-timeout      = 28800
slave-net-timeout        = 600
net_read_timeout         = 30
net_write_timeout        = 60
net_retry_count          = 10
net_buffer_length        = 16384
max_allowed_packet       = 64M

# 
table_cache              = 512
thread_stack             = 192K
thread_cache_size        = 20
thread_concurrency       = 8

# qcache settings
#query_cache_type         = 1
#query_cache_size         = 32M
#query_cache_size         = 256M
#query_cache_limit        = 2M
#query_cache_min_res_unit = 2K

# default settings
# time zone
default-time-zone        = system
character-set-server     = utf8
default-storage-engine   = InnoDB
#default-storage-engine   = MyISAM

# tmp & heap 
tmp_table_size           = 512M
max_heap_table_size      = 512M

log-bin                  = mysql-bin
log-bin-index            = mysql-bin.index
max_binlog_size          = 1G
sync-binlog      	 = 1000

relay-log                = relay-log
relay-log-index		 = relay-log.index
max_relay_log_size       = 1G

# warning & error log
log-warnings             = 1
log-error                = $bpath/log/mysql.err

# slow query log
long-query-time          = 1 
slow_query_log           = 1
slow_query_log_file      = ${bpath}/log/slow.log
#log-queries-not-using-indexes
# general query log
general_log				= 1
general_log_file                      = ${bpath}/log/mysql.log

# if use auto-ex, set to 0
relay-log-purge          = 1

# max binlog keeps days
expire_logs_days         = 7

binlog_cache_size        = 1M

# replication
replicate-wild-ignore-table     = mysql.%
replicate-wild-ignore-table     = test.%
# slave_skip_errors=all

key_buffer_size                 = 256M
sort_buffer_size                = 2M
read_buffer_size                = 2M
join_buffer_size                = 8M
read_rnd_buffer_size            = 8M
bulk_insert_buffer_size         = 64M
myisam_sort_buffer_size         = 64M
myisam_max_sort_file_size       = 10G
myisam_repair_threads           = 1
myisam_recover

transaction_isolation           = REPEATABLE-READ


innodb_file_per_table

#innodb_status_file              = 1
#innodb_open_files              = 2048
innodb_additional_mem_pool_size = 100M
innodb_buffer_pool_size         = 2G
innodb_data_home_dir            = $bpath/var/
innodb_data_file_path           = ibdata1:1G:autoextend
innodb_file_io_threads          = 4
innodb_thread_concurrency       = 16
innodb_flush_log_at_trx_commit  = 1

innodb_log_buffer_size          = 8M
innodb_log_file_size            = 1900M
innodb_log_files_in_group       = 2
innodb_log_group_home_dir       = $bpath/var/

innodb_max_dirty_pages_pct      = 90
innodb_lock_wait_timeout        = 50
#innodb_flush_method            = O_DSYNC

#baidu encrypt service
bdes_p_user = yushaozai
bdes_s_user = s_xdb_mysql_enc
bdes_group = g_secdb_pay_lcmiscdb 
bdes_role = BDES_fetch_key_role_dba_pay_0 

[mysqldump]
quick
max_allowed_packet              = 64M

[mysql]
disable-auto-rehash
default-character-set           = utf8
connect-timeout                 = 3

[isamchk]
key_buffer = 256M
sort_buffer_size = 256M
read_buffer = 2M
write_buffer = 2M

[myisamchk]
key_buffer = 256M 
sort_buffer_size = 256M
read_buffer = 2M
write_buffer = 2M

[mysqlhotcopy]
interactive-timeout
_EOF_
}

do_cmd cd $src_path

do_cmd sleep 1
do_cmd touch sql/share/errmsg.txt

export CFLAGS="-g -O3"
export CXXFLAGS="-g -O3"
do_cmd ./configure  --prefix=${bpath} --with-unix-socket-path=${bpath}/tmp/mysql.sock \
        --with-plugins=innobase \
        --enable-profiling \
    --with-charset=utf8 \
    --without-readline \
    --without-libedit \
    --with-extra-charsets=gbk,utf8,ascii,big5,latin1,binary,gb2312 \
    --enable-local-infile --enable-thread-safe-client --without-query-cache --with-debug

do_cmd make clean

do_cmd make -j 8 

do_cmd make install 

do_cmd make clean

#make clean && make && pwd 

do_cmd cd $bpath 

if [ ! -d etc ];then
        do_cmd mkdir etc
fi
if [ ! -d log ];then
        do_cmd mkdir log 
fi
if [ ! -d tmp ];then
        do_cmd mkdir tmp
fi
if [ ! -d var ];then
        do_cmd mkdir var 
fi

init_sql=$cur_dir/init.sql

do_cmd generate_sql_cnf $bpath $port $server_id $init_sql 

do_cmd chmod 644 $bpath/etc/my*.cnf 2>/dev/null
do_cmd chmod 600 $bpath/etc/user.*.cnf 2>/dev/null
do_cmd chmod 700 $bpath/var 
do_cmd chmod 755 $bpath/{bin,log,etc,libexec}

export LD_LIBRARY_PATH=$bpath/lib/mysql

do_cmd $bpath/bin/mysql_install_db --defaults-file=$bpath/etc/my.cnf 

log "[exec] start mysql, please wait ........."
#do_cmd ${bpath}/bin/mysql.server start
do_cmd ${bpath}/bin/mysqld_safe --defaults-file=${bpath}/etc/my.cnf --user="`whoami`" >/dev/null 2>&1 &
until [ -f $bpath/var/mysql.pid ]
do
        sleep 1
        log "[wait] wait mysql start up"
done

log "[succ] start mysql succeed"
do_cmd $bpath/bin/mysql -u root  < $init_sql 
do_cmd rm -rf $init_sql

do_cmd cd $cur_dirlog 

log "[succ]Bingo (:"
