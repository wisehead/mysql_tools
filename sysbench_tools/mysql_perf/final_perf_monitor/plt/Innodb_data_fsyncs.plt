set terminal png truecolor
set output "Innodb_data_fsync.png"
set autoscale
set grid
set title "Innodb_data_fsyncs"
set xlabel "VTime(sec)"
set ylabel "request"
set style data lines
sample=1
funx(x)=(x * sample)
funy(y)=(y / sample)
plot "< egrep 'Innodb_data_fsyncs' global_0.out |\
awk 'BEGIN {i=0; pre=0; } \
{ if( i >=1 ) {  fsyncs[i-1] = $2 - pre;} pre = $2; i++;}\
END{ num = NR-1 ; for(i = 0; i < num; i++) printf(\"%d %d \\n\", \
i, fsyncs[i]);}' " \
using (funx($1)):(funy($2)) lw 2 title "innodb_data_fsyncs" 
