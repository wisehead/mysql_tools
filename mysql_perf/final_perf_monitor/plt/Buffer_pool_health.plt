set terminal png truecolor
set output "Buffer_pool_dirty_pct.png"
set autoscale
set grid
set title "Buffer pool Health"
set xlabel "VTime(sec)"
set ylabel "%(pct)"
set style data lines
sample=1
funx(x)=(sample * x)
plot "< egrep 'Innodb_buffer_pool_pages_dirty|Innodb_buffer_pool_pages_data' global_0.out |awk 'BEGIN{i=0} { arr[(i - i%2)/2, i%2] = $2; i++} \
END{num = NR/2; for(i=0; i<num; i++) print i\" \" (1-arr[i,1]/arr[i,0])*100;}'" \
using (funx($1)):2 lw 2 title "dirty pages pct"
