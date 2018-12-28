#############################################################
#   File Name: build.sh
#     Autohor: Hui Chen (c) 2016
#        Mail: alex.chenhui@gmail.com 
# Create Time: 2016/06/01-12:30:01
#############################################################
#!/bin/sh 


autoreconf --force --install

make clean

srcdir=mysql-5163

if [ -f $srcdir.tar.gz ]
then
        rm ./$srcdir.tar.gz
fi

find . -name ".svn"|xargs rm -rf
find . -name ".deps"|xargs rm -rf

if [ -d output ]
then
        rm -rf ./output
fi

mkdir -p ./output/bin
mkdir -p ./output/build
mkdir -p ./output/include
mkdir -p ./output/lib

cd ../

tar -zcvf $srcdir.tar.gz $srcdir

cd $srcdir

sleep 1
touch sql/share/errmsg.txt

export CC=/opt/compiler/gcc-4.8.2/bin/gcc
export CXX=/opt/compiler/gcc-4.8.2/bin/g++
export CFLAGS='-fPIC -O3 -g'
export CXXFLAGS='-fPIC -O3 -g'
./configure  --prefix=$PWD/output/build --with-extra-charsets=all --without-docs --without-man --without-bench 
#./configure  --prefix=$PWD/output/build --with-extra-charsets=all --without-server --without-docs --without-man --without-bench 

make -j 8 

make install

cp ./output/build/include/mysql/* ./output/include
cp ./output/build/lib/mysql/libmysqlclient.a ./output/lib
cp ./output/build/libexec/mysqld ./output/bin
rm -rf ./output/build

cp -r demo_client ./output/
make -C ./output/demo_client/
cp ./output/demo_client/mysqldatabus ./output/bin
make clean -C ./output/demo_client

cp ./install_baidu_mysql.sh ./output
cp ./upgrade_baidu_mysql.sh ./output
mv ../$srcdir.tar.gz ./output
