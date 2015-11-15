#!/bin/bash

./run_against.py -algo ./astar_main.opt -scen myscen/bgmaps.scen,myscen/sc1.scen,myscen/random.scen,myscen/mazes.scen -output results/vs-astar-exptime-5e-6.out -w 1.0,1.25,1.5 -proc 1,2,4,8,16,24,32 -exptime 0.000005
