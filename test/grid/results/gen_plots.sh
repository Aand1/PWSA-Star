#!/bin/bash

RESULTS=results/pbbs.txt

# plot exptime=0, w=1: A*, burns_A*, safePBNF, PWSA*
./scripts/reformat.py -keep "{'exptime':['0.0'],'w':['1.0'],'map':['maps/sc1/Expedition.map.big-4']}" -exclude "{'algo':['simple_wPWSA*','Burns_A*']}" -input $RESULTS -output results/temp.txt
pplot scatter -x proc -y exectime -series algo -chart map -input results/temp.txt -output results/exptime0_w1_exectime.pdf

# plot exptime=0, w=1.0,1.5,2.0: wA*, wPWSA*
./scripts/reformat.py -keep "{'algo':['wA*','wPWSA*'],'exptime':['0.0'],'w':['1.0','1.5','2.0'],'map':['maps/sc1/Expedition.map.big-4','maps/bgmaps/AR0602SR.map.big-8']}" -input $RESULTS -output results/temp.txt
pplot scatter -x proc -y exectime -series algo,w -chart map -input results/temp.txt -output results/exptime0_w1-2_exectime.pdf
pplot scatter -x proc -y deviation -series algo,w -chart map -input results/temp.txt -output results/exptime0_w1-2_deviation.pdf

# plot exptime=0.000005,0.00001,0.00003,0.00005, w=1.0: A*, PWSA*, PA*SE, PA*NRE
./scripts/reformat.py -keep "{'algo':['wA*','wPWSA*','wPA*SE','wPA*NRE'],'exptime':['0.00001'],'w':['1.0'],'map':['maps/sc1/Expedition.map.min-2','maps/bgmaps/AR0602SR.map']}" -input $RESULTS -output results/temp.txt
pplot scatter -x proc -y exectime -series algo -chart map,exptime -input results/temp.txt -output results/exptime_many_w1_exectime.pdf

rm -f results/temp.txt
rm null
