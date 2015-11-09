#!/usr/bin/python

import sys
import itertools

# ===========================================================================
# ===========================================================================

# STEPS:
#  1. Filter out entries which we don't care about for this comparison
#     (for example, if we're comparing wPWSA* against wPA*SE on bgmaps,
#     then get rid of every entry which ran wA* ...)
#
#     ** Runs kept defined by function `keep` below
#
#  2. Map results of remaining runs to a non string format
#
#     ** Performed by function `simplifyResults` below
#
#  3. Combine identical runs with average (perhaps we ran every trial
#     10 times...)
#
#  4. Compute pairwise comparison
#
#     ** Performed by function `combineResults` below
#
#  5. Simplify params for each comparison to ignore some info (for example,
#     forget specific path info)
#
#     ** Performed by function `simplifyParams` below
#
#  6. Combine identical simplified runs with average

def keep(algo, params):
  return params["prog"] == "./pwsa_main.opt" or params["prog"] == algo

def simplifyResults(results):
  floatyKeys = ["exectime", "pathlen", "expanded"]
  return { k : float(results[k]) for k,v in results.iteritems() if k in floatyKeys }

def combineResults(rs1, rs2):
  return { "speedup" : rs2["exectime"] / rs1["exectime"] # this is backwards from others but is correct...
         , "deviation" : rs1["pathlen"] / rs2["pathlen"]
         , "expansion" : rs1["expanded"] / rs2["expanded"]
         }

def simplifyParams(params):
  return { "map" : params["map"]
         , "proc" : int(params.get("proc", "1"))
         , "w" : float(params.get("w", "1.0"))
         , "exptime" : float(params.get("exptime", "0.0"))
         }

# here's a crappy lexicographic ordering over assumed parameters
def cmpForSortedOutput ((ps1,rs1),(ps2,rs2)):
#  print "comparing " + str(ps1) + " , " + str(ps2)
  for key in ["proc", "w", "exptime", "map"]:
#    print "comparing %s" % key
    if ps1[key] < ps2[key]:
#      print "less than"
      return -1
    if ps1[key] > ps2[key]:
#      print "greater than"
      return 1
#  print "equal"
  return 0

# ===========================================================================
# ===========================================================================

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

def mergeDictsWith(combine):
  def doMerge(d1, d2):
    result = { k : v for k,v in d1.iteritems() } # I guess this is just a copy
    for k,v in d2.iteritems():
      if k in result:
        result[k] = combine(result[k], v)
      else:
        result[k] = v
    return result
  return doMerge

def intersectDictsWith(combine):
  def doIntersect(d1, d2):
    result = { k : v for k,v in d1.iteritems() if k in d2 }
    for k,v in d2.iteritems():
      if k in result:
        result[k] = combine(result[k], v)
    return result
  return doIntersect

def collectWith(conflictHandler):
  def doCollect(kvpairs):
    result = {}
    for k,v in kvpairs:
      if k in result:
        result[k].append(v)
      else:
        result[k] = [v]
    return { k : conflictHandler(vs) for k,vs in result.iteritems() }
  return doCollect

def keywiseAverages(vs):
  withCounts = [ { k : (v,1) for k,v in d.iteritems() } for d in vs ]
  summed = reduce(mergeDictsWith(lambda (x,cx),(y,cy): (x+y, cx+cy)), withCounts, {})
  return { k : v / c for k,(v,c) in summed.iteritems() }

# ===========================================================================
# ===========================================================================

def step1(algo, runslist):
  return [ (ps, rs) for ps, rs in runslist if keep(algo, ps) ]

def step2(runslist):
  return [ (ps, simplifyResults(rs)) for ps, rs in runslist ]

def step3(runslist):
  frozenkeys = [ (frozenset(k.items()), v) for k,v in runslist ]
  return collectWith(keywiseAverages)(frozenkeys)

def step4(algo, runsdict):
  def noprog(frozen):
    return frozenset([ (k,v) for k,v in frozen if k != "prog" ])
  pwsaRuns = { noprog(ps) : rs for ps,rs in runsdict.iteritems() if ("prog", "./pwsa_main.opt") in ps }
  algoRuns = { noprog(ps) : rs for ps,rs in runsdict.iteritems() if ("prog", algo) in ps }
#  print "PWSA*: " + str(pwsaRuns)
#  print "ALGO : " + str(algoRuns)
  return intersectDictsWith(combineResults)(pwsaRuns, algoRuns)

def step5(runsdict):
  return [ (simplifyParams(dict(ps)), rs) for ps, rs in runsdict.iteritems() ]

def step6(runslist):
  frozenkeys = [ (frozenset(k.items()), v) for k,v in runslist ]
  stillFrozen = collectWith(keywiseAverages)(frozenkeys)
  listed = [ (dict(ps), rs) for ps, rs in stillFrozen.iteritems() ]
  return sorted(listed, cmp=cmpForSortedOutput)

# def avg
# mergeWithAvg = mergeDictsWith(lambda vs :

args = sys.argv[1:]

# mode = "
# i = args.index("-mode") if "-mode" in args else None
# mode = args[i+1]
# args = args[:i] + args[(i+2):]

i = args.index("-algo")
algo = args[i+1]
args = args[:i] + args[(i+2):]

i = args.index("-input")
inputfile = args[i+1]
args = args[:i] + args[(i+2):]

i = args.index("-output")
outputfile = args[i+1]
args = args[:i] + args[(i+2):]

def splitAt(iterable, isDelim):
  return [list(g) for k, g in itertools.groupby(iterable, isDelim) if not k]
def isOuterDelim(x):
  return x == "" or x[0] == "="
def isInnerDelim(x):
  return x == "" or x[0] == "-"
def dictifyOneRun(lines):
  def dictify(kvlines):
    out = {}
    for line in kvlines:
      [key, value] = line.split()
      out[key] = value
    return out
  i = next(i for i,x in enumerate(lines) if isInnerDelim(x))
  return (dictify(lines[:i]), dictify(lines[(i+2):]))
runs = [ dictifyOneRun(group) for group in splitAt(fileGetLines(inputfile), isOuterDelim) ]

final = step6(step5(step4(algo,step3(step2(step1(algo,runs))))))

with open(outputfile, 'w') as fout:
#  print final
  for ps,rs in final:
    for k,v in ps.iteritems():
      fout.write("%s %s\n" % (k, str(v)))
    fout.write("---\n")
    for k,v in rs.iteritems():
      fout.write("%s %.4f\n" % (k, round(v, 4)))
    fout.write("==========\n")
