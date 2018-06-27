FILESYSTEM=$1
touch init_disk_${FILESYSTEM}.sh

echo "DISK=(/dev/nvme1n1 /dev/nvme0n1)" >> init_disk_${FILESYSTEM}.sh
echo "MOUNT_DIR=(/ssd /nvram)" >> init_disk_${FILESYSTEM}.sh
echo "MOUNT_FILE=\"/etc/fstab\"" >> init_disk_${FILESYSTEM}.sh
echo "MOUNT_NEW_FILE=\"/etc/fstab_new\"" >> init_disk_${FILESYSTEM}.sh
echo "killall mysqld" >> init_disk_${FILESYSTEM}.sh
#sudo mv ${MOUNT_FILE} ${MOUNT_NEW_FILE}
echo "sleep 2" >> init_disk_${FILESYSTEM}.sh
echo "sudo mv /etc/fstab /etc/fstab_new" >> init_disk_${FILESYSTEM}.sh
#delete the last two line
#rm -f /etc/fstab
echo "touch /etc/fstab" >> init_disk_${FILESYSTEM}.sh
echo "sed 'N;\$!P;\$!D;\$d' /etc/fstab_new > /etc/fstab" >> init_disk_${FILESYSTEM}.sh
#while [ $i -lt ${#DISK[@]} ]
echo "for ((i=0; i<2; i=i+1));" >> init_disk_${FILESYSTEM}.sh
echo "        do" >> init_disk_${FILESYSTEM}.sh
        #xie zai
echo "        umount \${DISK[\$i]}" >> init_disk_${FILESYSTEM}.sh
        #ge shi hua
echo "        mkfs -t \${FILESYSTEM} \${DISK[\$i]}" >> init_disk_${FILESYSTEM}.sh
        #gua zai
echo "        echo \"\${DISK[\$i]} \${MOUNT_DIR[\$i]} \${FILESYSTEM} noatime,defaults 0 0\" >> /etc/fstab" >> init_disk_${FILESYSTEM}.sh
echo "        mount -a" >> init_disk_${FILESYSTEM}.sh
echo "done" >> init_disk_${FILESYSTEM}.sh
echo "mkdir /ssd/mysql" >> init_disk_${FILESYSTEM}.sh
echo "mkdir /ssd/mysql/mysql" >> init_disk_${FILESYSTEM}.sh
echo "mkdir /ssd/mysql/mysql/var" >> init_disk_${FILESYSTEM}.sh
echo "mkdir /nvram/mysql" >> init_disk_${FILESYSTEM}.sh
echo "mkdir /nvram/mysql/mysql" >> init_disk_${FILESYSTEM}.sh
echo "mkdir /nvram/mysql/mysql/var" >> init_disk_${FILESYSTEM}.sh


