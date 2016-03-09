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

OUT_ROBOT_WASTAR_15=results/robot/w1.5_runs_wastar_reformatted.txt
OUT_ROBOT_WASTAR_30=results/robot/w3.0_runs_wastar_reformatted.txt
OUT_ROBOT_RUNS_15=results/robot/w1.5_runs_reformatted.txt
OUT_ROBOT_RUNS_30=results/robot/w3.0_runs_reformatted.txt

OUT_ROBOT_WASTAR=results/robot/wastar.txt
OUT_ROBOT_PWSA_PASE=results/robot/pwsa-and-pase.txt

ROBOT_PROC=( 2 4 8 16 24 32 )

if [ ! -d "$GEN_DIR" ]; then
  mkdir $GEN_DIR
fi

# copy astar results many times
for proc in "${PROC[@]}";
do
  # cat $OUT_ASTAR $OUT_ASTAR_EXPTIME_1 $OUT_ASTAR_EXPTIME_2 > $GEN_DIR/astar_$proc.txt
  cat $OUT_ASTAR > $GEN_DIR/astar_$proc.txt
  sed -i -e "s/proc 1/proc $proc/g" $GEN_DIR/astar_$proc.txt

done

# cat $GEN_DIR/astar_*.txt $OUT_PBNF $OUT_PWSA $OUT_PWSA_EXPTIME_1 $OUT_PWSA_EXPTIME_2 $OUT_PASE_EXPTIME_1 $OUT_WPANRE_EXPTIME_1 $OUT_WPANRE_EXPTIME_2 > $GEN_DIR/all.txt
cat $GEN_DIR/astar_*.txt $OUT_PBNF $OUT_PWSA > $GEN_DIR/all.txt

./scripts/reformat.py -keep "{'exptime':['0.0']}" -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_astar_exptime_0.txt
./scripts/reformat.py -keep "{'exptime':['0.0']}" -compare wPWSA*,PBNF -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_pbnf_exptime_0.txt

pplot scatter -x proc -xlabel "number of processors (threads)" -y speedup -ymin 1 -ymax 32 -series w -chart map,algo -input $GEN_DIR/pwsa_astar_exptime_0.txt -output $GEN_DIR/speedup_vs_astar_exptime_0.pdf
pplot scatter -x proc -xlabel "number of processors (threads)" -y speedup -ymin 1 -legend-pos bottomright -series w -chart map,algo -input $GEN_DIR/pwsa_pbnf_exptime_0.txt -output $GEN_DIR/speedup_vs_pbnf_exptime_0.pdf

./scripts/reformat.py -keep "{'exptime':['0.0']}" -input $GEN_DIR/all.txt -output $GEN_DIR/all_shrunk.txt
pplot scatter -x proc -xlabel "number of processors (threads)" -y deviation -series w -chart map,algo -input $GEN_DIR/all_shrunk.txt -output $GEN_DIR/deviation.pdf


cat $OUT_ROBOT_WASTAR_15 $OUT_ROBOT_WASTAR_30 > $OUT_ROBOT_WASTAR
cat $OUT_ROBOT_RUNS_15 $OUT_ROBOT_RUNS_30 > $OUT_ROBOT_PWSA_PASE

for proc in "${ROBOT_PROC[@]}";
do

  cat $OUT_ROBOT_WASTAR > $GEN_DIR/robot_astar_$proc.txt
  sed -i -e "s/proc 1/proc $proc/g" $GEN_DIR/robot_astar_$proc.txt

done

cat $GEN_DIR/robot_astar_*.txt $OUT_ROBOT_PWSA_PASE > $GEN_DIR/robot_all.txt
./scripts/reformat_robot.py -compare pwsa,wastar -input $GEN_DIR/robot_all.txt -output $GEN_DIR/robot_pwsa_wastar.txt
./scripts/reformat_robot.py -compare pase,wastar -input $GEN_DIR/robot_all.txt -output $GEN_DIR/robot_pase_wastar.txt
cat $GEN_DIR/robot_pwsa_wastar.txt $GEN_DIR/robot_pase_wastar.txt > $GEN_DIR/robot_speedups.txt
#pplot scatter -x proc -xlabel "number of processors (threads)" -y speedup -ymin 1 -ymax 32 -series algo -chart w -input $GEN_DIR/robot_speedups.txt -output $GEN_DIR/robot_speedups.pdf

./scripts/reformat_robot.py -keep "{'algo':['pwsa','pase'],'w':['1.5']}" -input $GEN_DIR/robot_all.txt -output $GEN_DIR/robot_dev.txt
#pplot scatter -x proc -xlabel "number of processors (threads)" -y deviation -series algo -input $GEN_DIR/robot_dev.txt -output $GEN_DIR/robot_deviation.pdf

# ./scripts/reformat.py -keep "{'exptime':['0.00005']}" -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_astar_exptime_5e-5.txt
# ./scripts/reformat.py -keep "{'exptime':['0.00005']}" -compare wPA*SE,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pase_astar_exptime_5e-5.txt
# ./scripts/reformat.py -keep "{'exptime':['0.00005']}" -compare wPA*NRE,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/panre_astar_exptime_5e-5.txt
# cat $GEN_DIR/pwsa_astar_exptime_5e-5.txt $GEN_DIR/pase_astar_exptime_5e-5.txt $GEN_DIR/panre_astar_exptime_5e-5.txt > $GEN_DIR/pwsa_pase_panre_astar_exptime_5e-5.txt
# pplot scatter -x proc -y speedup -ymin 1 -ymax 32 -series algo -chart map,w -input $GEN_DIR/pwsa_pase_panre_astar_exptime_5e-5.txt -output speedups_vs_astar_exptime_5e-5.pdf
#
#
# ./scripts/reformat.py -keep "{'exptime':['0.000001']}" -compare wPWSA*,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/pwsa_astar_exptime_1e-6.txt
# ./scripts/reformat.py -keep "{'exptime':['0.000001']}" -compare wPA*NRE,wA* -input $GEN_DIR/all.txt -output $GEN_DIR/panre_astar_exptime_1e-6.txt
# cat $GEN_DIR/pwsa_astar_exptime_1e-6.txt $GEN_DIR/panre_astar_exptime_1e-6.txt > $GEN_DIR/pwsa_panre_astar_exptime_1e-6.txt
# pplot scatter -x proc -y speedup -ymin 1 -ymax 32 -series algo -chart map,w -input $GEN_DIR/pwsa_panre_astar_exptime_1e-6.txt -output speedups_vs_astar_exptime_1e-6.pdf
