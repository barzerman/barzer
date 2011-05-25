#!/usr/bin/python

import urllib


from HTMLParser import HTMLParser
from xml.sax.saxutils import escape, quoteattr
import re, string
from sets import Set


base_url = "http://www.bbc.co.uk/tv/programmes/a-z/by/%s/all"
letters = string.ascii_lowercase + string.digits

id_pat = re.compile("^\/programmes\/(\w+)")
punct_pat = re.compile("[%s]" % string.punctuation)
article_pat = re.compile('(.*)\,\s+(The|A)$')
split_pat = re.compile("([%s])|\s+" % string.punctuation)

classes = Set(["series", "brand", "episode"])

class MyParser(HTMLParser):
    shows = []
    inShow = False
    getText = False
    entryId = u''
    entryName = u''
    
    def clear(self):
        self.inShow = self.getText = False
        self.entryId = self.entryName = u''

    def handle_starttag(self, tag, attrs):
        amap = dict(attrs)
        if tag == 'li':
            self.inShow = amap.get("class") in classes
        elif tag == 'a' and self.inShow:
            m = id_pat.match(amap.get('href', ''))
            if m: self.entryId = m.group(1)
            else: self.clear()
        elif tag == 'span' and self.inShow:
            self.getText = True
    def handle_data(self, data):
        if self.getText:
            self.entryName += data
    def handle_charref(self, name):
        if self.getText:
            #print "getting ref for: %s" % name
            self.entryName += unichr(int(name));
#    def handle_entityref(self, name):
        #if self.inShow: #print "entity `%s'!" % name
    def handle_endtag(self, tag):
        if tag == 'li':
            if (self.inShow):
                self.shows.append((self.entryId, self.entryName))
                s = u"%s -> %s";
                #print (s % (self.entryId, self.entryName)).encode("utf-8")
            self.clear()  
        elif tag == 'span':
            self.getText = False


def mkEnt(id):
    print '<mkent i="%s" s="1" c="100" />' % id,

def processShows(shows):
    for (id, name) in shows:
        print '<stmt><pat>',
        s = article_pat.sub("\\2 \\1", name)
        for y in (x for x in split_pat.split(name) if x):
            if punct_pat.match(y):
                print '<p>%s</p>' % escape(y),
            else: print ('<t>%s</t>' % y).encode('utf-8'),
        print '</pat><tran>',
        mkEnt(id)
        print '</tran></stmt>'
 


"""

title_pat = "\s*\<span\s+class\=\"title\"\>([^<]+)\<\/span\>\s*"
href_pat = "\s*\<a\s+href=\"\/programmes\/(\w+)\"\>%s\<\/a>\s*" % title_pat
li_pat = "\<li\s+class\=\"brand\"\>%s\<\/li\>" % href_pat
pattern = re.compile(li_pat)

"""


def loadPage(s):
    print "loading %s" % s
    file = urllib.urlopen(s)
    #parser = MyParser()
    
"""  list = pattern.finditer(file.read())a
    for m in list:
        print "%s -> %s" % (m.group(2), m.group(1))a
"""




def main():
    parser = MyParser()
    url = base_url % 'a'
    parser.feed(urllib.urlopen(url).read().decode("utf-8"))
    print '<?xml version="1.0" encoding="UTF-8"?>'
    print '<stmset xmlns:xsi="http://www.w3.org/2000/10/XMLSchema-instance" xmlns="http://www.barzer.net/barzel/0.1">'
    processShows(parser.shows)
    print '</stmset>'

    #for l in letters:
    #    loadPage(base_url % l);
    


if __name__ == "__main__":
    main()

