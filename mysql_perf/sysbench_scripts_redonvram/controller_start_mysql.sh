DISK=$1

touch start_mysql_${DISK}.sh

echo "/root/mysql/bin/mysql_install_db --defaults-file=/root/mysql/etc/my.cnf.nvme_${DISK} --basedir=/root/mysql >> /dev/null" >> start_mysql_${DISK}.sh

echo "nohup /root/mysql/libexec/mysqld --defaults-file=/root/mysql/etc/my.cnf.nvme_${DISK} --user=root >> /dev/null &" >> start_mysql_${DISK}.sh

echo "sleep 2" >> start_mysql_${DISK}.sh

echo "/root/mysql/bin/mysql --defaults-extra-file=/root/mysql/etc/user.root.cnf -e \"grant all privileges on *.* to 'root'@'10.32.247.86' identified by '' \"" >> start_mysql_${DISK}.sh
