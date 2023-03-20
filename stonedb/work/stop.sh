#############################################################
#   File Name: stop.sh
#     Autohor: Hui Chen (c) 2023
#        Mail: chenhui13@xxx.com
# Create Time: 2023/01/07-10:59:06
#############################################################
#!/bin/sh 
mysqladmin shutdown -uroot -p666666 -S /data/chenhui/install/tmp/mysql.sock
#mysqladmin shutdown -uroot -p666666 -S /tmp/mysql.sock
#mysqladmin shutdown -uroot -p666666
#mysqladmin --defaults-file=$bpath/etc/user.root.cnf shutdown 
