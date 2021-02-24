/root/mysql/bin/mysql_install_db --defaults-file=/root/mysql/etc/my.cnf.nvme_ssd --basedir=/root/mysql >> /dev/null
nohup /root/mysql/libexec/mysqld --defaults-file=/root/mysql/etc/my.cnf.nvme_ssd --user=root >> /dev/null &
sleep 2
/root/mysql/bin/mysql --defaults-extra-file=/root/mysql/etc/user.root.cnf -e "grant all privileges on *.* to 'root'@'10.32.247.86' identified by '' "
