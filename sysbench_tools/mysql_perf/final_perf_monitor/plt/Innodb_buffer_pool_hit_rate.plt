set terminal png truecolor
set output "Innodb_buffer_pool_hit_rate.png"
set autoscale
set grid
set title "Innodb buffer pool hit rate"
set xlabel "Time  /S"
set ylabel "rate(%)"
set style data lines
plot "<grep \"Buffer pool hit rate\" innodb_status.out |awk '{{if(NR%9==0){print NR/9\" \"($5/$7)*100}}}'"\
using 1:2 title  "hit rate"
