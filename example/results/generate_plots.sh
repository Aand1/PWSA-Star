#!/bin/bash

PROC=( 1 4 8 16 32 )

ASTAR_RESULTS=results/astar.txt
PBNF_RESULTS=results/pbnf.txt
PWSA_RESULTS=results/pwsa.txt

# PASE_RESULTS_EXPTIME=results/pase_5e-5.txt
# PWSA_RESULTS_EXPTIME=results/pwsa_5e-5.txt

GEN_DIR=results/gen

if [ ! -d "$GEN_DIR" ]; then
  mkdir $GEN_DIR
fi

# copy astar results many times
for proc in "${PROC[@]}";
do
  cp $ASTAR_RESULTS $GEN_DIR/astar_$proc.txt
  sed -i -e "s/proc 1/proc $proc/g" $GEN_DIR/astar_$proc.txt
done

cat $GEN_DIR/astar_*.txt $PBNF_RESULTS $PWSA_RESULTS > $GEN_DIR/all.txt

./scripts/reformat.py -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa-astar.txt
./scripts/reformat.py -compare wPWSA*,PBNF -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa-pbnf.txt

pplot scatter -x proc -y speedup -ymin 1 -ymax 32 -series w -chart map,algo -input $GEN_DIR/pwsa-astar.txt -output $GEN_DIR/speedup-vs-astar.pdf
pplot scatter -x proc -y speedup -ymin 1 -series w -chart map,algo -input $GEN_DIR/pwsa-pbnf.txt -output $GEN_DIR/speedup-vs-pbnf.pdf
