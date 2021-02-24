disk=$1

/root/mysql/bin/mysql_install_db --defaults-file=/root/mysql/etc/my.cnf.nvme_${disk} --basedir=/root/mysql >> /dev/null

nohup /root/mysql/libexec/mysqld --defaults-file=/root/mysql/etc/my.cnf.nvme_${disk} --user=root >> /dev/null &
