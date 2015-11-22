#!/usr/bin/python

import sys
import os
import tempfile
import subprocess

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

def shrink(factor, infile, outfile):
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

    fout.write("height %d\n" % (height / factor))
    fout.write("width %d\n" % (width / factor))
    fout.write("map\n")

    for row in xrange(0, height / factor):
      for col in xrange(0, width / factor):
        isObstacle = True
        for littlerow in xrange(0, factor):
          for littlecol in xrange(0, factor):
            c = grid[row*factor + littlerow][col*factor + littlecol]
            if c == '.' or c == 'G':
              isObstacle = False
        fout.write('@' if isObstacle else '.');
      fout.write('\n')


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
mode = sys.argv[1] # "blowup" or "shrink"
factor = int(sys.argv[2])
topNum = int(sys.argv[3])
infile = sys.argv[4]
outfile = sys.argv[5]

lines = fileGetLines(infile)
scenarios = []

ext = (".big-" if mode == "blowup" else ".min-") + str(factor)

for line in lines[1:]:
  [scenNum, mapName, width, height, sc, sr, dc, dr, x] = line.split()
  [width,height,sc,sr,dc,dr] = map(lambda s: int(s) * factor if mode == "blowup" else int(s) / factor, [width,height,sc,sr,dc,dr])

  if factor > 1 and mapName[-6:] != ext:
    if not os.path.exists(mapName + ext):
      print "Generating %s from %s" % (mapName + ext, mapName)
      if mode == "blowup":
        blowup(factor, mapName, mapName + ext)
      else:
        shrink(factor, mapName, mapName + ext)
    mapName = mapName + ext

  cmd = "./astar_main.opt -map %s -sc %d -sr %d -dc %d -dr %d -proc 1 -exptime 0.0" % (mapName, sc, sr, dc, dr)
  with tempfile.TemporaryFile() as ftemp:
    print cmd
    subprocess.call(cmd, stdout=ftemp, stderr=ftemp, shell=True)
    ftemp.seek(0)
    for line in ftemp.readlines():
      [key, value] = line.split()
      if key == "pathlen":
        scenarios.append((scenNum, mapName, width, height, sc, sr, dc, dr, value))
        break

scenarios.sort(key=lambda x: float(x[8]))
scenarios.reverse()
with open(outfile,"w") as fout:
  fout.write(lines[0] + "\n")
  for scenario in scenarios[:topNum]:
    fout.write("%s %s %d %d %d %d %d %d %s\n" % scenario)
