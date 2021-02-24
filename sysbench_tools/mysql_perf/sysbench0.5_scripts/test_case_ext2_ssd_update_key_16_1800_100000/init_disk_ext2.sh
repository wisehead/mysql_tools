DISK=(/dev/nvme1n1 /dev/nvme0n1)
MOUNT_DIR=(/ssd /nvram)
MOUNT_FILE="/etc/fstab"
MOUNT_NEW_FILE="/etc/fstab_new"
killall mysqld
sleep 2
sudo mv /etc/fstab /etc/fstab_new
touch /etc/fstab
sed 'N;$!P;$!D;$d' /etc/fstab_new > /etc/fstab
for ((i=0; i<2; i=i+1));
        do
        umount ${DISK[$i]}
        mkfs -t ${FILESYSTEM} ${DISK[$i]}
        echo "${DISK[$i]} ${MOUNT_DIR[$i]} ${FILESYSTEM} noatime,defaults 0 0" >> /etc/fstab
        mount -a
done
mkdir /ssd/mysql
mkdir /ssd/mysql/mysql
mkdir /ssd/mysql/mysql/var
mkdir /nvram/mysql
mkdir /nvram/mysql/mysql
mkdir /nvram/mysql/mysql/var
