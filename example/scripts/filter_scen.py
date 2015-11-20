#!/usr/bin/python

import sys
import tempfile
import subprocess

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

#topPercentile = float(sys.argv[1])
topNum = int(sys.argv[1])
blowup = int(sys.argv[2])
infile = sys.argv[3]
outfile = sys.argv[4]

lines = fileGetLines(infile)
scenarios = []
#with open(outfile,"w") as fout:
#  fout.write(lines[0] + "\n")
for line in lines[1:]:
  [scenNum, mapName, width, height, sc, sr, dc, dr, x] = line.split()
  cmd = "./astar_main.opt -map %s -sr %d -sc %d -dr %d -dc %d -proc 1 -exptime 0.0" % (mapName, int(sr)*blowup, int(sc)*blowup, int(dr)*blowup, int(dc)*blowup)
  with tempfile.TemporaryFile() as ftemp:
    print cmd
    subprocess.call(cmd, stdout=ftemp, stderr=ftemp, shell=True)
    ftemp.seek(0)
    for line in ftemp.readlines():
      [key, value] = line.split()
      if key == "pathlen":
        scenarios.append((scenNum, mapName, width, height, int(sc)*blowup, int(sr)*blowup, int(dc)*blowup, int(dr)*blowup, value))
        break
        #fout.write("%s %s %s %s %s %s %s %s %s\n" % (scenNum, mapName, width, height, sc, sr, dc, dr, value))

scenarios.sort(key=lambda x: float(x[8]))
scenarios.reverse()
with open(outfile,"w") as fout:
  fout.write(lines[0] + "\n")
#  for scenario in scenarios[:int(len(scenarios)*topPercentile)]:
  for scenario in scenarios[:topNum]:
    fout.write("%s %s %s %s %d %d %d %d %s\n" % scenario)
