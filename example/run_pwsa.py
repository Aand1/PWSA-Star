#!/usr/bin/python

import sys
import os
import subprocess
import random 
import numpy as np
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

# call both the pwsa and pastar_int binaries. 
# ./pwsa_main.opt -D 10000 -K 10000 -graph maps/maze512-4-0.map -srcY 228 -srcX 417 -dstY 308 -dstX 28 -proc 4 -isGrid 1 -useEuc 1

# scen files look like:   17 2 maps/mazes/maze512-4-0.map  512 512 112 168 108 162 8.24264

runs = {}
pathQuality = {}
procs = [1,2,3,4,6,8,12,16,20,24,32]
random.seed(42)

D = 1000
K = 1000

def save(path, ext='png', close=True, verbose=True):
    directory = os.path.split(path)[0]
    filename = "%s.%s" % (os.path.split(path)[1], ext)
    if directory == '':
        directory = '.'
    if not os.path.exists(directory):
        os.makedirs(directory)
    savepath = os.path.join(directory, filename)
    if verbose:
        print("Saving figure to '%s'..." % savepath),
    plt.savefig(savepath)
    if close:
        plt.close()
    if verbose:
        print("Done")

def runTest(map_name, srcX, srcY, dstX, dstY, p):
  args = ("./pwsa_main.opt", "-D", str(D), "-K", str(K), 
          "-graph", map_name, "-srcX", str(srcX), 
          "-srcY", str(srcY), "-dstX", str(dstX),
          "-dstY", str(dstY), "-isGrid", str(1), 
          "-useEuc", str(1), '-proc', str(p))
  popen = subprocess.Popen(args, stdout=subprocess.PIPE)
  popen.wait()
  output = popen.stdout.read()
  output = output.split('\n')
  if map_name not in runs:
    runs[map_name] = {}
    pathQuality[map_name] = {}
 
  if p not in runs[map_name]:
    runs[map_name][p] = []

  queryStr = str(srcX) + "|" +  str(srcY) + "|" + str(dstX) + "|" + str(dstY)
  if queryStr not in pathQuality[map_name]:
    pathQuality[map_name][queryStr] = []
 
  print(output) 
  n = int(output[0].split('=')[1])
  expanded = int(output[1].split('=')[1])
  pathLength = int(output[2].split('=')[1])
  execTime = float(output[3].split()[1])
  util = float(output[4].split()[1])

  runs[map_name][p] += [(n, expanded, pathLength, execTime, util, queryStr)]

def runTests(map_name, srcX, srcY, dstX, dstY, dist):
  # Try a couple of variations of (D, K) with all proc specs
  # TODO: get rid of this hackiness
  if (random.random() > 0.1 or dist < 600):
    return
  for p in procs:
    # running through ~10k scenarios. Can sample a bit to cut-down time.
    runTest(map_name, srcX, srcY, dstX, dstY, p)

scen_name = sys.argv[1]
with open(scen_name) as scen:
  lines = [line.rstrip('\n') for line in scen]
  lines = lines[1:]
  for line in lines:
    print(line)
    l = line.split()
    map_name = l[1]
    srcX = int(l[4])
    srcY = int(l[5])
    dstX = int(l[6])
    dstY = int(l[7])
    dist = float(l[8])
    runTests(map_name, srcX, srcY, dstX, dstY, dist)

def graph(formula, x_range):  
    x = np.array(x_range)  
    y = formula(x) 
    plt.plot(x, y)  

# speedup plots
for map_name in runs:
  mapName = map_name.split('/')[2]
  print(runs[map_name])
  p1Time = 0.0
  times = []
  for p in procs:
    tups = runs[map_name][p]
    tot = 0
    for tup in tups:
      n = tup[0]
      exp = tup[1]
      pl = tup[2]
      execTime = tup[3]
      util = tup[4]
      tot += execTime
    tot /= len(tups)
    print("tot = ", len(tups))
    times += [(p, tot)]
    if (p == 1):
      p1Time = tot
  x = []
  y = []
  print(times)
  for pair in times:
    x += [pair[0]]
    y += [p1Time / pair[1]]
  plt.plot(x, y)
  graph(lambda x: x, range(0, 32))
  plt.title('Speedup : ' + mapName)
  plt.ylabel('Speedup')
  plt.xlabel('num_proc')
  plt.show()
  save("images/speedup_" + mapName, ext="png", close=True, verbose=True)

# path-quality plots
for map_name in runs:
  mapName = map_name.split('/')[2]
  print(runs[map_name])

  x = []
  y = []

  maxDiv = 0.0
  trueLength = 0.0
  ourLength = 0.0
  maxQuery = ""

  queryMap = {} 
  for p in procs:
    tups = runs[map_name][p]
    procDivergence = []
    for tup in tups:
      pathLength = tup[2]
      queryStr = tup[5]
      if queryStr not in queryMap:
        # p == 1
        queryMap[queryStr] = pathLength
      else:
        divergence = (pathLength * 1.0) / queryMap[queryStr] # should always be >= 
        if (pathLength < queryMap[queryStr]):
          print("you got serious problems. Truelen = ", queryMap[queryStr], "queryStr = ", queryStr, " you got ", pathLength)
          sys.exit(-1)
        if (divergence > maxDiv):
          maxDiv = divergence
          trueLength = queryMap[queryStr]
          ourLength = pathLength
          maxQuery = queryStr
        procDivergence += [divergence]
    if (p > 1):
      avgDivergence = (reduce(lambda x, y: x+y, procDivergence) * 1.0) / len(procDivergence)
      x += [p]
      y += [avgDivergence]
  print(x)
  print(y)

  print("Max divergence on : " + maxQuery + " true=" + str(trueLength) + " ourLength = " + str(ourLength))

  plt.plot(x, y)
  plt.title('Average divergence : ' + mapName + " k = " + str(K) + " d = " + str(D))
  plt.ylabel('length')
  plt.xlabel('num_proc')
  plt.show()
  save("images/divergence_" + str(K) + "_" + str(D) + "_" +  mapName, ext="png", close=True, verbose=True)
