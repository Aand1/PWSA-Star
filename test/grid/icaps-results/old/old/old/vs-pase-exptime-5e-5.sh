#!/bin/bash

./run_against.py -algo ./pase_main.opt -scen myscen/bgmaps.scen,myscen/sc1.scen,myscen/random.scen,myscen/mazes.scen -output results/vs-pase-exptime-5e-5.out -w 1.0,1.25,1.5 -proc 1,2,4,8,16,24,32 -exptime 0.00005
