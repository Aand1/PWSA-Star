#!/bin/bash

PROC=( 1 4 8 16 32 )

for proc in ${PROC[@]};
do
  cp results/astar/astar.txt temp_astar_proc$proc.txt
  sed -i -e "s/proc 1/proc $proc/g" temp_astar_proc$proc.txt
done

cat temp_astar_proc*.txt > astar.txt
rm temp_astar_proc*.txt
