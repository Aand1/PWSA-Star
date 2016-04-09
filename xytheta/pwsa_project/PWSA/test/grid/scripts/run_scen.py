#!/usr/bin/python

import sys
import subprocess

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

args = sys.argv[1:]
i = args.index("-scen")
scenfiles = args[i+1].split(",")
args = args[0:i] + args[(i+2):]

for scenfile in scenfiles:
  for scenario in fileGetLines(scenfile)[1:]:
    [_, mapfile, w, h, sc, sr, dc, dr, opt] = scenario.split()
    mapArgs = ["-map", mapfile, "-sr", sr, "-sc", sc, "-dr", dr, "-dc", dc, "-opt", opt]
    # theseArgs = '"' + ' '.join(mapArgs + args) + '"'
    cmd = ' '.join(["prun --append -prog ./scripts/select_algo.py"] + mapArgs + args)
    # cmd = ' '.join(["prun --append"] + mapArgs + args)
    print cmd
    subprocess.call(cmd, shell=True)
