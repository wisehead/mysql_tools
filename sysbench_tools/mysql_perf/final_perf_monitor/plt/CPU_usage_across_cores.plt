    set terminal png truecolor
    set output "CPU_usage_across_cores.png"
    set autoscale
    set grid
    set title "CPU_usage_across_cores"
    set xlabel "ITime(sec)"
    set ylabel "{%user+%sys}"
    set style data lines
    sample=1
    funx(x)=(x * sample)
    plot "< egrep ^[0-9] mpstat.out | grep -v CPU | grep -v all | awk 'BEGIN{i=0} { arr[(i - i%8)/8, i % 8] = $4 + $6; i++;} \
    END{ num = NR/8; for(i=0; i<num; i++) \
    printf(\"%s  %s %s %s %s %s %s %s %s\\n\", \
    i ,arr[i,0] ,arr[i,1] ,arr[i,2] ,arr[i,3] ,arr[i,4] ,arr[i,5] ,arr[i,6] ,arr[i,7]);}' " \
     using (funx($1)):2 lw 2 title "cpu0" , ''   using (funx($1)):3 lw 2 title "cpu1" , ''   using (funx($1)):4 lw 2 title "cpu2" , ''   using (funx($1)):5 lw 2 title "cpu3" , ''   using (funx($1)):6 lw 2 title "cpu4" , ''   using (funx($1)):7 lw 2 title "cpu5" , ''   using (funx($1)):8 lw 2 title "cpu6" , ''   using (funx($1)):9 lw 2 title "cpu7"
