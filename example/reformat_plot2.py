#!/usr/bin/python

import sys
import itertools

# ===========================================================================
# ===========================================================================

def keep(algo1, algo2, params):
  return params["algo"] == algo1 or params["algo"] == algo2

def simplifyResults(results):
  floatyKeys = ["exectime", "pathlen", "expanded", "deviation"]
  return { k : float(results[k]) for k,v in results.iteritems() if k in floatyKeys }

# def combineResults(rs1, rs2):
#   return { "speedup" : rs2["exectime"] / rs1["exectime"]
#          , "pathlen-improvement" : rs2["pathlen"] / rs1["pathlen"]
#          , "expansion-improvement" : rs2["expanded"] / rs1["expanded"]
#          }

def simplifyParams(params):
  return { "algo" : params["algo"]
         , "map" : params["map"]
         , "proc" : int(params.get("proc", "1"))
         , "w" : float(params.get("w", "1.0"))
         , "exptime" : float(params.get("exptime", "0.0"))
         }

# here's a crappy lexicographic ordering over assumed parameters
def cmpForSortedOutput ((ps1,rs1),(ps2,rs2)):
  for key in ["proc", "w", "exptime", "map"]:
    if ps1[key] < ps2[key]:
      return -1
    if ps1[key] > ps2[key]:
      return 1
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

def step1(algo1, algo2, runslist):
  return [ (ps, rs) for ps, rs in runslist if keep(algo1, algo2, ps) ]

def step2(runslist):
  return [ (ps, simplifyResults(rs)) for ps, rs in runslist ]

def step3(runslist):
  frozenkeys = [ (frozenset(k.items()), v) for k,v in runslist ]
  return collectWith(keywiseAverages)(frozenkeys)

# def step4(algo1, algo2, runsdict):
#   def noalgo(frozen):
#     return frozenset([ (k,v) for k,v in frozen if k != "algo" ])
#   algoRuns1 = { noalgo(ps) : rs for ps,rs in runsdict.iteritems() if ("algo", algo1) in ps }
#   algoRuns2 = { noalgo(ps) : rs for ps,rs in runsdict.iteritems() if ("algo", algo2) in ps }
#   return intersectDictsWith(combineResults)(algoRuns1, algoRuns2)

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

i = args.index("-algo1")
algo1 = args[i+1]
args = args[:i] + args[(i+2):]

i = args.index("-algo2")
algo2 = args[i+1]
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

final = step6(step5(step3(step2(step1(algo1,algo2,runs)))))

with open(outputfile, 'w') as fout:
#  print final
  for ps,rs in final:
    for k,v in ps.iteritems():
      fout.write("%s %s\n" % (k, str(v)))
    fout.write("---\n")
    for k,v in rs.iteritems():
      fout.write("%s %.4f\n" % (k, round(v, 4)))
    fout.write("==========\n")
