#############################################################
#   File Name: create_user.sh
#     Autohor: Hui Chen (c) 2023
#        Mail: chenhui13@xxx.com
# Create Time: 2023/03/09-10:27:46
#############################################################
#!/bin/sh 
#/data/chenhui/install/bin/mysql -uroot -p -S /data/chenhui/install/tmp/mysql.sock

alter user 'root'@'localhost' identified by '666666';
GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '666666' WITH GRANT OPTION;
FLUSH PRIVILEGES;
