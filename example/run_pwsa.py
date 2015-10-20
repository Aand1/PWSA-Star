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
procs = [1,2,3,4,6,8,12,16,20,22]

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

def runTest(map_name, srcX, srcY, dstX, dstY, D, K, p):
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
 
  if p not in runs[map_name]:
    runs[map_name][p] = []

  print(args)
  print(output)
  n = int(output[0].split('=')[1])
  expanded = int(output[1].split('=')[1])
  pathLength = int(output[2].split('=')[1])
  execTime = float(output[3].split()[1])
  util = float(output[4].split()[1])
  print(p, util)

  runs[map_name][p] += [(D, K, n, expanded, pathLength, execTime, util)]

def runTests(map_name, srcX, srcY, dstX, dstY):
  # Try a couple of variations of (D, K) with all proc specs
  D = 10000
  K = 10000
  # TODO: get rid of this hackiness
  if (random.random() > 0.03):
    return
  for p in procs:
    # running through ~10k scenarios. Can sample a bit to cut-down time.
    runTest(map_name, srcX, srcY, dstX, dstY, D, K, p)

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
    runTests(map_name, srcX, srcY, dstX, dstY)

def graph(formula, x_range):  
    x = np.array(x_range)  
    y = formula(x) 
    plt.plot(x, y)  

# speedup plots
for map_name in runs:
  mapName = map_name.split('/')[2]
  print(runs[map_name])
  procs = runs[map_name]
  p1Time = 0.0
  times = []
  for p in procs:
    tups = procs[p]
    tot = 0
    for tup in tups:
      D = tup[0]
      K = tup[1]
      n = tup[2]
      exp = tup[3]
      pl = tup[4]
      execTime = tup[5]
      util = tup[6]
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
  graph(lambda x: x, range(0, 22))
  plt.title('Speedup : ' + mapName)
  plt.ylabel('Speedup')
  plt.xlabel('num_proc')
  plt.show()
  save("speedup_" + mapName, ext="png", close=True, verbose=True)

print(runs)

#  tups = sorted(tups, key=lambda x: x[2])
#  p1Time = tups[0][6]
#  x = []
#  y = []
#  for tup in tups:
#    D = tup[0]
#    K = tup[1]
#    p = tup[2]
#    n = tup[3]
#    exp = tup[4]
#    pl = tup[5]
#    execTime = tup[6]
#    util = tup[7]
#    x += [p]
#    y += [p1Time / execTime]
#  plt.plot(x, y)
#  plt.show()
#  save("foo", ext="png", close=True, verbose=True)
