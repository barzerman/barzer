#!/usr/bin/python

# ngram rules 
# 3gram n is good if for every subngram ns P(ns) is contained in P(n) 
# 2gram n is good if n is not contained in any valid 3gram AND P(ns) is contained in P(n) in 90% of cases
# 1gram n is good if n is not contained in any valid 2gram or 3gram and n  

tokenizer_re = re.compile(r'[\n;.\-,|()\[\]\'" ]')
import Stemmer 

stemmer=Stemmer.Stemmer('english')

g_phrase=[]
g_ngram = {}

def update_ngram(pidx, tup, stup):
    print pidx,tup,stup


def generate_ngrams(pidx,tok)
    l=len(tok)
    l_1=l-1
    l_2=l-2

    for i in range(0,l):
        t=tok[i]
        update_ngram( (t), pidx ) 
        if i < l_1:
            t_1=tok[i+1]
            update_ngram( pidx, (t[0],t_1[0]), (t[1],t_1[1]) )
            if i < l_2:
                t_2=tok[i+2]
                update_ngram( pidx, (t[0],t_1[0],t_2[0]), (t[1],t_1[1],t_2[1]) )
     
def read_phrase(docid,phrase):
    pidx=len(g_phrase)
    prestemTok=tokenizer_re.split(phrase)
    stemTok=stemmer.stemWords( prestemTok )
    tok=zip( stemTok, prestemTok )

    generate_ngrams(pidx,tok)
    g_phrase.push(
        {
            'orig': phrase,
            'stem': stemTok,
        }
    )
    

def init_files():
    print 'initializin files'
    phrfilename='phrases.txt'
    env = { 'phrasefile' : open( phrfilename, 'r' ) }
    return env

def process_phrases(env):
    for l in env.phrasefile:
        (docid,d1,d2,phrase)= l.strip().split('|')
        read_phrase(docid,phrase)

env=init_files()
process_phrases(env)
process_ngrams(env)
