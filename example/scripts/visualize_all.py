#!/usr/bin/python

import sys
import subprocess
import os

algo = "./pwsa_main.opt"

args = sys.argv[1:]

scenIdx = args.index("-scen")
scenfile = args[1 + scenIdx]
args = args[:scenIdx] + args[(scenIdx+2):]

outdirIdx = args.index("-outdir")
outdir = args[1 + outdirIdx]
args = args[:outdirIdx] + args[(outdirIdx+2):]

if not os.path.isdir(outdir):
  os.makedirs(outdir)

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

i = 0
for scenario in fileGetLines(scenfile)[1:]:
  [scenNum, mapName, width, height, sc, sr, dc, dr, opt] = scenario.split()
  mapArgs = "-map %s -sr %s -sc %s -dr %s -dc %s" % (mapName, sr, sc, dr, dc)
  visualizeArg = "-visualize %s/%d.out" % (outdir, i)
  cmd = ' '.join([algo] + args) + " " + mapArgs + " " + visualizeArg
  print cmd
  subprocess.call(cmd, shell=True)
  subprocess.call("./view.py %s/%d.out %s/%d.png" % (outdir, i, outdir, i), shell=True)
  subprocess.call("rm %s/%d.out" % (outdir, i), shell=True)
  i += 1
