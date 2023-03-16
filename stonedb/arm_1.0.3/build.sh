#!/bin/sh

set -e
#build_type=Debug
build_type=Release

install_target=/stonedb57/install

# then remove the target installation
#rm -fr ${install_target}

# step 3. create build directory
cd release
#mkdir -p ${install_target}

# step 4. begin to build
cmake ../ \
-DCMAKE_BUILD_TYPE=${build_type} \
-DCMAKE_INSTALL_PREFIX=${install_target} \
-DMYSQL_DATADIR=${install_target}/data \
-DSYSCONFDIR=${install_target} \
-DMYSQL_UNIX_ADDR=${install_target}/tmp/mysql.sock \
-DWITH_EMBEDDED_SERVER=OFF \
-DWITH_TIANMU_STORAGE_ENGINE=1 \
-DWITH_MYISAM_STORAGE_ENGINE=1 \
-DWITH_INNOBASE_STORAGE_ENGINE=1 \
-DWITH_PARTITION_STORAGE_ENGINE=1 \
-DMYSQL_TCP_PORT=3306 \
-DENABLED_LOCAL_INFILE=1 \
-DEXTRA_CHARSETS=all \
-DDEFAULT_CHARSET=utf8mb4 \
-DDEFAULT_COLLATION=utf8mb4_general_ci \
-DDOWNLOAD_BOOST=0 \
-DWITH_BOOST=/usr/local/stonedb-boost \
-DWITH_ROCKSDB=/usr/local/stonedb-gcc-rocksdb \
-DLOCAL_MARISA_DIR=/usr/local/stonedb-marisa \
-DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -DDBUG_OFF -fabi-version=2 -fno-omit-frame-pointer -fno-strict-aliasing -Wno-error -fpermissive -Wno-unused-variable -Wno-unused-parameter -gdwarf-2 -std=c++1z -fno-sized-deallocation"
2>&1 | tee -a ${build_log}

# step 5. make & make install
#make VERBOSE=1 -j`nproc`                                             2>&1 | tee -a ${build_log}
#make install                                                         2>&1 | tee -a ${build_log}
#echo "current dir is `pwd`"                                                              2>&1 | tee -a ${build_log}
