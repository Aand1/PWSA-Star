#!/usr/bin/python

import sys
import tempfile
import subprocess
import itertools

def errorUnknown(name):
  raise Exception("Unrecognized name: " + name)
  return ""

pwsa = "./pwsa_main.opt"

args = sys.argv[1:]

baseIdx = args.index("-baseline")
baselineName = args[1 + baseIdx]
args = args[:baseIdx] + args[(baseIdx+2):]
baseline = "./pase_main.opt" if baselineName == "pase" else \
           "./astar_main.opt" if baselineName == "astar" else \
           errorUnknown(baselineName)

scenIdx = args.index("-scen")
scenfile = args[1 + scenIdx]
args = args[:scenIdx] + args[(scenIdx+2):]

outputIdx = args.index("-output")
outputfile = args[1 + outputIdx]
args = args[:outputIdx] + args[(outputIdx+2):]

runsIdx = args.index("-runs")
runs = args[1 + runsIdx]
args = args[:runsIdx] + args[(runsIdx+2):]

# speedupIdx = args.index("-speedup")
# speedupfile = args[1 + speedupIdx]
# args = args[:speedupIdx] + args[(speedupIdx+2):]

# resultIdx = args.index("-results")
# resultsfile = args[1 + resultIdx]
# args = args[:resultIdx] + args[(resultIdx+2):]

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

def readOutputOf(cmd):
  out = {}
  with tempfile.SpooledTemporaryFile(max_size=1024*10) as ftemp:
    print cmd
    subprocess.call(cmd, stdout=ftemp, stderr=ftemp, shell=True)
    ftemp.seek(0)
    for line in ftemp.readlines():
      try:
        [key,value] = line.split()
        out[key] = value
      except:
        print "Had trouble parsing this line: " + line
  return out

for scenario in fileGetLines(scenfile)[1:]:
  [scenNum, mapName, width, height, sc, sr, dc, dr, opt] = scenario.split()
  mapArgs = "-map %s -sr %s -sc %s -dr %s -dc %s" % (mapName, sr, sc, dr, dc)

  allParams = []
  for i in xrange(0,len(args),2):
    paramName = args[i][1:] # get rid of the "-"
    paramValues = args[i+1].split(',')
    allParams.append([(paramName, value) for value in paramValues])

  #resultsFile = "results" + mapName[4:] + "_vs_" + baselineName + ".txt"
  #with open(speedupfile, "a") as fspeedup:
  with open(outputfile, "a") as fresults:
    for argTuple in itertools.product(*allParams):
      theseArgs = ' '.join(map(lambda (x,y): "-" + x + " " + y, argTuple))
      pwsaCmd = ' '.join([pwsa, theseArgs, mapArgs])
      baseCmd = ' '.join([baseline, theseArgs, mapArgs])
      for i in xrange(0,int(runs)):
        try:
          pwsaOut = readOutputOf(pwsaCmd)
          baseOut = readOutputOf(baseCmd)

          speedup = float(baseOut["exectime"]) / float(pwsaOut["exectime"])
          deviation = float(pwsaOut["pathlen"]) / float(baseOut["pathlen"])
          expansions = float(pwsaOut["expanded"]) / float(baseOut["expanded"])

          # fresults.write("map " + mapName + "\n")
          for (k,v) in argTuple:
            fresults.write("%s %s\n" % (k, v))
          #  fresults.write(k + " " + v + "\n")
          fresults.write("---\n")
          fresults.write("speedup %s\n" % str(speedup))
          fresults.write("deviation %s\n" % str(deviation))
          fresults.write("expansions %s\n" % str(expansions))
          fresults.write("==========\n")

          # for (k,v) in argTuple:
          #   fspeedup.write(k + " " + v + "\n")
          # fspeedup.write("---\n")
          # fspeedup.write("prun_speedup baseline\n")
          # fspeedup.write("exectime %s\n" % baseOut["exectime"])
          # fspeedup.write("==========\n")
          # for (k,v) in argTuple:
          #   fspeedup.write(k + " " + v + "\n")
          # fspeedup.write("---\n")
          # fspeedup.write("prun_speedup parallel\n")
          # fspeedup.write("exectime %s\n" % pwsaOut["exectime"])
          # fspeedup.write("==========\n")
        except:
          print "Skipping..."
