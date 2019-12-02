set terminal png
set output "tlb.png"

set title "TLB benchmark (10 Mi operations)"

set key right center
    
set xlabel "number of pages"
    
set ylabel "time in s"

set logscale x 2    

plot "bench.dat" u 1:2 w linespoints  title "page size 4K bytes",  \
