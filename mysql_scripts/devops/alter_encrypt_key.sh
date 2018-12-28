#############################################################
#   File Name: alter_encrypt_key.sh
#     Autohor: Hui Chen (c) 2017
#        Mail: chenhui13@baidu.com
# Create Time: 2017/06/28-19:56:03
#############################################################
#!/bin/sh 
#!/bin/sh
for i in {1..1000000}
    do 
        echo $i 
        #head -n $var $1|tail -1 >>error.wrong.sql  
        mysql -uroot -proot -e "alter encrypt_key"
        #sleep 10
    done    
