#!/bin/bash

#./scripts/run_scen.py -scen myscen/bgmaps/AR0400SR.map-25.scen -algo "wPA*SE" -w 1.2,1.5,2.0,2.5 -proc 4,8,16,32 -exptime 0.0005 -runs 1 -attempts 3 -output results/pase/AR0400SR.txt
#./scripts/run_scen.py -timeout 120 -scen myscen/bgmaps/AR0400SR.map-25.scen -algo "wPA*SE" -w 2.5 -proc 4,8,16,32 -exptime 0.0005 -runs 1 -attempts 3 -output results/pase/AR0400SR.txt

./scripts/run_scen.py -scen myscen/sc1/AcrosstheCape.map-25.scen -algo "wPA*SE" -w 1.2,1.5,2.0,2.5 -proc 4,8,16,32 -exptime 0.00005 -runs 2 -attempts 3 -output results/pase/AcrosstheCape.txt

./scripts/run_scen.py -scen myscen/sc1/Expedition.map-25.scen -algo "wPA*SE" -w 1.2,1.5,2.0,2.5 -proc 4,8,16,32 -exptime 0.00005 -runs 2 -attempts 3 -output results/pase/Expedition.txt

./scripts/run_scen.py -scen myscen/mazes/maze512-4-0.map-25.scen -algo "wPA*SE" -w 1.2,1.5,2.0,2.5 -proc 4,8,16,32 -exptime 0.00005 -runs 2 -attempts 3 -output results/pase/maze512-4-0.txt

./scripts/run_scen.py -scen myscen/random/random512-25-0.map-25.scen -algo "wPA*SE" -w 1.2,1.5,2.0,2.5 -proc 4,8,16,32 -exptime 0.00005 -runs 2 -attempts 3 -output results/pase/random512-25-0.txt 
