#!/usr/bin/python

import sys
import tempfile
import subprocess
import itertools
import os

# def errorUnknown(name):
#   raise Exception("Unrecognized name: " + name)
#   return ""

pwsa = "./pwsa_main.opt"

serial = ["./astar_main.opt"]

args = sys.argv[1:]

algoIdx = args.index("-algo")
algos = args[1 + algoIdx].split(",")
args = args[:algoIdx] + args[(algoIdx+2):]

# baseIdx = args.index("-baseline")
# baselineName = args[1 + baseIdx]
# args = args[:baseIdx] + args[(baseIdx+2):]
# baseline = "./pase_main.opt" if baselineName == "pase" else \
#            "./astar_main.opt" if baselineName == "astar" else \
#            errorUnknown(baselineName)

scenIdx = args.index("-scen")
scenfiles = args[1 + scenIdx].split(",")
args = args[:scenIdx] + args[(scenIdx+2):]

outputIdx = args.index("-output")
outputfile = args[1 + outputIdx]
args = args[:outputIdx] + args[(outputIdx+2):]

# runsIdx = args.index("-runs")
# runs = args[1 + runsIdx]
# args = args[:runsIdx] + args[(runsIdx+2):]

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
        raise Exception("Error when reading output of " + cmd)
  return out

memories = {}
def readMemoizedOutputOfSerial(cmd):
  x = cmd.split()
  noProcCmd = ' '.join(x[:x.index("-proc")] + x[(x.index("-proc") + 2):])
  if noProcCmd not in memories:
    out = readOutputOf(cmd)
    print "SAVING: %s" % cmd
    memories[noProcCmd] = out
  return memories[noProcCmd]

def runEachAlgo(theseArgs, fresults, maxRetry):
  try:
    pwsaCmd = ' '.join([pwsa, theseArgs])
    pwsaOut = readOutputOf(pwsaCmd)
    pwsaExectime = float(pwsaOut["exectime"])
    pwsaPathlen = float(pwsaOut["pathlen"])
    pwsaExpanded = float(pwsaOut["expanded"])
    for algo in algos:
      baseCmd = ' '.join([algo, theseArgs])
      baseOut = readOutputOf(baseCmd) if algo not in serial else readMemoizedOutputOfSerial(baseCmd)

      speedup = float(baseOut["exectime"]) / pwsaExectime
      deviation = pwsaPathlen / float(baseOut["pathlen"])
      expansions = pwsaExpanded / float(baseOut["expanded"])

      fresults.write("algo %s\n" % algo)
      fresults.write("scen %s\n" % scenfile)
      for (k,v) in argTuple:
        fresults.write("%s %s\n" % (k, v))
      fresults.write("---\n")
      fresults.write("speedup %s\n" % str(speedup))
      fresults.write("deviation %s\n" % str(deviation))
      fresults.write("expansions %s\n" % str(expansions))
      fresults.write("==========\n")
      fresults.flush()
      return
  except:
    if maxRetry == 0:
      print "Skipping..."
      return
    else:
      runEachAlgo(theseArgs, fresults, maxRetry - 1)


with open(outputfile, "a") as fresults:
  for scenfile in scenfiles:
    for scenario in fileGetLines(scenfile)[1:]:
      [scenNum, mapName, width, height, sc, sr, dc, dr, opt] = scenario.split()
      mapArgs = "-map %s -sr %s -sc %s -dr %s -dc %s" % (mapName, sr, sc, dr, dc)

      allParams = []
      for i in xrange(0,len(args),2):
        paramName = args[i][1:] # get rid of the "-"
        paramValues = args[i+1].split(',')
        allParams.append([(paramName, value) for value in paramValues])

      for argTuple in itertools.product(*allParams):
        theseArgs = ' '.join(map(lambda (x,y): "-" + x + " " + y, argTuple))
        runEachAlgo(' '.join([theseArgs, mapArgs]), fresults, 2)

    os.fsync(fresults.fileno()) # force write to file in case something bad happens

        # for algo in algos:
        #   baseCmd = ' '.join([algo, theseArgs, mapArgs])
        #
        #   try:
        #     pwsaOut = readOutputOf(pwsaCmd)
        #     baseOut = readOutputOf(baseCmd)
        #
        #     speedup = float(baseOut["exectime"]) / float(pwsaOut["exectime"])
        #     deviation = float(pwsaOut["pathlen"]) / float(baseOut["pathlen"])
        #     expansions = float(pwsaOut["expanded"]) / float(baseOut["expanded"])
        #
        #     # fresults.write("map " + mapName + "\n")
        #     fresults.write("algo %s\n" % algo)
        #     fresults.write("scen %s\n" % scenfile)
        #     for (k,v) in argTuple:
        #       fresults.write("%s %s\n" % (k, v))
        #     fresults.write("---\n")
        #     fresults.write("speedup %s\n" % str(speedup))
        #     fresults.write("deviation %s\n" % str(deviation))
        #     fresults.write("expansions %s\n" % str(expansions))
        #     fresults.write("==========\n")
        #   except:
        #     print "Skipping..."
