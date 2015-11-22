#!/bin/bash

BIG_SCEN=myscen/main/Expedition.map.big-4.scen-10,myscen/main/maze512-4-0.map.big-8.scen-10,myscen/main/AR0602SR.map.big-8.scen-10
SMALL_SCEN=myscen/main/Expedition.map.min-2.scen-10,myscen/main/maze512-4-0.map.scen-10,myscen/main/AR0602SR.map.scen-10
W=1.0,1.1,1.2
PROC=1,4,8,16,32
OUT_ASTAR=results/astar.txt
OUT_PBNF=results/pbnf.txt
OUT_PWSA=results/pwsa.txt

OUT_ASTAR_EXPTIME_1=results/astar_5e-5.txt
OUT_PWSA_EXPTIME_1=results/pwsa_5e-5.txt
OUT_PASE_EXPTIME_1=results/pase_5e-5.txt
OUT_WPANRE_EXPTIME_1=results/wpanre_5e-5.txt

OUT_ASTAR_EXPTIME_2=results/astar_1e-6.txt
OUT_PWSA_EXPTIME_2=results/pwsa_1e-6.txt
OUT_WPANRE_EXPTIME_2=results/wpanre_1e-6.txt

for i in `seq 1 1`;
do
  ./scripts/run_scen.py -scen $BIG_SCEN -algo "wA*" -w $W -proc 1 -output $OUT_ASTAR -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $BIG_SCEN -algo "wPWSA*" -w $W -proc $PROC -output $OUT_PWSA -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $BIG_SCEN -algo "PBNF" -w $W -proc $PROC -output $OUT_PBNF -runs 1 -attempts 3

  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wA*" -w $W -proc $PROC -exptime 0.00005 -output $OUT_ASTAR_EXPTIME_1 -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wPA*SE" -w $W -proc $PROC -exptime 0.00005 -output $OUT_PASE_EXPTIME_1 -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wPWSA*" -w $W -proc $PROC -exptime 0.00005 -output $OUT_PWSA_EXPTIME_1 -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wPA*NRE" -w $W -proc $PROC -exptime 0.00005 -output $OUT_PWSA_EXPTIME_1 -runs 1 -attempts 3

  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wA*" -w $W -proc $PROC -exptime 0.000001 -output $OUT_ASTAR_EXPTIME_2 -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wPWSA*" -w $W -proc $PROC -exptime 0.000001 -output $OUT_PWSA_EXPTIME_2 -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $SMALL_SCEN -algo "wPA*NRE" -w $W -proc $PROC -exptime 0.000001 -output $OUT_PWSA_EXPTIME_2 -runs 1 -attempts 3
done
