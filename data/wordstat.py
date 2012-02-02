#!/usr/bin/python
# -*- coding: utf-8 -*-


import fileinput,codecs, sys, re

#reads in the file in CLASS|SUBCLASS|ID|name format 
#and produces statements 
writer = codecs.getwriter('utf-8')(sys.stdout)
reader = codecs.getreader('utf-8')(sys.stdin)
utf8_re=re.compile(u'[^а-яА-Я]+',re.UNICODE)

wordcnt={}
for line in reader:
    if len(line)<3:
        continue
    fld=utf8_re.split(line)
    for i in fld:
        if len(i)>1 and i[0] != ' ':
            if not i in wordcnt:
                wordcnt[i]=1
            else:
                wordcnt[i]+=1

writer = codecs.getwriter('utf-8')(sys.stdout)

#handling capitalized words - discovering proper names 
for i in wordcnt:
    if len(i)>1 and i[0] != i[0].lower() and i[1] == i[1].lower():
        tolc=i
        tolc=tolc[0].lower()+tolc[1:]
        cnt=wordcnt[i]
        if tolc not in wordcnt and cnt>1:
            writer.write(i)
            print ",",cnt
if False:
    for i in wordcnt:
        if wordcnt[i]>5:
            writer.write(i)
            print ",",wordcnt[i]
