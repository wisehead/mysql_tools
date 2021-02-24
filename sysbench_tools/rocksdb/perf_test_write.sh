#############################################################
#   File Name: perf_test.sh
#     Autohor: Hui Chen (c) 2019
#        Mail: chenhui13@baidu.com
# Create Time: 2019/06/27-14:29:19
#############################################################
#!/bin/sh 
#fillrandom,readrandom
#allow_concurrent_memtable_write=true
 
versionstr=batchRunQt4
output_path=/ssd1/chenhui/rocksdbpath
 
# size constants
K=1024
M=$((1024 * K))
G=$((1024 * M))
 
const_params="--memtablerep=skiplist --db=$output_path/rdb/db --wal_dir=$output_path/rdb/wal --benchmark_write_rate_limit=0 --soft_pending_compaction_bytes_limit=437438953472 --hard_pending_compaction_bytes_limit=437438953472  --statistics=1 --stats_per_interval=1 --cache_numshardbits=6 --compression_max_dict_bytes=0  --level_compaction_dynamic_level_bytes=true --stats_interval_seconds=10 --cache_index_and_filter_blocks=1 --pin_l0_filter_and_index_blocks_in_cache=0 --rate_limit_delay_max_milliseconds=1000000 --verify_checksum=1 --histogram=1 --compaction_pri=3 --bloom_bits=10 --open_files=-1 --use_adaptive_mutex=false --enable_write_thread_adaptive_yield=false --write_thread_max_yield_usec=100 --delayed_write_rate=2000554432 --compaction_readahead_size=0 --min_write_buffer_number_to_merge=4 --seed=1539069512 --max_compaction_bytes=4294967296 --disable_wal=0 --max_total_wal_size=$((4 * G)) "
 
valuesizes=(64 512)
cp_type=(none zstd lz4)
rw_type=(99 90 80 50 1)
i=0
j=0
k=0
itotal=${#valuesizes[@]}
cpotal=${#cp_type[@]}
rwtal=${#rw_type[@]}
 
while [ "$i" -lt "$itotal" ]
do
  while [ "$j" -lt "$cpotal" ]
  do
    rm -rf $output_path/rdb
    rm -rf $output_path/log/tmp
    mkdir -p $output_path/rdb
    mkdir -p $output_path/log/tmp
 
    echo " === $versionstr-$i-$j === " >> $output_path/top.log
    date >> $output_path/top.log
    echo " === $versionstr-$i-$j === " >> $output_path/iostat.log
    date >> $output_path/iostat.log
 
    echo " === $versionstr-$i-$j make data === " >> $output_path/log/tmp/benchmark.fill.log
    #fill
    cmd="/home/chenhui/rocksdb/db_bench  --num=102400000  --num_column_families=1 --use_existing_db=0 --batch_size=10 --benchmarks='fillseq,overwrite,readseq,readseq' --sync=0 --num_levels=7 --key_size=16 --value_size=${valuesizes[$i]}  --compression_type=${cp_type[$j]}  --cache_size=$((4 * G)) --bytes_per_sync=$((512 * K)) --wal_bytes_per_sync=$((512 * K)) --max_bytes_for_level_multiplier=10 --use_direct_io_for_flush_and_compaction=false --max_background_compactions=8 --level0_stop_writes_trigger=92 --write_buffer_size=$((32 * M)) --target_file_size_base=$((32 * M)) --target_file_size_multiplier=2 --max_bytes_for_level_base=$((256 * M)) --level0_file_num_compaction_trigger=2 --subcompactions=2 --level0_slowdown_writes_trigger=48 --max_write_buffer_number=64 --block_size=$((64 * K)) --max_background_flushes=64 --enable_pipelined_write=true --allow_concurrent_memtable_write=true --disable_auto_compactions=false --memtable_use_huge_page=true $const_params \
    2>&1 | tee -a $output_path/log/tmp/benchmark.fill.log"
    echo $cmd | tee  -a $output_path/log/tmp/benchmark.fill.log
    eval $cmd
    #summarize_result $output_path/log/tmp/benchmark.fill.log 0 $versionstr
    echo 3 > /proc/sys/vm/drop_caches;
    sleep 85;
 
    while [ "$k" -lt "$rwtal" ]
    do
      echo " === $versionstr-$i-$j-$k=== " >> $output_path/log/tmp/benchmark.fill.log
      cmd="/home/chenhui/rocksdb/db_bench --num=2048000 --readwritepercent=${rw_type[$k]} --num_column_families=1 --use_existing_db=1  --threads=32 --batch_size=10 --benchmarks='readrandomwriterandom' --sync=0 --num_levels=7 --key_size=16 --value_size=${valuesizes[$i]}  --compression_type=${cp_type[$j]}  --cache_size=$((4 * G)) --bytes_per_sync=$((512 * K)) --wal_bytes_per_sync=$((512 * K)) --max_bytes_for_level_multiplier=10 --use_direct_io_for_flush_and_compaction=false --max_background_compactions=8 --level0_stop_writes_trigger=92 --write_buffer_size=$((32 * M)) --target_file_size_base=$((32 * M)) --target_file_size_multiplier=2 --max_bytes_for_level_base=$((256 * M)) --level0_file_num_compaction_trigger=2 --subcompactions=2 --level0_slowdown_writes_trigger=48 --max_write_buffer_number=64 --block_size=$((64 * K)) --max_background_flushes=64 --enable_pipelined_write=true --allow_concurrent_memtable_write=true --disable_auto_compactions=false --memtable_use_huge_page=true $const_params \
      2>&1 | tee -a $output_path/log/tmp/benchmark.fill.log"
      echo $cmd | tee  -a $output_path/log/tmp/benchmark.fill.log
      eval $cmd
      #summarize_result $output_path/log/tmp/benchmark.fill.log 0 $versionstr
 
      sleep 15;
      echo 3 > /proc/sys/vm/drop_caches;
      ((k++))
    done
     
    mv $output_path/log/tmp $output_path/log/tmp$versionstr-$i-$j
    ((k=0))
    ((j++))
  done
  ((j=0))
  ((i++))
done
