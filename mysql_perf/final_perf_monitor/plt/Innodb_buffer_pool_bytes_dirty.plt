set terminal png truecolor
set output "Innodb_buffer_pool_bytes_dirty.png"
set autoscale
set grid
set title "Innodb_buffer_pool_bytes_dirty"
set xlabel "VTime(s)"
set ylabel "MBytes"
set style data lines
sample=1
funx(x)=(sample * x)
plot "< grep 'Innodb_buffer_pool_bytes_dirty' global_0.out |awk 'BEGIN{ i=0} {print i \" \" $2/1048576; i++};' " using (funx($1)) :2 lw 2 title "Innodb_buffer_pool_bytes_dirty"
