import sys
import threading
import time
import MySQLdb


MYSQL_DSN = {
    'host':'10.50.37.22',
    'user':'dbproxy_yace',
    'passwd':'123456',
    'db':'wapiknow',
    'port':3100
}


def conn_mysql(dsn):
    retry = 0
    while retry < 3:
        try:
            conn = MySQLdb.connect(**dsn)
            return conn
        except MySQLdb.Error as e:
            print e.args[0], e.args[1]
            print ("connect mysql retry %d times,excption %s" % (retry, str(e.args[0])))
            time.sleep(2)
            retry = retry + 1
    return 1


class Mythread (threading.Thread): 
    def __init__(self, mconn, thread_id, general_log):
        threading.Thread.__init__(self)
        self.mconn = mconn
        self.thread_id = thread_id
        self.general_log = general_log

    def run(self):                 
        print "Starting Thread_id " + str(self.thread_id) + "! File Name " + str(self.general_log)
        dir_log = '/home/mysql/opdir/' + str(self.general_log)
        try:
            glog_content = open(dir_log, "r")
            for row in glog_content.readlines():
                status = self.exec_sql(row)
                while (status == 1):
                    status = self.exec_sql(row)
                    time.sleep(2)
        except ValueError:
            print "MYSQL LOG NOT EXISTS!"
            os._exit(0)
     
    def exec_sql(self, row):
        cursor = self.mconn.cursor()
        self.mconn.autocommit(1)
        try:
            cursor.execute(row)
            cursor.close()
        except MySQLdb.Warning, w:
            pass
        except MySQLdb.Error, e:
            print "MySQL CONNECT FAILED"
            return 1


def main(argv):
    global MYSQL_DSN
    if len(argv) == 2: 
        general_list = []
        parr = argv[1]
        for list_num in range(int(parr)):
            file_name = 'mysql.log_' + str(list_num)
            general_list.append(file_name)
        for th_id in range(int(parr)):
            conn = conn_mysql(MYSQL_DSN)
            print conn
            general_log = general_list.pop()
            thread_id = th_id
            time.sleep(1)
            thread = Mythread(conn, thread_id, general_log)
            thread.start()
    else:
        print "Usage %s [parrallel_num]" % argv[0]


if __name__ == '__main__':
    main(sys.argv)
