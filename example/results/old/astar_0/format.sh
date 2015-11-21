#!/bin/bash

cat results/astar_0/*.txt > astar_proc1.txt

cp astar_proc1.txt astar_proc4.txt
cp astar_proc1.txt astar_proc8.txt
cp astar_proc1.txt astar_proc16.txt
cp astar_proc1.txt astar_proc32.txt

sed -i -e "s/proc 1/proc 4/g" astar_proc4.txt
sed -i -e "s/proc 1/proc 8/g" astar_proc8.txt
sed -i -e "s/proc 1/proc 16/g" astar_proc16.txt
sed -i -e "s/proc 1/proc 32/g" astar_proc32.txt

cat astar_proc*.txt > astar.txt
rm astar_proc*.txt
