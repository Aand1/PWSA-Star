#!/usr/bin/python

import itertools
import sys
import ast

# def simplifyResults(results):
#   floatyKeys = ["exectime", "pathlen", "expanded", "deviation"]
#   return { k : float(results[k]) for k,v in results.iteritems() if k in floatyKeys }
#
# def combineResults(rs1, rs2):
#   return { "speedup" : rs2["exectime"] / rs1["exectime"]
#          , "pathlen-improvement" : rs2["pathlen"] / rs1["pathlen"]
#          , "expansion-improvement" : rs2["expanded"] / rs1["expanded"]
#          }
#
# def simplifyParams(params):
#   return { "algo" : params["algo"]
#          , "map" : params["map"]
#          , "proc" : int(params.get("proc", "1"))
#          , "w" : float(params.get("w", "1.0"))
#          , "exptime" : float(params.get("exptime", "0.0"))
#          }
#
# # here's a crappy lexicographic ordering over assumed parameters
# def cmpForSortedOutput ((ps1,rs1),(ps2,rs2)):
#   for key in ["map", "algo", "proc", "w", "exptime"]:
#     if ps1[key] < ps2[key]:
#       return -1
#     if ps1[key] > ps2[key]:
#       return 1
#   return 0

# ============================================================================
# ============================================================================

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

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

def readResultsFile(filename):
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
    return (dictify(lines[:i]), dictify(lines[(i+1):]))

  return [ dictifyOneRun(group) for group in splitAt(fileGetLines(filename), isOuterDelim) ]

# def mergeDictsWith(combine):
#   def doMerge(d1, d2):
#     result = { k : v for k,v in d1.iteritems() } # I guess this is just a copy
#     for k,v in d2.iteritems():
#       if k in result:
#         result[k] = combine(result[k], v)
#       else:
#         result[k] = v
#     return result
#   return doMerge
#
# def intersectDictsWith(combine):
#   def doIntersect(d1, d2):
#     result = { k : v for k,v in d1.iteritems() if k in d2 }
#     for k,v in d2.iteritems():
#       if k in result:
#         result[k] = combine(result[k], v)
#     return result
#   return doIntersect
#
# def collectWith(conflictHandler):
#   def doCollect(kvpairs):
#     result = {}
#     for k,v in kvpairs:
#       if k in result:
#         result[k].append(v)
#       else:
#         result[k] = [v]
#     return { k : conflictHandler(vs) for k,vs in result.iteritems() }
#   return doCollect
#
# def keywiseAverages(vs):
#   withCounts = [ { k : (v,1) for k,v in d.iteritems() } for d in vs ]
#   summed = reduce(mergeDictsWith(lambda (x,cx),(y,cy): (x+y, cx+cy)), withCounts, {})
#   return { k : v / c for k,(v,c) in summed.iteritems() }
#
# def keep(keepdict, params):
#   for k,v in params.iteritems():
#     if k in keepdict and v not in keepdict[k]:
#       return False
#   return True
#
# def refreezeAfter(f):
#   return (lambda d: frozenset(f(dict(d)).items()))
#
# def combineAlgoParam(algo1, algo2, frozen):
#   return frozenset([ ((k,v) if k != "algo" else (k, algo1 + "_vs_" + algo2)) for k,v in frozen ])

# ============================================================================
# ============================================================================

if __name__ == "__main__":
  argdefaults = { "-algo" : "wPWSA*"
                , "-scen" : "myscen/example.scen"
                , "-input" : "results.txt"
                , "-output" : "results_reformatted.txt"
                }
  args = parseArgs(argdefaults)
  inputfiles = args["-input"].split(",")
  outputfile = args["-output"]

  keepPathsList = []
  for scenfile in args["-scen"].split(","):
    for scenario in fileGetLines(scenfile)[1:]:
      [_, mapfile, w, h, sc, sr, dc, dr, opt] = scenario.split()
      keepPathsList.append((mapfile, sc, sr, dc, dr))
  keepPaths = set(keepPathsList)

  algos = args["-algo"].split(",")
  def keepRun(ps, rs):
    return ((ps['map'], ps['sc'], ps['sr'], ps['dc'], ps['dr']) in keepPaths) and (ps['algo'] in algos)

  filtered = [ (ps, rs) for inputfile in inputfiles for (ps, rs) in readResultsFile(inputfile) if keepRun(ps, rs) ]

  with open(outputfile, 'w') as fout:
    for ps,rs in filtered:
      for k,v in ps.iteritems():
        fout.write("%s %s\n" % (k, v))
      fout.write("---\n")
      for k,v in rs.iteritems():
        fout.write("%s %s\n" % (k, v))
      fout.write("==========\n")
