#!/usr/bin/python
# -*- coding: utf-8 -*-


import fileinput, codecs, sys, re,pybarzer
from collections import defaultdict
from itertools import imap, ifilter
barzer=pybarzer.Barzer()
TOK_RE = re.compile(u'\W+', re.UNICODE)

MAX_NGRAM = 3
STOP_BEGIN = {u'и',u'или',u'а',u'ли',u'либо',u'нибудь',u'котором'}
STOP_END = {u'в',u'на',u'из',u'под',u'перед',u'за',u'над',u'через',u'и',u'не',
            u'или',u'без',u'а',u'по',u'с',u'со',u'от',u'для',u'до',u'к',
            u'котором'}

prestemNgram={}

def stem(s):
    return s

NORM_DIGIT_RE = re.compile('[\d\.\,]+')
def normalize(s):
    s = NORM_DIGIT_RE.sub(' N ', s)
    return s

def tokenize(s):
    s = normalize(s)
    xs = TOK_RE.split(s)
    return map(stem, ifilter(bool, xs))

def ngramize(lst):
    l = len(lst)
    for i in xrange(l):
        xs = []
        if lst[i] in STOP_BEGIN: continue
        for j in xrange(i, min(i+MAX_NGRAM, l)):
            w = lst[j]
            xs.append(w)
            if w in STOP_END: continue
            yield tuple(xs)


class Stat(object):
    def __init__(self):
        self.word_cnt = 0
        self.stat = defaultdict(int)
    def add_chunk(self, phrase):
        self.word_cnt += len(phrase)
    def add_ngram(self, ngram):
        self.stat[ngram] += 1
    def get_stat(self):
        for k, v in self.stat.iteritems():
            yield [unicode(len(k)), unicode(v), u','.join(k)]

def read_file(fd):
    docs = defaultdict(Stat)
    corpus = Stat()
    
    for line in fd:
        line = line.strip().decode('utf-8')
        if not line: continue
        docid, t, chunk_n, txt = line.split('|', 3)
        doc = docs[docid]
        lst = tokenize(txt)
        doc.add_chunk(lst)
        for n in ngramize(lst):
            stemN=tuple( barzer.stem(x.encode('utf-8')) for x in n )
            if stemN not in prestemNgram :
                prestemNgram[ stemN ] = n
                newTuple=n
            else:
                newTuple=prestemNgram[ stemN ]
            corpus.add_ngram(newTuple)
            doc.add_ngram(newTuple)
            #corpus.add_ngram(n)
            #doc.add_ngram(n)
    
    return docs, corpus
    

def render(fd, l):
    fd.write(u"{}\n".format(u'|'.join(l)).encode('utf-8'))

def main():
    docs, corpus = read_file(fileinput.input())

    
    with open('doc_out.txt', 'wb') as do, open('corp_out.txt', 'wb') as co:
        for l in corpus.get_stat():
            render(co, l)
        
        for docid, doc in docs.iteritems():
            for l in doc.get_stat():
                render(do, [docid]+l)

if __name__ == '__main__':
    sys.exit(main())
    
        

    #l = [(','.join(k), v.cnt, ','.join(v.ids)) for k, v in s.iteritems()]
    """
    l = [(len(k),v.cnt,  ' '.join(k),k) for k, v in s.iteritems()]
    l.sort(lambda x,y: y[1] - x[1])
    l.sort(lambda x,y: y[0] - x[0])
    """

