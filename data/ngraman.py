#!/opt/local/bin/python
# -*- coding: utf-8 -*-


import fileinput,codecs, sys, re
from collections import defaultdict

tokenizer_re = re.compile(r'[\n;.\-,|()\[\]\'" ]')
nonpure_unigram_re = re.compile(r'[^ a-z]')

out_re = re.compile('\d')

engwords=set()
synonyms=dict()

def normalize(xs):
    def filt(s):
        return not (s in {'in', '', 'of', 'on', 'for', 'and', 'or', '&'} or out_re.search(s))
    return filter(filt, xs)

class boo(object):
    def __init__(self):
        self.ids = set()
        self.cnt = 0
    def push(self, i):
        #self.ids.add(str(i))
        self.cnt += 1

class EntInfo(object):
    def __init__(self):
        self.freq = 0
        self.names = []
    def __init__(self,f):
        self.freq = f
        self.names = []
    def push(self, i):
        self.names.append(i)
    
entities=defaultdict(dict)

with open( "spell/english_words.txt" ) as f:
    for i in f:
        engwords.add(i.strip())
print>>sys.stderr, len(engwords),"words"

if __name__ == '__main__':
    s = defaultdict(boo)
    for line in fileinput.input():
        idd, _, n = line.partition(',')
        b = tokenizer_re.split(n)
        
        l = len(b)        
        for i in xrange(l):
            if  b[i] in {'b','di','lb','+', 'e', 'c','any','with','at','en','a','if','/','an','in', '', 'of', 'on', 'de','s','y','for', 'and', 'or', '&'}:
                continue
            xs = []
            for j in xrange(i, min(i+4, l)):
                xs.append(b[j])
                if len(b[j]) > 1 and b[j] not in {'di','s','e','lb','t','c','foo','al','la','lo','de','any','with','the','to','at','a','if','an','in', '', 'of', 'on', 'for', 'and', 'or', '&'}:
                    s[tuple(xs)].push(idd)
    #l = [(','.join(k), v.cnt, ','.join(v.ids)) for k, v in s.iteritems()]
    l = [(len(k),v.cnt,  ' '.join(k),k) for k, v in s.iteritems()]
    l.sort(lambda x,y: y[1] - x[1])
    l.sort(lambda x,y: y[0] - x[0])

    for v in l:
        if v[0] ==1: 
            if nonpure_unigram_re.search(v[2]) or (v[1] < 1) or (v[2] in engwords and v[1] < 1) or (v[1] < 1 and (len(v[2])<4)):
                print>>sys.stderr,v[2]
                continue
        if v[0] ==1 or (v[0] == 3 and v[1]>1) or (v[0]==2) or (v[0]==4 and v[1]>1):
            print '|'.join(map(str, v[:-1]))
