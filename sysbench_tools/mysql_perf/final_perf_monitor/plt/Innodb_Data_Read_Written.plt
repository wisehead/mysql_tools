set terminal png truecolor
set output "Innodb_data_read_and_written.png"
set autoscale
set grid
set title "Innodb_data_read_and_written"
set xlabel "VTime(sec)"
set ylabel "MBytes"
set style data lines
sample=1
funx(x)=(sample * x)
funy(y)=(y / sample)
plot "< egrep  'Innodb_data_read\\b|Innodb_data_written\\b' global_0.out | \
awk 'BEGIN {i=0; for (i=0;i<2;i++) pre[i]=0; i=0} \
{ if( i >=2 ) {  dataOper[(i - i%2)/2, i % 2] = $2 - pre[ i % 2];} pre[i % 2] = $2; i++;}\
END{ num = NR/2 -1 ; for(i = 0; i < num; i++) printf(\"%d %s %s\\n\", \
i, dataOper[i,0]/1048576, dataOper[i,1]/1048576);}'"\
using (funx($1)):(funy($2)) lw 2 title " Innodb_data_read", '' \
using (funx($1)):(funy($3)) lw 2 title " Innodb_data_written"
