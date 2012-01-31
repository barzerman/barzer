#!/usr/bin/python

import re
import fileinput

#reads in the file in CLASS|SUBCLASS|ID|name format 
#and produces statements 

tokenizer_re = re.compile(r'[\n;.\-,|()\[\]\'" ]')

def parse_line(line):
    fld=line.split('|')
    if len(fld) >= 4:
        name=tokenizer_re.split(fld[3])
        return ( fld[0], fld[1], fld[2], name )

stmtnum=10
for line in fileinput.input():
    b=tokenizer_re.split(line)
    (eclass,esubclass,eid,name)=parse_line(line)
    print '<stmt n="{0}"><pat>'.format(stmtnum),
    for n in name:
        if len(n) > 0:
            print '<t>{0}</t>'.format(n),
    print '</pat><tran><mkent c="{0}" s="{1}" i="{2}"/>'.format( eclass,esubclass,eid),
    print '</stmt>'
    stmtnum = stmtnum+10
    #for i in b:
        #print i

