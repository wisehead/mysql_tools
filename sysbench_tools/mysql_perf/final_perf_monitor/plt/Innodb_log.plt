set terminal png truecolor
set output "Innodb_log.png"
set autoscale
set grid
set title "Innodb Log"
set xlabel "VTime(sec)"
set xlabel "request"
set style data lines
sample=1
funx(x)=(x * sample)
funy(y)=(y / sample)
plot "< egrep 'Innodb_log_waits|Innodb_log_write_requests|Innodb_log_writes' global_0.out |\
awk 'BEGIN {i=0; for (i=0;i<3;i++) pre[i]=0; i=0} \
{ if( i >=3 ) {  requests[(i - i%3)/3, i % 3] = $2 - pre[ i % 3];} pre[i % 3] = $2; i++;}\
END{ num = NR/3 -1 ; for(i = 0; i < num; i++) printf(\"%d %s %s %s\\n\", \
i, requests[i,0],requests[i,1],requests[i,2]);}' "\
using (funx($1)):(funy($2)) lw 2 title "innodb log waits", '' \
using (funx($1)):(funy($3)) lw 2 title "innodb log writes request", '' \
using (funx($1)):(funy($4)) lw 2 title "innodb log writes"
