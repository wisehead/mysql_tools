#############################################################
#   File Name: start.sh
#     Autohor: Hui Chen (c) 2023
#        Mail: chenhui13@xxx.com
# Create Time: 2023/01/07-10:57:15
#############################################################
#!/bin/sh 
mysqld_safe --defaults-file=/data/chenhui/install/my.cnf --core_file --user=mysql &
