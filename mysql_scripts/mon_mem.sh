#############################################################
#   File Name: mon_mem.sh
#     Autohor: Hui Chen (c) 2017
#        Mail: chenhui13@baidu.com
# Create Time: 2017/09/25-19:33:56
#############################################################
#!/bin/sh 

SLEEP_TIME=$1

while [ true ]
do
    #iostat -xkt 1 1
    free|grep -i mem|awk '{print$3}'
    #sleep $((SLEEP_TIME-1))
    sleep 1
done
