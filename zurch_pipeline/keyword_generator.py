#!/usr/bin/python

# ngram rules 
# 3gram n is good if for every subngram ns P(ns) is contained in P(n) 
# 2gram n is good if n is not contained in any valid 3gram AND P(ns) is contained in P(n) in 90% of cases
# 1gram n is good if n is not contained in any valid 2gram or 3gram and n  

import Stemmer  #PyStemmer
import argparse, re, sys

tokenizer_re = re.compile(r'[\n;.,|()\[\]\" ]')
stemmer=Stemmer.Stemmer('english')

"""
g_phrase=[]
g_ngram = {}
"""

def subgram_generator(a):
    l=len(a)
    for i in range(0,l):
        r=range(0,min(l-i,l-1))
        for j in r:
            u=1+i+j
            yield (a[i]) if u==i+1 else a[i:1+i+j]

def update_ngram(env, pidx, tup, stup):
    g_ngram = env['g_ngram']
    if tup in g_ngram:
        g_ngram[ tup ]['phrase'].add( pidx );
    else:
        g_ngram[ tup ] = { 'phrase' : set([pidx]), 'prestem': stup };

def generate_ngrams(env, pidx,tok):
    l=len(tok)
    l_1=l-1
    l_2=l-2

    for i in range(0,l):
        t=tok[i]
        update_ngram(env, pidx,t[0],t[1] ) 
        if i < l_1:
            t_1=tok[i+1]
            update_ngram(env, pidx, (t[0],t_1[0]), (t[1],t_1[1]) )
            if i < l_2:
                t_2=tok[i+2]
                update_ngram(env, pidx, (t[0],t_1[0],t_2[0]), (t[1],t_1[1],t_2[1]) )
     
def read_phrase(env, docid, phrase):
    g_phrase = env['g_phrase']
    pidx=len(g_phrase)
    prestemTok=tokenizer_re.split(phrase)
    stemTok=stemmer.stemWords( prestemTok )
    tok=zip( stemTok, prestemTok )

    generate_ngrams(env, pidx,tok)
    g_phrase.append(
        {
            'orig': phrase,
            'stem': stemTok,
        }
    )
    

def init_env(parser):
    phrfilename='phrases.txt'
    parser.add_argument('-i', '--infile', help='input file')
    parser.add_argument('-o', '--outfile', help='output file')
    args=parser.parse_args()

    try:
        env = {'g_phrase': [], 'g_ngram': {}}
        if 'infile' in args and args.infile != None:
            env[ 'infile' ] = { 'file': open( args.infile, 'r' ), 'name': args.infile  }
        else:
            env[ 'infile' ] = { 'file': sys.stdin, 'name': ''  }

        if 'outfile' in args and args.outfile != None:
            env[ 'outfile' ] = { 'file': open( args.outfile, 'w' ) , 'name': args.outfile  }
        else:
            env[ 'outfile' ] = { 'file': sys.stdout, 'name': '' }
            
        env[ 'success' ] = True
    except:
        return {}
    return env

def process_phrases(env):
    errCnt=0
    lineCnt=0
    for l in env['infile']['file']:
        lineCnt = lineCnt+1
        try:
            (docid,d1,d2,phrase)= tuple(l.strip().split('|'))
            read_phrase(env, docid,phrase)
        except:
            errCnt=errCnt+1
    
    print >>sys.stderr, '{0} lines read with {1} errors'.format( lineCnt, errCnt )

def includes_subngram_idx(env, ngram):
    g_ngram = env['g_ngram']
    ps=g_ngram[ ngram ][ 'phrase' ]
    for i in subgram_generator(ngram):
        if not g_ngram[i]['phrase'] <= ps:
            return False
    return True

def print_ngram( n,env ):
    g_ngram = env['g_ngram']
    i=g_ngram[n]['prestem']
    f=env['outfile']['file']
    if isinstance(i, basestring):
        f.write( i+'\n' )
    elif len(i) == 2:
        f.write( '{0} {1}\n'.format( i[0], i[1] ) )
    elif len(i) == 3:
        f.write( '{0} {1} {2}\n'.format( i[0], i[1], i[2] ) )
    else:
        for x in i:
            f.write(x)
        f.write('\n') 

def process_ngrams(env):
    g3=[]
    g2=[]
    g1=[]
    g_ngram = env['g_ngram']
    for i in g_ngram:
        if not isinstance(i, basestring):
            if len(i) == 3:
                g3.append(i)
            elif len(i) == 2:
                g2.append(i)
        else:
            g1.append(i)

    good_g3= set()
    for i in g3:
        if includes_subngram_idx(env, i):
            good_g3.add(i)

    good_g2= set()
    for i in g2:
        i0=i[0]
        i1=i[1]
        if includes_subngram_idx(env, i):
            shouldAdd=True
            for j in good_g3:
                if ( (i[0] == j[0] and i[1]== j[1]) or (i[0] == j[1] and i[1] == j[2]) ):
                    shouldAdd=False
                    break
            if shouldAdd:
                good_g2.add(i)

    good_g1=set()
    for i in g1: 
        shouldAdd = False
        for j in good_g2:
            if i not in j:
                shouldAdd= True
        if shouldAdd:
            for j in good_g3:
                if i in j:
                    shouldAdd=False
                    break
        if shouldAdd:
            good_g1.add( i )
                
                
    print >>sys.stderr, 'Before processing:', len(g3),'3grams, ', len(g2), '2grams ,', len(g1), ' 1grams'
    print >>sys.stderr, 'After processing:', len(good_g3),'3grams, ', len(good_g2), '2grams ,', len(good_g1), ' 1grams'
    for i in good_g3:
        print_ngram(i,env)
    for i in good_g2:
        print_ngram(i,env)
    for i in good_g1:
        print_ngram(i,env)
        

def main():
    parser = argparse.ArgumentParser(description='autmated keyword extraction')

    env=init_env(parser)
    if 'success' in env:
        process_phrases(env)
        process_ngrams(env)
if __name__ == '__main__':
   sys.exit(main())
