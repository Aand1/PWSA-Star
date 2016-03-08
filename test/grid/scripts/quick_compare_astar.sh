#!/bin/bash

FILE=temp_quick_compare_vs_astar.txt
#SCEN=myscen/sc1/AcrosstheCape.map-1.scen,myscen/sc1/Expedition.map-1.scen,myscen/mazes/maze512-4-0.map-1.scen,myscen/random/random512-25-0.map-1.scen
SCEN=myscen/sc1/Expedition.big4.map-1.scen
W=1.0,1.1
PROC=4,8,16,32
ALGO="wPWSA*","wPWSA*PC","wA*"

./scripts/run_scen.py -scen $SCEN -algo $ALGO -w $W -proc $PROC -output $FILE
./scripts/reformat.py -compare "wPWSA*","wA*" -input $FILE -output pwsa-$FILE
./scripts/reformat.py -compare "wPWSA*PC","wA*" -input $FILE -output pwsapc-$FILE
./scripts/reformat.py -keep "{'algo':['wPWSA*','wPWSA*PC']}" -input $FILE -output dev-$FILE

cat pwsa-$FILE pwsapc-$FILE > final-$FILE
pplot scatter -x proc -y speedup -series algo -chart map,w -input final-$FILE -ymin 1 -ymax 32 -output plots-speedup.pdf
pplot scatter -x proc -y deviation -series algo,w -chart map -input dev-$FILE -ymin 1 -output plots-deviation.pdf

rm $FILE pwsa-$FILE pwsapc-$FILE final-$FILE dev-$FILE
