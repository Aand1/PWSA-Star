#!/usr/bin/python

import sys
import subprocess

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

args = sys.argv[1:]
i = args.index("-scen")
scenfile = args[i+1]
args = args[0:i] + args[(i+2):]

for scenario in fileGetLines(scenfile)[1:]:
  [_, mapfile, w, h, sc, sr, dc, dr, _] = scenario.split()
  mapArgs = ["-map", mapfile, "-sr", sr, "-sc", sc, "-dr", dr, "-dc", dc]
  # theseArgs = '"' + ' '.join(mapArgs + args) + '"'
  cmd = ' '.join(["prun"] + mapArgs + args)
  print cmd
  subprocess.call(cmd, shell=True)
