使用Percona Data Recovery Tool for InnoDB恢复数据

昨晚收到一则求助，一个用户的本地数据库的重要数据由于误操作被删除，需要进行紧急恢复，用户的数据库日常并没有进行过任何备份，binlog也没有开启，所以从备份和binlog入手已经成为不可能，咨询了丁奇，发了一篇percona的文章给我，顿时感觉有希望，于是到percona的官网上下载了恢复工具：
一.安装：
.tar -xvf percona-data-recovery-tool-for-innodb-0.5.tar.gz
.cd percona-data-recovery-tool-for-innodb-0/mysql-source/
../configure
.cd percona-data-recovery-tool-for-innodb-0
.make

二.解析ibd文件：
此过程会将表的idb文件解析为很多的page，innodb的page分为两大部分，一部分一级索引部分（primary key），另一部分为二级索引部分（secondary key），所以解析出来的idb包括了主键数据和索引数据两大部分（如果该表有多个二级索引，则会生成多个文件）
./page_parser -5 -f t_bibasic_storage.ibd
参数解释：
-5：代表 row format为Compact
-f：代表要解析的文件
结果如下：
pages-1377707810/FIL_PAGE_INDEX
0-161 0-325 0-463 0-464 0-465
可以看到t_bibasic_storage.ibd解析出来5个文件（161为主键索引的index_id,325,463,464,465为二级索引的index_id,该id可以通过开启innodb_table_monitor知晓）

三.生成表定义：
由于该工具在解析数据pages的时候，需要获得该table的表结构定义，所以需要执行如下命令：
./create_defs.pl –host xxxx –port 3306 –user root –password xxx –db didb –table t_bibasic_storage >include/table_defs.h
上面的命令会将t_bibasic_storage表的表结构定义传入到table_defs.h中，在生成了表结构定义后，重新make该恢复工具：
.make

四.开始恢复pages中删除的数据：
在重新编译工具后，执行如下命令：
./constraints_parser -5 -D -f pages-1377707810/FIL_PAGE_INDEX/0-161 >/tmp/t_bibasic_salessend.sql
参数：
-5 -f的参数和page_parser相同；
-D:该参数的含义为代表恢复删除的数据页；

恢复完成后生成如下语句和文件：
LOAD DATA INFILE ‘/tmp/t_bibasic_proinfo.dmp’ REPLACE INTO TABLE `t_bibasic_proinfo` FIELDS TERMINATED BY ‘\t’ OPTIONALLY ENCLOSED BY ‘”‘ LINES STARTING BY ‘t_bibasic_proinfo\t’ (id, procode, skuoid, skucode, skuname, catatt, dutydepoid, dutydepname, seasonatt, brandatt, prostatus, choosedate, syear, smonth, sday, created, unioncomcode);

/tmp/t_bibasic_salessend.sql 该文件就是我们需要load data的文本文件；

总结：
1）。该恢复工具只支持innodb存储引擎，文件的格式需要为：Compact
2）。数据被误删除后，需要尽快将保护现场，停止数据库，把idb文件拷贝出来，防止ibd文件写入数据被覆盖（笔者恢复的一个表中，由于数据删除后，表中仍有大量写入，导致大部分数据没有恢复出来）；
3）。千叮嘱万嘱咐，数据库的备份重于泰山；
