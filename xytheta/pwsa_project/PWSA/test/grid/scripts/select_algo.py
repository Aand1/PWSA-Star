#!/usr/bin/python

import sys
import subprocess

i = sys.argv.index("-algo")
algo = sys.argv[i+1]
newargs = sys.argv[1:i] + sys.argv[(i+2):]

algos = { "wPWSA*"          : "./grid_main.opt -algo wPWSA*"
        , "simple_wPWSA*"   : "./grid_main.opt -algo simple_wPWSA*"
        , "wPA*SE"          : "./grid_main.opt -algo wPA*SE"
        , "wA*"             : "./grid_main.opt -algo wA*"
        , "wPA*NRE"         : "./grid_main.opt -algo wPA*NRE"
        , "Burns_A*"        : "./scripts/run_pbnf.py -algo astar"
        , "SafePBNF_4096"   : "./scripts/run_pbnf.py -algo safepbnf -nblocks 4096"
        , "SafePBNF_16384"  : "./scripts/run_pbnf.py -algo safepbnf -nblocks 16384"
        , "SafePBNF_65536"  : "./scripts/run_pbnf.py -algo safepbnf -nblocks 65536"
        , "SafePBNF_262144" : "./scripts/run_pbnf.py -algo safepbnf -nblocks 262144"
        }

executable = algos[algo]

cmd = ' '.join([executable] + newargs)
#print cmd
sys.exit(subprocess.call(cmd, shell=True))
