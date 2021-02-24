set terminal png truecolor
set output "Buffer_pool_wait_free.png"
set autoscale
set grid
set title "Buffer pool wait free"
set xlabel "VTime(s)"
set ylabel "free wait"
set style data lines
sample=1
funx(x)=(sample * x)
plot "< grep 'Innodb_buffer_pool_wait_free' global_0.out \
| awk 'BEGIN{ i=0} {print i \" \" $2; i++};' " \
using (funx($1)) :2 lw 2 title "Wait Free"
