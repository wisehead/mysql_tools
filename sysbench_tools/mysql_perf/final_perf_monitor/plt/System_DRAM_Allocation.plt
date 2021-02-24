set terminal png truecolor
set output "System_DRAM_Allocation.png"
set autoscale
set grid
set title "System_DRAM_Allocation"
set xlabel "VTime(sec)"
set ylabel "MBytes"
set style data lines
sample=1
funx(x)=( sample * x)
plot "< grep -v swap vmstat.out|grep -v swpd|awk 'BEGIN{i=0} {print i \" \" $3/1000 \" \" $4/1000 \" \" $5/1000 \" \" $6/1000 \" \"; i++}'" \
using (funx($1)):2 lw 2 title "swap" , '' using (funx($1)): 3 lw 2 title "free",''  using (funx($1)):4 lw 2 title "buff", ''using (funx($1)):5 lw 2 title "cache"
