#!/usr/bin/python

import sys
import os
import math

fname = sys.argv[1]
outputName = fname.replace('.map', '')
outputName += '.adj'

root2 = 14142
one = 10000

def makeValidPos(row, col):
  validPos = []
  if (row != 0):
    if (col != 0):
      validPos += [(row-1, col-1, root2)]
    validPos += [(row-1, col, one)]
    if (col != (width - 1)):
      validPos += [(row-1, col+1, root2)]
  if (col != 0):
    validPos += [(row, col-1, one)]
  if (col != (width - 1)):
    validPos += [(row, col+1, one)]
  if (row != (height - 1)):
    if (col != 0):
      validPos += [(row+1, col-1, root2)]
    validPos += [(row+1, col, one)]
    if (col != (width - 1)):
      validPos += [(row+1, col+1, root2)]
  return validPos

with open(fname) as f:
  lines = [line.rstrip('\n') for line in f]
  if ('height' not in lines[1] or 'width' not in lines[2]):
    print("Bad input file")
    sys.exit(-1)
  height = int(lines[1].split(' ')[1])
  width = int(lines[2].split(' ')[1])
  grid = []
  coords = {}
  idToCoord = []
  n = 0
  for i in xrange(4, len(lines)):
    row = i - 4
    grid += [[0] * width]
    line = lines[i]
    for col in xrange(len(line)):
      if (line[col] == '.' or line[col] == 'G'):
        grid[row][col] = 1
        coords[(row,col)] = n
        idToCoord += [(row, col)]
        n += 1

  print("n = ", n)

  graph = {}
  offsets = [0] * (n + 1)
  m = 0
  for row in xrange(height):
    for col in xrange(width):
      if (grid[row][col] == 1):
        vtx = coords[(row, col)]
        graph[vtx] = []
        validPos = makeValidPos(row, col)
        deg = 0
        for pos in validPos:
          if grid[pos[0]][pos[1]] == 1:
            ngh = coords[(pos[0], pos[1])]
            graph[vtx] += [(ngh, pos[2])]
            m += 1
            deg += 1
        offsets[vtx+1] = deg
        if (vtx != 0):
          offsets[vtx] += offsets[vtx-1]
  print("m = ", m)
  offsets[0] = 0

  with open(outputName, 'w') as out:
    out.write('WeightedAdjacencyGraph\n')
    out.write(str(n) + '\n')
    out.write(str(m) + '\n')
    # output vertex offset
    for i in xrange(0, len(offsets) - 1):
      offset = offsets[i]
      out.write(str(offset) + '\n')

    # output edge endpoints
    for i in xrange(n):
      vtx = graph[i]
      for ngh in vtx:
        out.write(str(ngh[0]) + "\n")

    # output edge weights
    for i in xrange(n):
      vtx = graph[i]
      for ngh in vtx:
        out.write(str(ngh[1]) + "\n")

    for i in xrange(n):
      coord = idToCoord[i]
      out.write(str(coord[0]) + " " + str(coord[1]) + "\n")


  out.close()
  

