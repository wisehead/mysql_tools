set terminal png truecolor
set output "Com_operation.png"
set autoscale
set grid
set title "Com_operation"
set xlabel "VTime(sec)"
set ylabel "TPS"
set style data lines
sample=1
funx(x)=(sample * x)
funy(y)=(y / sample)
plot "< grep -P 'Com_begin|Com_commit|Com_delete\\b|Com_insert\\b|Com_select\\b|Com_update\\b' global_0.out | \
awk 'BEGIN {i=0; for (i=0;i<6;i++) pre[i]=0; i=0} \
{ if( i >=6 ) {  comOperArr[(i - i%6)/6, i % 6] = $2 - pre[ i % 6];} pre[i % 6] = $2; i++;}\
END{ num = NR/6 -1 ; for(i = 0; i < num; i++) printf(\"%d %d %d %d %d %d %d\\n\", \
i, comOperArr[i,0],comOperArr[i,1],comOperArr[i,2],comOperArr[i,3],comOperArr[i,4],comOperArr[i,5]);}'"\
using (funx($1)):(funy($2)) lw 2 title "begin", '' \
using (funx($1)):(funy($3)) lw 2 title "commit", '' \
using (funx($1)):(funy($4)) lw 2 title "delete", '' \
using (funx($1)):(funy($5)) lw 2 title "insert", '' \
using (funx($1)):(funy($6)) lw 2 title "select", '' \
using (funx($1)):(funy($7)) lw 2 title "update" 
