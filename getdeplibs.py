#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os
uname=os.popen('uname').read().strip()
LDD='ldd ' if uname=='Linux' else 'otool -L '
g_visited=set()
def recurse(arr):
    for i in arr:
        if len(i)>0 and i not in g_visited:
            g_visited.add(i)
            cmd=LDD+i
            tmp=os.popen( cmd ).read().strip().split('\n')
            descendArr=[]
            for line in tmp:
                if uname=='Linux':
                    l=line.split('=>')
                    if len(l) == 2: 
                        l0=l[0].strip()
                        if os.path.isfile(l0):
                            descendArr.append(l0)
                        f=l[1].strip().split(' ')[0]
                        if os.path.isfile(f) and f not in g_visited:
                            descendArr.append(f)
                else:
                    l=line.split(' ')
                    if len(l) >0:
                        l0=l[0].strip()
                        if os.path.isfile(l0) and l0 not in g_visited:
                            descendArr.append(l0)
            recurse(descendArr)

if len(sys.argv)<=1:
    sys.stderr.write( "usage: getdeplibs.py <file>\n")
    quit()
firstbin= sys.argv[1] if len(sys.argv)>1 else "./barzer.exe"
arr=[firstbin]
recurse(arr)

for i in g_visited:
    if i != firstbin:
        print i
