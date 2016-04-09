#!/usr/bin/python

import sys
import subprocess
import random

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

i = sys.argv.index("-algo")
algo = sys.argv[i+1]
newargs = sys.argv[1:i] + sys.argv[(i+2):]

i = newargs.index("-scen")
scenfile = newargs[i+1]
newargs = newargs[0:i] + newargs[(i+2):]

scenarios = fileGetLines(scenfile)[1:]
[num, mapfile, w, h, sc, sr, dc, dr, x] = scenarios[random.randint(0,len(scenarios) - 1)].split()

cmd = ' '.join([algo] + newargs + ["-map", mapfile, "-sr", sr, "-sc", sc, "-dr", dr, "-dc", dc])
print cmd
sys.exit(subprocess.call(cmd, shell=True))
