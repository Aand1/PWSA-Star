#!/usr/bin/python

import sys

factor = int(sys.argv[1])
infile = sys.argv[2]
outfile = sys.argv[3]

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

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
