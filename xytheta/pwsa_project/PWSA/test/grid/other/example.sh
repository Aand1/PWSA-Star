#!/bin/bash

# speedup
prun speedup -baseline "./astar_main.opt" -parallel "./pwsa_main.opt -proc 2,4,8,12,16" -map maps/mazes/maze512-4-0.map -sr 1 -sc 1 -dr 511 -dc 511 -K 80 -D 10 -exptime 0.000005

# direct comparison
prun -prog ./pase_main.opt,./pwsa_main.opt -map maps/mazes/maze512-4-0.map -sr 1 -sc 1 -dr 511 -dc 511 -K 80 -D 10 -exptime 0.000005 -proc 4
