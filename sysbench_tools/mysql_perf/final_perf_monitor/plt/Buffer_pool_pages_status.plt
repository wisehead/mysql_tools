set terminal png truecolor
set output "Buffer_pool_pages.png"
set autoscale
set grid
set title "Buffer pool pages"
set xlabel "time(s)"
set ylabel "buffer pool pages status(pages)"
set style data lines
sample=1
funx(x)=(sample * x)
plot "< egrep 'Innodb_buffer_pool_pages_total|Innodb_buffer_pool_pages_misc|Innodb_buffer_pool_pages_data' global_0.out | \
awk 'BEGIN{i=0} {pageArr[(i-i%3)/3, i%3] = $2; i++;} END{num=NR/3; for(i=0;i<num;i++) \
print i \" \" pageArr[i,0] \" \" pageArr[i,1] \" \" pageArr[i,2] ;}' "\
using (funx($1)):2 lw 2 title "Data pages", ''\
using (funx($1)):3 lw 2 title "Misc pages", ''\
using (funx($1)):4 lw 2 title "Total pages" 
