#!/home/mysql/.jumbo/bin/python
# -*- coding:utf-8 -*-

import sys
import MySQLdb
import readline
import warnings

warnings.filterwarnings("ignore")


def usage():
    """
    使用说明
    """
    print "./rpl.py IP1:PORT1 IP2:PORT2"


def parse_param():
    """
    获取并解析程序的选项
    """
    if len(sys.argv) != 3:
        print "\033[1;31;40m[NOTICE]Please input correct parameters...\033[0m"
        sys.exit()
    db1_list = sys.argv[1].split(":")
    db2_list = sys.argv[2].split(":")
    return db1_list[0],db1_list[1],db2_list[0],db2_list[1]

def get_master_info(db_user, db_passwd, db_ip, db_port):
    """
    获取主库binlog信息
    """
    conn=MySQLdb.connect(db_ip ,db_user, db_passwd, "mysql", int(db_port))
    cursor = conn.cursor()
    cursor.execute("show master status")
    result =  cursor.fetchall()[0]
    sql = "grant replication slave on *.* to '%s'@'10.%%.%%.%%' identified by '%s'" %(db_user, db_passwd)
    # print sql
    cursor.execute(sql)
    cursor.close()  
    conn.close()
    print "\033[1;32;40m[NOTICE]: Capture master binlog file name and position success !\033[0m"
    return result[0],result[1]

def slave_config(db1_user, db1_passwd, db1_ip, db1_port, db2_user, db2_passwd, db2_ip, db2_port, file_name, file_pos):
    """
    主从同步设置，基于name和pos的方式主从连接，以后会采用GTID的方式连接
    """
    conn_db2 = MySQLdb.connect(db2_ip ,db2_user, db2_passwd, "mysql", int(db2_port))
    cursor_db2 = conn_db2.cursor()
    ret = cursor_db2.execute("stop slave") 
    if ret != 0 :
        print "\033[1;31;40m[ERROR]: stop slave fail !\033[0m"

    rep_sql = "change master to master_host='%s',master_port=%s,master_user='%s',master_password='%s',master_log_file='%s',master_log_pos=%s" %(db1_ip, db1_port, db1_user, db1_passwd, file_name, file_pos)

    ret = cursor_db2.execute(rep_sql)
    if ret != 0:
        print "\033[1;31;40m[ERROR]: config slave fail !\033[0m"

    ret = cursor_db2.execute("start slave")  
    if ret != 0 :
        print "\033[1;31;40m[ERROR]: start slave fail !\033[0m"
    
    cursor_db2.close() 
    conn_db2.close()  
    print "\033[1;32;40m[NOTICE]: Slave configure sucess !\033[0m"

def slave_check(db2_user,db2_passwd, db2_ip, db2_port):
    conn = MySQLdb.connect(db2_ip ,db2_user, db2_passwd, "mysql", int(db2_port))
    cursor = conn.cursor(cursorclass = MySQLdb.cursors.DictCursor)    
    cursor.execute("show slave status")  
    res = cursor.fetchall()
    slave_io_running = res[0]["Slave_IO_Running"]
    slave_slave_running = res[0]["Slave_SQL_Running"]
    cursor.close()
    conn.close()  
    if (slave_io_running != "Yes") or (slave_slave_running != "Yes"):
        print "\033[1;31;40m[ERROR]: Replication configure Fail !\033[0m"  
        sys.exit()
    print "\033[1;32;40m[NOTICE]: Replication configure success !\033[0m"

    
if __name__ == "__main__":
    """
    主函数
    """
    db1_ip,db1_port,db2_ip,db2_port = parse_param()
    
    print "\033[1;33;40m---------------Please input Replication information-------------\033[0m"
    print "(%s:%s)\033[4;33;40mMaster username:\033[0m" % (db1_ip, db1_port),
    db1_user = raw_input()
    print "(%s:%s)\033[4;33;40mMaster password:\033[0m" % (db1_ip, db1_port),
    db1_passwd = raw_input()
    print "(%s:%s)\033[4;36;40mSlave username:\033[0m" % (db2_ip, db2_port),
    db2_user = raw_input()
    print "(%s:%s)\033[4;36;40mSlave password:\033[0m" % (db2_ip, db2_port),
    db2_passwd = raw_input()

    master_log_file, master_log_pos = get_master_info(db1_user,db1_passwd, db1_ip, db1_port)
    # print "Master Log File is %s, Master Log position is %s" %(master_log_file, master_log_pos)
    print "\033[1;33;40m---------------Beginning to configure slave------------------\033[0m"
    slave_config(db1_user,db1_passwd, db1_ip, db1_port, db2_user,db2_passwd, db2_ip, db2_port, master_log_file, master_log_pos)
    slave_check(db2_user,db2_passwd, db2_ip, db2_port)


