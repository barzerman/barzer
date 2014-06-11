#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, re
from os.path import isfile

uname = os.uname()[0]

try:
    LDD, PAT = {
        'Linux': ('ldd', '=>\s+(.*)\s+\(0x[\da-f]+\)$'),
        'Darwin': ('otool -L', '^([^\(]+)\s+\(')
    }[uname]
except KeyError:
    sys.stderr.write("Unknown platform\n")
    sys.exit(1)

RE = re.compile(PAT)

STOPLIST = {
    '*': {},
    'Linux': {'libpython'},
    'Darwin': {'libSystem', 'Python'}
}

def in_stoplist(w):
    n = os.path.basename(w).split('.', 1)[0]

    return (n in STOPLIST['*'] or n in STOPLIST[uname])



g_visited=set()
def recurse(arr):
    for i in arr:
        if len(i) > 0 and not (in_stoplist(i) or i in g_visited):
            g_visited.add(i)
            descendArr=[]
            for line in os.popen("{0} {1}".format(LDD, i)):
                m = RE.search(line.strip())
                if m:
                    fname = m.group(1).strip()
                    if fname and isfile(fname) and fname not in g_visited:
                        descendArr.append(fname)
            recurse(descendArr)


if len(sys.argv) <= 1:
    sys.stderr.write( "usage: getdeplibs.py <file>\n")
    sys.exit(2)
firstbin= sys.argv[1] if len(sys.argv)>1 else "./barzer.exe"
arr = [firstbin]
recurse(arr)

for i in g_visited:
    if i != firstbin:
        print i
