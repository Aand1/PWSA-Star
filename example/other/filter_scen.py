#!/usr/bin/python

import sys
import tempfile
import subprocess

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

#topPercentile = float(sys.argv[1])
topNum = int(sys.argv[1])
infile = sys.argv[2]
outfile = sys.argv[3]

lines = fileGetLines(infile)
scenarios = []
#with open(outfile,"w") as fout:
#  fout.write(lines[0] + "\n")
for line in lines[1:]:
  [scenNum, mapName, width, height, sc, sr, dc, dr, x] = line.split()
  cmd = "./pwsa_main.opt -map %s -sr %s -sc %s -dr %s -dc %s -proc 1 -exptime 0.0" % (mapName, sr, sc, dr, dc)
  with tempfile.TemporaryFile() as ftemp:
    print cmd
    subprocess.call(cmd, stdout=ftemp, stderr=ftemp, shell=True)
    ftemp.seek(0)
    for line in ftemp.readlines():
      [key, value] = line.split()
      if key == "pathlen":
        scenarios.append((scenNum, mapName, width, height, sc, sr, dc, dr, value))
        break
        #fout.write("%s %s %s %s %s %s %s %s %s\n" % (scenNum, mapName, width, height, sc, sr, dc, dr, value))

scenarios.sort(key=lambda x: float(x[8]))
scenarios.reverse()
with open(outfile,"w") as fout:
  fout.write(lines[0] + "\n")
#  for scenario in scenarios[:int(len(scenarios)*topPercentile)]:
  for scenario in scenarios[:topNum]:
    fout.write("%s %s %s %s %s %s %s %s %s\n" % scenario)
