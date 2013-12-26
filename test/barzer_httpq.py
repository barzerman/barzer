#!/usr/bin/python

#queries a running server barzer.exe http -p PORT -cfg XXX -home XXX

import sys, argparse, json, urllib2, urllib

def init_env(parser): # parser is argparse.ArgumentParser
    parser.add_argument('-i', '--infile', help='input file')
    parser.add_argument('-o', '--outfile', help='output file')
    parser.add_argument('-p', '--port', help='port number')
    parser.add_argument('-u', '--user', help='user id')
    args=parser.parse_args()
    
    try:
        env = {}
        if 'infile' in args and args.infile != None:
            env[ 'infile' ] = { 'file': open( args.infile, 'r' ), 'name': args.infile  }
        else:
            env[ 'infile' ] = { 'file': sys.stdin, 'name': ''  }

        if 'outfile' in args and args.outfile != None:
            env[ 'outfile' ] = { 'file': open( args.outfile, 'w' ) , 'name': args.outfile  }
        else:
            env[ 'outfile' ] = { 'file': sys.stdout, 'name': '' }

        env[ 'port' ]= args.port if args.port != None else 5767
        env[ 'user' ]= args.user if args.user != None else 1000106

        env[ 'success' ] = True
    except:
        return {}
    return env

def http_query(env,q):
    url='http://localhost:8080/query?'
    values = { 'u':env['user'], 'ret' : 'json', 'q': q.encode('utf-8') }
    data= urllib.urlencode(values)
    url = url + data;
    f=urllib2.urlopen( url + data )
    page=f.read()
    return page


def beads_are_good(b):
    l=len(b)
    if l == 0:
        return False

    junk=0.0
    entcnt=0.0
    junkTypeSet={ 'token', 'number' }
    entTypeSet={ 'ent', 'ercexpr', 'erc' }
    for i in b:
        if i['type'] in entTypeSet:
            entcnt+= 1.0
        if i['type'] in junkTypeSet:
            junk+=1.0
        elif 'confidence' in i and i['confidence']<2:
            junk+=0.5
    print entcnt,junk
    if junk > 0.0:
        if entcnt/junk >0.3:
            return True
        return False
    else:
        return True
    
env=init_env(argparse.ArgumentParser(description='running bulk query test'))

for i in env[ 'infile' ]['file']:
    try:
        q = i.strip().decode('utf-8')
        #qs=bc.query( q, env['user' ], ret='json' )
        qs=http_query( env, q )
        o=json.loads(qs,encoding='utf-8')
        if beads_are_good(o['beads']):
            print q
    except:
        print 'WEIRD', i.strip()
