#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, re



try:
    LDD, PAT = {
        'Linux': ('ldd', '=>\s+(.*)\s+\(0x[\da-f]+\)$'),
        'Darwin': ('otool -L', '^([^\(]+)\s+\(')
    }[os.uname()[0]]
except KeyError:
    sys.stderr.write("Unknown OS\n")
    sys.exit(1)

RE = re.compile(PAT)

g_visited=set()
def recurse(arr):
    for i in arr:
        if len(i)>0 and i not in g_visited:
            g_visited.add(i)
            descendArr=[]
            for line in os.popen("{0} {1}".format(LDD, i)):
                m = RE.search(line.strip())
                if m:
                    fname = m.group(1).strip()
                    if fname and os.path.isfile(fname) and fname not in g_visited:
                        descendArr.append(fname)
            recurse(descendArr)

if len(sys.argv)<=1:
    sys.stderr.write( "usage: getdeplibs.py <file>\n")
    sys.exit(2)
firstbin= sys.argv[1] if len(sys.argv)>1 else "./barzer.exe"
arr=[firstbin]
recurse(arr)

for i in g_visited:
    if i != firstbin:
        print i
