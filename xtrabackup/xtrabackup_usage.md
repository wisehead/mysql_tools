#1.流式备份命令

```bash
#备份环节（流式备份需要在远程机器上也必须安装xtrabackup）
xtrabackup --defaults-file=${CONF} --user=${USER} -password=${PASSWD} --socket=${SOCKET} --tmpdir=/home/mysql/opdir --no-timestamp --slave-info --stream=xbstream --compress --compress-threads=4 --throttle=100 --backup --target-dir=/home/mysql/opdir 2>>${innobackupexLog} | ssh ${parpareip} "/home/mysql/opbin/xtrabackup/xbstream -x -C /home/mysql/snapshot_increment/23210/base"  >>${innobackupexLog} 2>&1
#CONF：代表数据库的配置文件etc/my.cnf
#USER:账号
#PASSWD:密码
#SOCKET：socket文件，一般在tmp/mysql.sock
#parpareip:代表远程的机器ip
#-C：代表存储目录，可自定义一个目录
#innobackupexLog：代表重定向输入文件
```

#2.sample2

```
innobackupex  --defaults-file=/data/mysql/3306/etc/my.cnf --user=root --compress --stream=xbstream ./ | ssh root@192.168.31.65 "xbstream -x -C /data/mysql/backup/"
```

