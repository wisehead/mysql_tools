set terminal png truecolor
set output "Innodb_data_reads_and_writes.png"
set autoscale
set grid
set title "Innodb_data_reads_and_writes"
set xlabel "VTime(sec)"
set ylabel "Operation"
set style data lines
sample=1
funx(x)=(sample * x)
funy(y)=(y / sample)
plot "< egrep  'Innodb_data_reads\\b|Innodb_data_writes\\b' global_0.out | \
awk 'BEGIN {i=0; for (i=0;i<2;i++) pre[i]=0; i=0} \
{ if( i >=2 ) {  dataOper[(i - i%2)/2, i % 2] = $2 - pre[ i % 2];} pre[i % 2] = $2; i++;}\
END{ num = NR/2 -1 ; for(i = 0; i < num; i++) printf(\"%s %s %s\\n\", \
i, dataOper[i,0], dataOper[i,1]);}'"\
using (funx($1)):(funy($2)) lw 2 title "Innodb_data_reads", '' \
using (funx($1)):(funy($3)) lw 2 title "Innodb_data_writes"

