#!/opt/local/bin/python
# -*- coding: utf-8 -*-

import sys

import barzer_client
bc=barzer_client.BarzerClient("localhost", 5666)

queryfile= sys.argv[1] if len(sys.argv)>1 else "query.txt"

with open( queryfile ) as f:
    for i in f:
        b=i.split('|')
        print bc.query(unicode(b[1],"utf8"), b[0])
        
