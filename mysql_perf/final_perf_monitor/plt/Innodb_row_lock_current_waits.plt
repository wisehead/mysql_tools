set terminal png truecolor
set output "Innodb_row_lock_current_waits.png"
set autoscale
set grid
set title "Innodb_row_lock_current_waits"
set xlabel "VTime(s)"
set xlabel "waints"
set style data lines
sample=1
funx(x)=(sample * x)
plot "< grep 'Innodb_row_lock_current_waits' global_0.out |awk 'BEGIN{ i=0} {print i \" \" $2; i++};' " using (funx($1)) :2 lw 2 title "Innodb_row_lock_current_waits"
