set terminal png truecolor 
set output "disk_Bandwidth.png"
set autoscale
set grid
set title "disk_Bandwidth"
set xlabel "ITime(sec)"
set ylabel "KBytes/s"
set style data lines
sample=1
funx(x)=(x * sample)
plot "< egrep '^vdc' iostat.out|awk 'BEGIN{i=0} { arr[i, 0] = $6; arr[i,1]=$7; i++;} \
END{ num = NR/1; for(i=0; i<num; i++) \
printf(\"%s  %s %s\\n\", \
i, arr[i,0], arr[i,1]);}' " \
using (funx($1)):2 lw 2 title "cds Read", '' using (funx($1)):3 lw 2 title "cds Write"
