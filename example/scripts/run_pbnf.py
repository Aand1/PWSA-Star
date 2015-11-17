#!/usr/bin/python

import os
import sys
import subprocess
import tempfile

def parseArgs(argdefaults):
  result = {}
  for name, default in argdefaults.iteritems():
    try:
      i = sys.argv.index(name)
      if name[:2] == "--":
        # flag argument
        result[name] = True
      else:
        result[name] = sys.argv[i+1]
    except ValueError:
      result[name] = default
  return result

def convert(fname, outputName, sr, sc, dr, dc):
  with open(fname, "r") as f:
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

    with open(outputName, 'w') as out:
      out.write("%d %d\n" % (height, width))
      out.write("Board:\n")

      for i in xrange(len(grid)):
        row = grid[i]
        r = ""
        for elm in row:
          if (elm == 1):
            r += " "
          else:
            r += "#"
        out.write(r + "\n")

      out.write("Unit\nEight-way\n")
      out.write("%s %d   %s %d\n" % (sc, height - int(sr) - 1, dc, height - int(dr) - 1))

argdefaults = { "-map" : "maps/simple_map.map"
              , "-sr"  : "1"
              , "-sc"  : "1"
              , "-dr"  : "1"
              , "-dc"  : "1"
              , "-w"   : "1.0"
              , "-proc" : "1"
              , "-opt" : "1.0"
              , "-minexpand" : "32"
              , "-nblocks" : "1024"
              }
args = parseArgs(argdefaults)

pbnfgrid = args["-map"].replace(".map","")
pbnfgrid += '.pbnf_grid'
convert(args["-map"], pbnfgrid, args["-sr"], args["-sc"], args["-dr"], args["-dc"])

with tempfile.SpooledTemporaryFile(max_size=10*1024) as log:
  #wpbnf-<weight>-<min_expansions>-<threads>-<nblocks>
  executable = "../pbnf/src/grid_search.x86_64"
  alg = "safepbnf" if float(args["-w"]) == 1.0 else "wpbnf"
  arg = "%s-%s-%s-%s-%s" % (alg, args["-w"], args["-minexpand"], args["-proc"], args["-nblocks"])
  cmd = [executable, arg]
  #print ' '.join(cmd)
  with open(pbnfgrid, "r") as grid:
    retcode = subprocess.call(cmd, stdout=log, stdin=grid)

  log.seek(0)
  lines = log.readlines()
  for line in lines:
    #cols: "sol cost" "sol length" "nodes expanded" "nodes generated" "raw wall time"
    cols = line.split(" ")
    if cols[0] == "row:":
      pathlen = float(cols[1])
      expanded = int(cols[3])
      exectime = float(cols[5])
      deviation = pathlen / float(args["-opt"])
      print "pathlen %f" % pathlen
      print "expanded %d" % expanded
      print "exectime %f" % exectime
      print "deviation %f" % deviation
      break
    if line == "# No Solution\n":
      retcode = 1 # failure...

if retcode != 0:
  print ''.join(lines)
sys.exit(retcode)
