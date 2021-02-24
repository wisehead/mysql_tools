    set terminal png truecolor
    set output "Average_Service_Time.png"
    set autoscale
    set grid
    set title "Average_Service_Time"
    set xlabel "ITime(sec)"
    set ylabel "milliseconds"
    set style data lines
    sample=1
    funx(x)=(x * sample)
    plot "< egrep '^vd' iostat.out|awk 'BEGIN{i=0} { arr[(i - i%3)/3, i % 3] = $11; i++;} \
    END{ num = NR/3; for(i=0; i<num; i++) \
    printf(\"%s  %s %s %s\\n\", \
    i ,arr[i,0] ,arr[i,1],arr[i,2]);}' " \
     using (funx($1)):2 lw 2 title "vda svctm" , ''   using (funx($1)):3 lw 2 title "vdb svctm",''   using (funx($1)):4 lw 2 title "vdc svctm"
