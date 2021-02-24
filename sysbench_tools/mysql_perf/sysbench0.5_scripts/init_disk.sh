FILESYSTEM=$1
DISK=(/dev/nvme1n1 /dev/nvme0n1)
MOUNT_DIR=(/ssd /nvram)
MOUNT_FILE="/etc/fstab"
MOUNT_NEW_FILE="/etc/fstab_new"
killall mysqld
#sudo mv ${MOUNT_FILE} ${MOUNT_NEW_FILE}
sleep 2
sudo mv /etc/fstab /etc/fstab_new
#delete the last two line 
#rm -f /etc/fstab
touch /etc/fstab
sed 'N;$!P;$!D;$d' /etc/fstab_new > /etc/fstab
i=0
#while [ $i -lt ${#DISK[@]} ]
for ((i=0; i<2; i=i+1));
	do
	#xie zai
	umount ${DISK[$i]}
	#ge shi hua
	mkfs -t ${FILESYSTEM} ${DISK[$i]}
	#gua zai
	echo "${DISK[$i]} ${MOUNT_DIR[$i]} ${FILESYSTEM} noatime,defaults 0 0" >> /etc/fstab
	mount -a
done
