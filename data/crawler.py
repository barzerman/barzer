#!/opt/local/bin/python2.7
# -*- coding: utf-8 -*-

from sets import Set
import urllib2, urlparse, codecs, sys, re
import re
utf8_re=re.compile(u'[^а-яА-Я\n]+',re.UNICODE)
nlpat=re.compile('\n+')

writer = codecs.getwriter('utf-8')(sys.stdout)

history = set()
queue = set()
#data = open('rawdata', 'w')
log = open('log', 'w')
error = open('error', 'w')
except_types = {'exe', 'zip', 'rar', '7z', 'swf', 'css' ,'msi','js' ,'xml'}

# if proxy
"""proxy_url = "http://n.chepanov:p9tbuwHR@192.168.1.1:3128" 
proxy_support = urllib2.ProxyHandler({'http': proxy_url}) 
opener = urllib2.build_opener(proxy_support) 
urllib2.install_opener(opener) """
# endif proxy

#urlre = re.compile(r'<a\s*href=[\'|"](.*?)[\'"].*?>')
urlre = re.compile(u"""href=['"]?([^'"\s>]+)""",re.UNICODE)

def makeAbsoluteURL(href, base_url, dmn):
    if href.startswith('/') or href.startswith('.'):
        return  "http://" + base_url[1] + href
    elif href.startswith('#'):
        return "http://" + base_url[1] + base_url[2] + href
    elif not href.startswith('http'):
        return "http://" + base_url[1] + "/" + href

    return href

def allowed(href,dmn):
    if not href.split('.')[-1] in except_types:
        if urlparse.urlparse(href)[1].endswith(dmn):
            return True    
    return False



for arg in sys.argv:
    print arg
    print arg
    domain = urlparse.urlparse(arg)[1]
    queue.add(arg) 

    while len(queue) > 0 :
        url = queue.pop()
        if (url in history):
            continue
        history.add(url)
        #print "FUCK", len(history)
        try:           
            site = urllib2.urlopen(url)
        except:
            try:
                error.write("Can't open " + url + '\n')
            except:
                error.write("Can't open url")
            continue
        charset = site.headers.getheader('content-type').split('charset=')[-1]
        try:
            reader = codecs.getreader(charset)(site)
        except:
            reader = codecs.getreader('windows-1251')

        url = urlparse.urlparse(url)
        try:
            page = reader.read()
        except:
            continue
        #hrefs = urlre.findall(page)
        #writer.write(page)

        links = set()
        for href in urlre.findall(page):
            if not href.startswith('#') and not href in links:
                links.add(href)
                thisurl = makeAbsoluteURL(href, url, domain)
                if allowed(thisurl,domain):
                    if (not thisurl in history and not thisurl in queue ):
                        queue.add(thisurl)
                        #history.add(thisurl)
                        #print "CUNT",thisur
        ss=utf8_re.sub(' ',page+" ");
        writer.write(ss)
    
    queue=set()
    history=set()
