#!/usr/bin/python

import os
import sys
import subprocess
import tempfile

i = sys.argv.index("-algo")
algo = sys.argv[i+1]
newargs = sys.argv[1:i] + sys.argv[(i+2):]

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

def convert(fname, outputName):
  if os.path.isfile(outputName):
    with open(outputName, 'r') as f:
      height = f.readline().split(' ')[0]
      return int(height)
  else:
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

        return height

argdefaults = { "-map" : "maps/simple_map.map"
              , "-sr"  : "1"
              , "-sc"  : "1"
              , "-dr"  : "1"
              , "-dc"  : "1"
              , "-w"   : "1.0"
              , "-proc" : "1"
              , "-opt" : "1.0"
              , "-minexpand" : "64"
              , "-nblocks" : "65536"
              }
args = parseArgs(argdefaults)

pbnfstub = args["-map"].replace(".map",".pbnf_stub")
# convert(args["-map"], pbnfstub, args["-sr"], args["-sc"], args["-dr"], args["-dc"])
height = convert(args["-map"], pbnfstub)

with tempfile.SpooledTemporaryFile(max_size=10*1024) as log:
  #wpbnf-<weight>-<min_expansions>-<threads>-<nblocks>
  executable = "../../pbnf/src/grid_search.x86_64"
  if not os.path.isfile(executable):
    sys.stderr.write("ERROR: unable to find executable %s\n" % executable)
    sys.exit(1) # failure
  algarg = "astar"
  if algo == "safepbnf":
    algarg = "safepbnf-%s-%s-%s-%s" % (args["-w"], args["-minexpand"], args["-proc"], args["-nblocks"])
  #alg = "safepbnf" if float(args["-w"]) == 1.0 else "wpbnf"
  #algarg = "%s-%s-%s-%s-%s" % (alg, args["-w"], args["-minexpand"], args["-proc"], args["-nblocks"])
  #algarg = "astar"
  #algarg = "%s-%s" % ("wastar", args["-w"])
  formatarg = r'"%s\nUnit\nEight-way\n%d %d\t%d %d"'
  stubarg = '"$(cat %s)"' % pbnfstub
  coordsarg = "%s %d %s %d" % (args["-sc"], height - int(args["-sr"]) - 1, args["-dc"], height - int(args["-dr"]) - 1)
  cmd = ' '.join(["printf", formatarg, stubarg, coordsarg, "|", executable, algarg])

  #print cmd
  retcode = subprocess.call(cmd, stdout=log, shell=True)

  log.seek(0)
  lines = log.readlines()
  # for line in lines:
  #   #cols: "sol cost" "sol length" "nodes expanded" "nodes generated" "raw wall time"
  #   cols = line.split(" ")
  #   if cols[0] == "row:":
  #     pathlen = float(cols[1])
  #     expanded = int(cols[3])
  #     exectime = float(cols[5])
  #     deviation = pathlen / float(args["-opt"])
  #     print "pathlen %f" % pathlen
  #     print "expanded %d" % expanded
  #     print "exectime %f" % exectime
  #     print "deviation %f" % deviation
  #     break
  #   if line == "# No Solution\n":
  #     retcode = 1 # failure...
  for line in lines:
    line = line.rstrip('\n')
    if line == "# No Solution":
      retcode = 1 # failure
      break
    cols = line.split(" ")
    if cols[0] == "cost:":
      print "pathlen %s" % cols[1]
      print "deviation %f" % (float(cols[1]) / float(args["-opt"]))
    if cols[0] == "wall_time:":
      print "exectime %s" % cols[1]
    if cols[0] == "expanded:":
      print "expanded %s" % cols[1]

if retcode != 0:
  print ''.join(lines)
sys.exit(retcode)
