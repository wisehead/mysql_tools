set terminal png truecolor 
set output "disk_TPS.png"
set autoscale
set grid
set title "disk_TPS"
set xlabel "ITime(sec)"
set ylabel "request/s"
set style data lines
sample=1
funx(x)=(x * sample)
plot "< egrep '^vd' iostat.out|awk 'BEGIN{i=0} { arr[(i-i%3)/3, i%3] = $4; arr_2[(i-i%3)/3,i%3]=$5;  i++;} \
END{ num = NR/3; for(i=0; i<num; i++) \
printf(\"%s  %s %s %s %s %s %s\\n\", \
i, arr[i,0], arr[i,1], arr[i,2], arr_2[i,0],arr_2[i,1],arr_2[i,2]);}' " \
using (funx($1)):2 lw 2 title "vda read", '' using (funx($1)):3 lw 2 title "vdb read" ,'' using (funx($1)):4 lw 2 title "vdc read", '' using (funx($1)):5 lw 2 title "vda write",'' using (funx($1)):6 lw 2 title "vdb write", '' using (funx($1)):7 lw 2 title "vdc write"
