#############################################################
#   File Name: getpid.sh
#     Autohor: Hui Chen (c) 2023
#        Mail: chenhui13@xxx.com
# Create Time: 2023/01/07-18:50:48
#############################################################
#!/bin/sh 
ps -ef|grep mysqld|grep -v grep|grep -v mysqld_safe|awk '{print$2}'
