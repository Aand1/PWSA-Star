#!/bin/bash

SCEN=myscen/main/Expedition.map.big-4.scen-10,myscen/main/maze512-4-0.map.big-4.scen-10,myscen/main/AR0602SR.map.big-4.scen-10
#SCEN=myscen/bgmaps/AR0402SR.map.big-4.scen-10
W=1.0,1.2,1.5,2.0
PROC=1,4,8,16,32
OUT_ASTAR=results/astar/astar.txt
OUT_PBNF=results/pbnf/pbnf.txt
OUT_PWSA=results/pwsa/pwsa.txt

for i in `seq 1 2`;
do
  ./scripts/run_scen.py -scen $SCEN -algo "wA*" -w $W -proc 1 -output $OUT_ASTAR -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SCEN -algo "wPWSA*" -w $W -proc $PROC -output $OUT_PWSA -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SCEN -algo "PBNF" -w $W -proc $PROC -output $OUT_PBNF -runs 1 -attempts 3
done
