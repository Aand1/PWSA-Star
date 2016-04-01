#!/bin/bash

RESULTS_DIR=results

BIG_SCEN=myscen/main/Expedition.map.big-4.scen-10,myscen/main/maze512-4-0.map.big-8.scen-10,myscen/main/AR0602SR.map.big-8.scen-10
MAIN_BIG_SCEN=myscen/main/Expedition.map.big-4.scen-10,myscen/main/AR0602SR.map.big-8.scen-10
SMALL_SCEN=myscen/main/Expedition.map.min-2.scen-10,myscen/main/maze512-4-0.map.scen-10,myscen/main/AR0602SR.map.scen-10
MAIN_SMALL_SCEN=myscen/main/Expedition.map.min-2.scen-10,myscen/main/AR0602SR.map.scen-10
PBNF_SCEN=myscen/main/Expedition.map.big-4.scen-10,myscen/main/AR0602SR.map.big-8.scen-10
W=1.0,1.1,1.2
W1=1.0
PROC=1,4,8,16,32
OUT=$RESULTS_DIR/pbbs.txt
BIG_ALGO=wA*,wPWSA*,simple_wPWSA*
MAIN_BIG_ALGO=wA*,wPWSA*
SMALL_ALGO=wA*,wPA*NRE,wPA*SE,wPWSA*,simple_wPWSA*
MAIN_SMALL_ALGO=wA*,wPA*NRE,wPA*SE,wPWSA*
PBNF_ALGO=Burns_A*,SafePBNF_4096,SafePBNF_16384,SafePBNF_65536,SafePBNF_262144

MAIN_EXPTIME=0.000005,0.00001,0.00003,0.00005
MAIN_W=1.0,1.1,1.5,2.0

# for i in `seq 1 5`;
# do
#   #./scripts/run_scen.py -scen $BIG_SCEN -algo $BIG_ALGO -w $W -exptime 0.0 -proc $PROC -output $OUT -runs 1 -attempts 3
#   #./scripts/run_scen.py -scen $SMALL_SCEN -algo $SMALL_ALGO -w $W -exptime 0.000001 -proc $PROC -output $OUT -runs 1 -attempts 3
#   #./scripts/run_scen.py -scen $SMALL_SCEN -algo $SMALL_ALGO -w $W -exptime 0.00005 -proc $PROC -output $OUT -runs 1 -attempts 3
#   #./scripts/run_scen.py -scen $SMALL_SCEN -algo $SMALL_ALGO -w $W1 -exptime 0.00001 -proc $PROC -output $OUT -runs 1 -attempts 3
#   #./scripts/run_scen.py -scen $SMALL_SCEN -algo $SMALL_ALGO -w $W1 -exptime 0.000005 -proc $PROC -output $OUT -runs 1 -attempts 3
# done

for i in `seq 1 5`;
do
  ./scripts/run_scen.py -scen $MAIN_BIG_SCEN -algo $MAIN_BIG_ALGO -w $MAIN_W -exptime 0.0 -proc $PROC -output $OUT -runs 1 -attempts 3
  ./scripts/run_scen.py -algo $PBNF_ALGO -scen $PBNF_SCEN -w $W1 -proc $PROC -output $OUT -runs 1 -attempts 3
  ./scripts/run_scen.py -scen $MAIN_SMALL_SCEN -algo $MAIN_SMALL_ALGO -w 1.0 -exptime $MAIN_EXPTIME -proc $PROC -output $OUT -runs 1 -attempts 3
done

#for i in `seq 1 1`;
#do
#  ./scripts/run_scen.py -algo $PBNF_ALGO -scen $PBNF_SCEN -w $W1 -proc $PROC -output $OUT -runs 1 -attempts 3
#done
