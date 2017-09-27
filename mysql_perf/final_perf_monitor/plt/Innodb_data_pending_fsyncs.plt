set terminal png truecolor
set output "Innodb_data_pending_fyncs.png"
set autoscale
set grid
set title "Innodb_data_pending_fsync"
set xlabel "VTime(s)"
set xlabel "fsyncs request"
set style data lines
sample=1
funx(x)=(sample * x)
plot "< grep 'Innodb_data_pending_fsyncs' global_0.out |awk 'BEGIN{ i=0} {print i \" \" $2; i++};' " using (funx($1)) :2 lw 2 title "innodb_data_pending_fsyncs"
