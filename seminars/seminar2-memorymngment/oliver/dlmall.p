set terminal png
set output "dlmall.png"

set title "With and without merge"

set key right center

set xlabel "tbd"

set ylabel "tbd"

#use log scale if we use doubling of number of pages
#set logscale x 2

plot  "dlmallOrg.dat" u 1:2 w linespoints title "Vanilla", \
      "dlmallMer.dat" u 1:2 w yerrorlines title "Merge", \
      "dlmallOrgImp.dat" u 1:2 w linespoints title "Vanilla improved", \
      "dlmallMerImp.dat" u 1:2 w yerrorlines title "Merge improved"
