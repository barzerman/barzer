#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import BarzerClient
import sys
from lxml import etree
from Queue import Queue
from threading import Thread

num_worker_threads = 8

class TestResult:
	def __init__(self, query,user, resultxml):
		self.query = query
		self.user = user
		root = etree.fromstring(resultxml)
		self.score = int(root.find("score").text)

tests = []

def usage(args):
	print """
Usage: 
queries in_filename [out_filename]  -  read queries line by line from file
test [in_filename]                  -  testing barz xmls
	"""

def read_queries(config):
	f = open(config['infile'], 'r')
	out = open(config['outfile'], 'w')
	bc = BarzerClient.BarzerClient(BarzerClient.DEFAULT_HOST,BarzerClient.DEFAULT_PORT)
	lines = f.read().splitlines()
	user = "0"
	if len(lines) > 0:
		user = lines[0]
	else:
		print "No user specified. Root user used"
	out.write("<test>")
	for q in lines[1:]:
		out.write(bc.query(q.decode("utf-8"),user))
	out.write("</test>")
	out.close()


def test_file(config):

	def do_test(t):
		query = t.find("./query").text
		user = t.get("u")
		bc = BarzerClient.BarzerClient(BarzerClient.DEFAULT_HOST,BarzerClient.DEFAULT_PORT)
		new_xml = bc.query(query, user)
		resultxml = bc.match_xml(new_xml, etree.tostring(t, encoding='utf-8'))
		tests.append(TestResult(query,user, resultxml))

	def worker():
	    while True:
	        test = q.get()
	        do_test(test)
	        q.task_done()

	tree = etree.parse(config['infile'])
	root = tree.getroot()
	q = Queue()
	for i in range(num_worker_threads):
	    t = Thread(target=worker)
	    t.setDaemon(True)
	    t.start()
	for t in root.iter("barz"):
		q.put(t)
	q.join()


def read_args(args):
	config = {'mode':'help', 'infile':None, 'outfile':None}
	if len(args) < 2:
		return config	
	mode = args[1]
	if mode in ["queries", "q"]:
		config['mode'] = "queries"
		config['infile']=args[2]
		config['outfile']=args[3] if len(args) > 3 else 'tests.xml'
	elif mode in ["test","t"]:
		config['mode'] = "test"
		config['infile']= args[2] if len(args) > 2 else 'tests.xml'
	else:
		return config
	return config

def main():
	config = read_args(sys.argv)
	{'test': test_file,
	 'queries': read_queries,
	 'help': usage,
	}.get(config['mode'], usage)(config)
	for t in sorted(tests, key=lambda TestResult: TestResult.score, reverse=True):
		print str(t.score) + "||" + t.user +  "||" + t.query.encode("utf-8")
	
if __name__ == "__main__":
    main()
