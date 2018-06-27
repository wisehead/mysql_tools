#!/usr/bin/env bash
src_dir=`pwd`/..
install_dir=/usr/local/mysql

if [ -d /usr/local/mysql ]; then
echo "MySQL server is installed at /usr/local/mysql"
else
cd $src_dir && mkdir -p build
(cd ${src_dir}/build && cmake .. -DDOWNLOAD_BOOST=1 -DWITH_BOOST=../boost -DENABLE_DOWNLOADS=1)
make
make install
fi

if [ ! -f /etc/profile.d/mysql.sh ]; then
  echo -e "if ! echo \$PATH | grep -q /usr/local/mysql/bin; then\nPATH=/usr/local/mysql/bin:\$PATH\nfi" > /etc/profile.d/mysql.sh
  source /etc/profile.d/mysql.sh
fi

if [ ! -f /etc/ld.so.conf.d/mysql.conf ]; then
echo "/usr/local/mysql/lib" > /etc/ld.so.conf.d/mysql.conf
fi

if ! ldconfig -p | grep -q /usr/local/mysql/lib; then
ldconfig >/dev/null
fi
