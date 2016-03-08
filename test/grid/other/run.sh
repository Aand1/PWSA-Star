#!/bin/bash

#./run_against.py -algo ./astar_main.opt,./pase_main.opt -scen myscen/bgmaps.scen,myscen/sc1.scen,myscen/random.scen,myscen/mazes.scen -output results/exptime-5e-5.out -runs 2 -w 1.0,1.25,1.5 -proc 1,2,4,8,16,24,32 -exptime 0.00005

#./run_against.py -algo ./astar_main.opt,./pase_main.opt -scen myscen/bgmaps.scen,myscen/sc1.scen,myscen/random.scen,myscen/mazes.scen -output results/exptime-5e-6.out -runs 2 -w 1.0,1.25,1.5 -proc 1,2,4,8,16,24,32 -exptime 0.000005

#./run_against.py -algo ./astar_main.opt,./pase_main.opt -scen myscen/bgmaps.scen,myscen/sc1.scen,myscen/random.scen,myscen/mazes.scen -output results/exptime-0.out -runs 2 -w 1.0,1.25,1.5 -proc 1,2,4,8,16,24,32 -exptime 0.0

./run_against.py -algo ./pase_main.opt -scen myscen/sc1.scen -output results/vs-pase-exptime-5e-5.out -w 1.0,1.25,1.5 -proc 2,4,8,16,24,32 -exptime 0.00005

cp results/vs-pase-exptime-5e-5.out results/backup/vs-pase-exptime-5e-5-after-sc1.out

./run_against.py -algo ./pase_main.opt -scen myscen/mazes.scen -output results/vs-pase-exptime-5e-5.out -w 1.0,1.25,1.5 -proc 2,4,8,16,24,32 -exptime 0.00005

cp results/vs-pase-exptime-5e-5.out results/backup/vs-pase-exptime-5e-5-after-mazes.out

./run_against.py -algo ./pase_main.opt -scen myscen/random.scen -output results/vs-pase-exptime-5e-5.out -w 1.0,1.25,1.5 -proc 2,4,8,16,24,32 -exptime 0.00005

cp results/vs-pase-exptime-5e-5.out results/backup/vs-pase-exptime-5e-5-after-random.out
