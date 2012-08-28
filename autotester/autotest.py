#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import sys, operator
import BarzerClient # must not write anything to standart output (!)
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import iterparse
from Queue import Queue
from threading import Thread,Lock
import argparse
import datetime, time

now = datetime.datetime.now()
lock = Lock()

DELIM = "<<<"
NUM_WORKER_THREADS = 24
LEVEL = 0
BARZER_HOST = BarzerClient.DEFAULT_HOST
BARZER_PORT = BarzerClient.DEFAULT_PORT

statistics = {"passed": 0, "not_passed": 0 ,"updated": 0, "error": 0, "over_level":0, "total": 0, "time":""}

class Query:
	"""\
	Represents a single test query
	"""
	def __init__(self, user,query):
		self.query = query.strip()
		self.user = user.strip()
		self.bc = BarzerClient.BarzerClient(BARZER_HOST, BARZER_PORT)

	def ask_barzer(self):	
		return self.bc.query(self.query.decode("utf-8"), self.user)

	def get_barzer(self):
		return self.bc


def generate(args):
	connectivity_error = False
	tests = open(args.tests_fname,'w')
	tests.write('<tests gen_time="' +now.strftime("%Y-%m-%d %H:%M")+'">')
	for line in open(args.queries_fname, 'r'):
		line.strip()
		if line[0]!="#":
			chunks = line.split(DELIM)
			if len(chunks) > 2:
				id = int(chunks[0].strip())
				q = Query(chunks[1].strip(), chunks[2].strip())
				barzxml = q.ask_barzer()
				if barzxml.startswith('<barz error="Connectivity error"'):
					connectivity_error = True
					statistics["error"] += 1
					break
				if barzxml.startswith('<error>invalid user id'):
					print '<error id="' + str(id) + '"> Invalid user id ' + str(q.user) + '</error>'
					statistics["error"] += 1
					continue
				statistics["updated"] += 1
				tests.write('\n<test id="' + str(id) + '">')
				tests.write(barzxml)
				tests.write('</test>')
	if connectivity_error:
		print '<error> No connection to barzer (' + BARZER_HOST + ":" + str(BARZER_PORT) + '</error>'		
	tests.write("</tests>")

def do_test(test):
	global LEVEL
	q = Query(test[1], test[2])
	new_xml = q.ask_barzer()
	resultxml = q.get_barzer().match_xml(new_xml, test[3])
	resultxml = ET.fromstring(resultxml)
	if (resultxml[0].tag == "score"):
		score = int(resultxml[0].text)
		lock.acquire() 
		statistics["passed" if score == 0 else "not_passed"] += 1
		statistics["total"] += 1
		if score > LEVEL:
			statistics["over_level"] += 1
			test[4].put((str(score), test[0], test[1],test[2]))
		lock.release()

def run_all(args):

	def worker():
	    while True:
	        test = q.get()
	        do_test(test)
	        q.task_done()
	def print_worker():
		while True:
			r = print_queue.get()
			print '<test score="' + r[0] + '" id="' + r[1] + '" user="' + r[2] +'">' + r[3] + '</test>'
			print_queue.task_done()
	q = Queue()
	for i in range(NUM_WORKER_THREADS):
	    t = Thread(target=worker)
	    t.setDaemon(True)
	    t.start()

	print_queue = Queue()
	t=Thread(target=print_worker)
	t.setDaemon(True)
	t.start()
	for event, elem in iterparse(args.tests_fname, events = ('end',)):
		if elem.tag =="test":
			id = elem.get("id")	
			barz = elem.find("./barz")
			user = barz.get("u")
			testxml = ET.tostring(barz, encoding="utf-8")
			query = barz.find("./query").text.encode("utf-8")
			if (query is not None) and (user is not None) and (id is not None) and (testxml is not None):
				q.put((id, user, query, testxml, print_queue))
	q.join()
	print_queue.join()

def default_action():
	print "Type:\n -r for running tests\n -g for generating tests\n -h for more information:"

def print_stat(args):
	global statistics
	print '<stat',
	for (k,v) in statistics.items():
		if args.generate and k in ["updated", "error", "time"]:
		 	print k + '="' + str(v) + '"' ,
		if args.run and k in ["total","passed", "not_passed", "over_level",  "time"]:
			print k + '="' + str(v) + '"' ,
	print '></stat>'

def main():
	global BARZER_PORT, BARZER_HOST, LEVEL
	parser = argparse.ArgumentParser(description='Barzer autotesting script',
	 epilog="""Run autotest.py -g first to generate file with correct responses""")
	parser.add_argument('-g', '--generate',action="store_true", default=False, help="generate xml with correct answers") 
	parser.add_argument('-r','--run', action='store_true', default=False, help='run all the tests')
	parser.add_argument('-l', '--level',action='store', default=0, type=int, help='show tests result with score higher than level. Use level = -1 to show all the results. Default level: 0 ')
	parser.add_argument('-in','--queries_fname', default="queries", help='File with queries in format: id<<<user_id<<<query. Default filename: queries')
	parser.add_argument('-out', '--tests_fname', default="answers.xml", help='File with barz xmls. Can be generated using -g option. Default filename: answers.xml')
	parser.add_argument('-barzer', default=BARZER_HOST+ ":" + str(BARZER_PORT), help="barzer instance location. Default: "+BARZER_HOST+ ":" + str(BARZER_PORT))
	args = parser.parse_args(sys.argv[1:])

	if not (args.generate or args.run):
		default_action()
		return
	if args.level:
		LEVEL = args.level
	try:
		if args.barzer:
			b_arg  = args.barzer.split(":")
			if len(b_arg):
				BARZER_HOST = b_arg[0]
			if len(b_arg) > 1:
				BARZER_PORT = int(b_arg[1])
	finally:
		BARZER_HOST = BarzerClient.DEFAULT_HOST
		BARZER_PORT = BarzerClient.DEFAULT_PORT
	print '<autotest time="'+ now.strftime("%Y-%m-%d %H:%M") + '" level="'+ str(LEVEL) + '">'
	t1 = time.time()
	if args.generate:
		generate(args)
	elif args.run:
		run_all(args)
	t2 = time.time()
	m, s = divmod(t2-t1, 60)
	h, m = divmod(m, 60)
	statistics["time"] = "%d:%02d:%02d" % (h, m, s)
	print_stat(args)
	print '</autotest>'
if __name__ == "__main__":
    main()
