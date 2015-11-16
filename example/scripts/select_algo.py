#!/usr/bin/python

import sys
import subprocess

i = sys.argv.index("-algo")
algo = sys.argv[i+1]
newargs = sys.argv[1:i] + sys.argv[(i+2):]

algos = { "wPWSA*"   : "./pwsa_main.opt -pathcorrect 0"
        , "wPWSA*PC" : "./pwsa_main.opt -pathcorrect 1"
        , "wPA*SE"   : "./pase_main.opt"
        , "wA*"      : "./astar_main.opt"
        , "wPBNF"    : "./scripts/run_pbnf.py"
        }

executable = algos[algo]

cmd = ' '.join([executable] + newargs)
#print cmd
sys.exit(subprocess.call(cmd, shell=True))
