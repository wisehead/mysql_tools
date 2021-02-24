set terminal png truecolor
set output "Innodb_Page_Trend.png"
set autoscale
set grid
set title "Innodb Page"
set xlabel "VTime(sec)"
set ylabel "pages"
set style data lines
sample=1
funx(x)=(sample * x)
funy(y)=(y / sample)
plot "< egrep  'Innodb_pages_created|Innodb_pages_read|Innodb_pages_written' global_0.out | \
awk 'BEGIN {i=0; for (i=0;i<3;i++) pre[i]=0; i=0} \
{ if( i >=3 ) {  dataOper[(i - i%3)/3, i % 3] = $2 - pre[ i % 3];} pre[i % 3] = $2; i++;}\
END{ num = NR/3 -1 ; for(i = 0; i < num; i++) printf(\"%s %s %s %s\\n\", \
i, dataOper[i,0], dataOper[i,1], dataOper[i,2]);}'"\
using (funx($1)):(funy($2)) lw 2 title " create", '' \
using (funx($1)):(funy($3)) lw 2 title " read" , '' \
using (funx($1)):(funy($4)) lw 2 title " written"

