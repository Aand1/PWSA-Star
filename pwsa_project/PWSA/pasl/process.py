#!/usr/bin/python

import os
import os.path
import sys
import fileinput
import re

mypath = '.'

file_map = {}

for root, directories, filenames in os.walk(mypath):
  for filename in filenames:
    fname = os.path.join(root, filename)
    key = filename.rstrip('\n')
    key = re.sub(r'\W+', '', key)
    file_map[key] = fname

pathfiles = []
for root, directories, filenames in os.walk(mypath):
 for filename in filenames: 
   fname = os.path.join(root,filename)
   if (fname.endswith('.hpp') or fname.endswith('.cpp')):
     pathfiles += [fname]

linesDone = 0
for fname in pathfiles:
  rp = os.path.relpath('.', fname).count('/')
#  with open(fname, 'r') as f:
#    lines = [line.rstrip('\n') for line in f]
#    for line in lines:
  for line in fileinput.input(fname, inplace=True):
    if (line.startswith("#include")):
      l = line.split(' ')
      incfile = l[1]
      incfile = os.path.basename(incfile.replace("\"", ""))
      if (".hpp" in incfile):
        linesDone += 1
        rr = "../"*rp
        newinc = incfile
        incfile = re.sub(r'\W+', '', incfile)
        if (incfile in file_map):
          sys.stderr.write(incfile)
          suffix = file_map[incfile]
          suffix = suffix[2:]
          newinc = rr + suffix
        toprint = "#include \"%s\"\n" % (newinc.rstrip('\n'))
        print (toprint),
      else:
        print line,
    else:
      print line,

print(file_map)

print("linesDone = ", linesDone)
