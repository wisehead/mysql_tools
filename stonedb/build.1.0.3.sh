#############################################################
#   File Name: build.sh
#     Autohor: Hui Chen (c) 2022
#        Mail: chenhui13@xxx.com
# Create Time: 2022/12/29-16:10:51
#############################################################
#!/bin/sh 
export CFLAGS="-O0 -g"
export CXXFLAGS="-O0 -g -std=c++11"
install_target=/data/chenhui/install
#rm -rf release
#mkdir release
cd release

cmake ../  
-DCMAKE_BUILD_TYPE=Debug 
-DCMAKE_INSTALL_PREFIX=${install_target} 
-DMYSQL_DATADIR=${install_target} 
-DSYSCONFDIR=${install_target} 
-DMYSQL_UNIX_ADDR=${install_target}/tmp/mysql.sock 
-DWITH_EMBEDDED_SERVER=OFF 
-DWITH_TIANMU_STORAGE_ENGINE=1 
-DWITH_MYISAM_STORAGE_ENGINE=1 
-DWITH_INNOBASE_STORAGE_ENGINE=1 
-DWITH_PARTITION_STORAGE_ENGINE=1 
-DMYSQL_TCP_PORT=3306 
-DENABLED_LOCAL_INFILE=1 
-DEXTRA_CHARSETS=all 
-DDEFAULT_CHARSET=utf8mb4 
-DDEFAULT_COLLATION=utf8mb4_general_ci 
-DDOWNLOAD_BOOST=0 
-DWITH_BOOST=/usr/local/stonedb-boost 
-DWITH_MARISA=/usr/local/stonedb-marisa 
-DWITH_ROCKSDB=/usr/local/stonedb-gcc-rocksdb     
-DCMAKE_C_FLAGS='-O0 -g'  
-DCMAKE_CXX_FLAGS='-O0 -g -std=c++11'   
-DCMAKE_C_FLAGS_RELEASE='-O0 -g'  
-DCMAKE_CXX_FLAGS_RELEASE='-O0 -g -std=c++11' 

#make -j8
#make install
