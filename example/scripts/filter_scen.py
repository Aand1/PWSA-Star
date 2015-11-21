#!/usr/bin/python

import sys
import os
import tempfile
import subprocess

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

def blowup(factor, infile, outfile):
  lines = fileGetLines(infile)
  grid = map(lambda line: list(line), lines[4:])
  print "height: %d" % len(grid)
  print "width: %d" % len(grid[0])

  with open(outfile, "w") as fout:
    fout.write(lines[0] + "\n")

    [_, height] = lines[1].split(' ')
    height = int(height)
    [_, width] = lines[2].split(' ')
    width = int(width)

    fout.write("height %d\n" % (height * factor))
    fout.write("width %d\n" % (width * factor))
    fout.write("map\n")

    for row in xrange(0, height * factor):
      for col in xrange(0, width * factor):
        fout.write(grid[row / factor][col / factor])
      fout.write("\n")

#topPercentile = float(sys.argv[1])
topNum = int(sys.argv[1])
factor = int(sys.argv[2])
infile = sys.argv[3]
outfile = sys.argv[4]

lines = fileGetLines(infile)
scenarios = []

ext = ".big-" + str(factor)

for line in lines[1:]:
  [scenNum, mapName, width, height, sc, sr, dc, dr, x] = line.split()
  if factor > 1 and mapName[-6:] != ext:
    if not os.path.exists(mapName + ext):
      print "Generating %s from %s" % (mapName + ext, mapName)
      blowup(factor, mapName, mapName + ext)
    mapName = mapName + ext
  cmd = "./astar_main.opt -map %s -sr %d -sc %d -dr %d -dc %d -proc 1 -exptime 0.0" % (mapName, int(sr)*factor, int(sc)*factor, int(dr)*factor, int(dc)*factor)
  with tempfile.TemporaryFile() as ftemp:
    print cmd
    subprocess.call(cmd, stdout=ftemp, stderr=ftemp, shell=True)
    ftemp.seek(0)
    for line in ftemp.readlines():
      [key, value] = line.split()
      if key == "pathlen":
        scenarios.append((scenNum, mapName, int(width)*factor, int(height)*factor, int(sc)*factor, int(sr)*factor, int(dc)*factor, int(dr)*factor, value))
        break
        #fout.write("%s %s %s %s %s %s %s %s %s\n" % (scenNum, mapName, width, height, sc, sr, dc, dr, value))

scenarios.sort(key=lambda x: float(x[8]))
scenarios.reverse()
with open(outfile,"w") as fout:
  fout.write(lines[0] + "\n")
#  for scenario in scenarios[:int(len(scenarios)*topPercentile)]:
  for scenario in scenarios[:topNum]:
    fout.write("%s %s %d %d %d %d %d %d %s\n" % scenario)
