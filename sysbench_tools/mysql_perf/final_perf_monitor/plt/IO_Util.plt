    set terminal png truecolor
    set output "IO_Util.png"
    set autoscale
    set grid
    set title "IO_Util"
    set xlabel "ITime(sec)"
    set ylabel "%"
    set style data lines
    sample=1
    funx(x)=(x * sample)
    plot "< egrep '^vdc' iostat.out|awk 'BEGIN{i=0} { arr[i, 0] = $12; i++;} \
    END{ num = NR; for(i=0; i<num; i++) \
    printf(\"%s  %s\\n\", \
    i ,arr[i,0]);}' " \
     using (funx($1)):2 lw 2 title "CDS util" 
