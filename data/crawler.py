#!/usr/bin/python
# -*- coding: utf-8 -*-

from sets import Set
import urllib2, urlparse, codecs, sys, re
import re,string

base_url='http://www.apinfo.ru/airports/iata.html'

for i in string.ascii_uppercase:
    url=base_url+'?'+i
    try:
        response=urllib2.urlopen(url)
        encoding=response.headers['content-type'].split('=')[1]
        print encoding
        html=response.read()
     
        print unicode(html,encoding).encode('utf-8')
        #print unicode(html,encoding)
    except Exception as e:
        print( e )
