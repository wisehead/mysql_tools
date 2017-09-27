set terminal png truecolor
set output "Innodb_io_trends.png"
set autoscale
set grid
set title "Innodb io status"
set xlabel "Time /S"
set ylabel "request(times)"
set style data lines
plot "<cat innodb_status.out| grep reads |grep avg|awk '{print NR\" \"$1\" \"$6}'" \
using 1:2 title  "reads/s",''\
using 1:3 title  "writes/s"
