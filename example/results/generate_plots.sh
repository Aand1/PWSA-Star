#!/bin/bash

PROC=( 1 4 8 16 32 )

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

GEN_DIR=results/gen

if [ ! -d "$GEN_DIR" ]; then
  mkdir $GEN_DIR
fi

# copy astar results many times
for proc in "${PROC[@]}";
do
  cat $OUT_ASTAR $OUT_ASTAR_EXPTIME_1 $OUT_ASTAR_EXPTIME_2 > $GEN_DIR/astar_$proc.txt
  sed -i -e "s/proc 1/proc $proc/g" $GEN_DIR/astar_$proc.txt
done

cat $GEN_DIR/astar_*.txt $OUT_PBNF $OUT_PWSA $OUT_PWSA_EXPTIME_1 $OUT_PWSA_EXPTIME_2 $OUT_PASE_EXPTIME_1 $OUT_WPANRE_EXPTIME_1 $OUT_WPANRE_EXPTIME_2 > $GEN_DIR/all.txt

./scripts/reformat.py -keep "{'exptime':['0.0']}" -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_astar_exptime_0.txt
./scripts/reformat.py -keep "{'exptime':['0.0']}" -compare wPWSA*,PBNF -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_pbnf_exptime_0.txt

pplot scatter -x proc -y speedup -ymin 1 -ymax 32 -series w -chart map,algo -input $GEN_DIR/pwsa_astar_exptime_0.txt -output $GEN_DIR/speedup_vs_astar_exptime_0.pdf
pplot scatter -x proc -y speedup -ymin 1 -series w -chart map,algo -input $GEN_DIR/pwsa_pbnf_exptime_0.txt -output $GEN_DIR/speedup_vs_pbnf_exptime_0.pdf



./scripts/reformat.py -keep "{'exptime':['0.00005']}" -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_astar_exptime_5e-5.txt
./scripts/reformat.py -keep "{'exptime':['0.00005']}" -compare wPA*SE,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pase_astar_exptime_5e-5.txt
./scripts/reformat.py -keep "{'exptime':['0.00005']}" -compare wPA*NRE,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/panre_astar_exptime_5e-5.txt
cat $GEN_DIR/pwsa_astar_exptime_5e-5.txt $GEN_DIR/pase_astar_exptime_5e-5.txt $GEN_DIR/panre_astar_exptime_5e-5.txt > $GEN_DIR/pwsa_pase_panre_astar_exptime_5e-5.txt
pplot scatter -x proc -y speedup -ymin 1 -ymax 32 -series algo -chart map,w -input $GEN_DIR/pwsa_pase_panre_astar_exptime_5e-5.txt -output speedups_vs_astar_exptime_5e-5.pdf


./scripts/reformat.py -keep "{'exptime':['0.000001']}" -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_astar_exptime_1e-6.txt
./scripts/reformat.py -keep "{'exptime':['0.000001']}" -compare wPA*NRE,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/panre_astar_exptime_1e-6.txt
cat $GEN_DIR/pwsa_astar_exptime_1e-6.txt $GEN_DIR/panre_astar_exptime_1e-6.txt > $GEN_DIR/pwsa_panre_astar_exptime_1e-6.txt
pplot scatter -x proc -y speedup -ymin 1 -ymax 32 -series algo -chart map,w -input $GEN_DIR/pwsa_panre_astar_exptime_1e-6.txt -output speedups_vs_astar_exptime_1e-6.pdf
