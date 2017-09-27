set terminal png truecolor
set output "Innodb_row_lock_time.png"
set autoscale
set grid
set title "Innodb_row_lock_time"
set xlabel "VTime(sec)"
set ylabel "milli seconds"
set style data lines
sample=1
funx(x)=(x * sample)
funy(y)=(y / sample)
plot "< egrep 'Innodb_row_lock_time\\b' global_0.out |\
awk 'BEGIN {i=0; pre=0; } \
{ if( i >=1 ) {  requests[i-1] = $2 - pre;} pre = $2; i++;}\
END{ num = NR-1 ; for(i = 0; i < num; i++) printf(\"%d %d \\n\", \
i, requests[i]);}' " \
using (funx($1)):(funy($2)) lw 2 title "Innodb_row_lock_time" 
