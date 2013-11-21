#!/usr/bin/python

import urllib2, sys, time
from bs4 import BeautifulSoup
from optparse import OptionParser

usage_str = "usage: %prog [options]"
cmd_parse = OptionParser(usage_str)
cmd_parse.add_option("-f", "--file", dest="filename", default=None, help="input file", metavar="FILE")
cmd_parse.add_option("-d", "--dir", dest="outdir", default='.', help="output directory", metavar="DIR")

class UrlIterator: 
    def __init__(self,inStr): # input stream (file or stdin)
        self.inStr=inStr
    
try:
    (options, args) = cmd_parse.parse_args()
except:
    print "wrong cmd args"
    cmd_parse.print_help()
    exit(1)

inFile=sys.stdin if options.filename is None else open(options.filename,'r')

url_base='https://www.healthcare.gov'
i=0
for line in inFile:
    (u,name) = line.strip().split('|')
    url=url_base+u
    page=urllib2.urlopen( url )
    soup=BeautifulSoup( page.read() )
    title=soup.find_all('h2')[1].text
    print '{0}|{1}|{2}'.format( u.split('/')[-1], title, soup.find_all('p')[0] )
    if i%7 == 0:
        time.sleep(1)

#print options.filename
