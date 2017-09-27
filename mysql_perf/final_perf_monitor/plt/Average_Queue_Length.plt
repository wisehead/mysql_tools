    set terminal png truecolor
    set output "Average_Queue_Length.png"
    set autoscale
    set grid
    set title "Average_Queue_Length"
    set xlabel "ITime(sec)"
    set ylabel ""
    set style data lines
    sample=1
    funx(x)=(x * sample)
    plot "< egrep '^vd' iostat.out|awk 'BEGIN{i=0} { arr[(i - i%3)/3, i % 3] = $9;i++;} \
    END{ num = NR/3; for(i=0; i<num; i++) \
    printf(\"%s  %s %s %s\\n\", \
    i ,arr[i,0] ,arr[i,1],arr[i,2]);}' " \
     using (funx($1)):2 lw 2 title "vda avgqu-sz" , ''   using (funx($1)):3 lw 2 title "vdb avgqu-sz", ''   using (funx($1)):4 lw 2 title "vdc avgqu-sz"
