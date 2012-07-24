#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import sys, operator
import BarzerClient
from lxml import etree
from lxml.etree import XMLSyntaxError
from Queue import Queue
from threading import Thread
import argparse

def usage(args):
	print """
Usage: 
queries in_filename [out_filename]  -  read queries line by line from file
test [in_filename]                  -  testing barz xmls
	"""
# querymap = {id: Query}
querymap = {}

#resultmap = {id: score}
resultmap = {}

#max id
max_id = 0

#lxml object with all right answers
xml_root = None

num_worker_threads = 8

class Query:
	def __init__(self, user,query, active = True):
		self.query = query.strip()
		self.user = user.strip()
		self.active = active

def load_data(queries_fname, tests_fname):
	def load_queries():
		global max_id
		for line in open(queries_fname, 'r'):
			active = line[0]!="#"
			chunks = line.split("<<<") if active else line[1:].split("<<<")
			if len(chunks) == 3:
				id = int(chunks[0])
				max_id = max(id,max_id)
				querymap[id] = Query(chunks[1], chunks[2],active)
	def load_xmls():
		global xml_root
		try:
   			f = open(tests_fname,'r') 
   			xml_root = etree.parse(f).getroot()
   			return
		except (IOError,XMLSyntaxError) as e:
   			print tests_fname + ": " + str(e)

		xml_root = etree.Element("tests")

	load_queries()
	load_xmls()


def save_data(queries_fname, tests_fname):

	def save_queries():	
		f = open(queries_fname,'w')
		for (i,q) in querymap.items():
			f.write(("" if q.active else "#")+str(i) + "<<<" + q.user +  "<<<" + q.query + "\n")

	def save_xmls():
		global xml_root
		f = open(tests_fname,'w')
		f.write(etree.tostring(xml_root,pretty_print=True,  encoding='utf-8'))

	save_queries()
	save_xmls()

def update_one(id):
	if not querymap[id].active:
		print "Query with id #"+str(id)+" is disabled. You can use: -a enable -id " + str(id)
		return 
	bc = BarzerClient.BarzerClient(BarzerClient.DEFAULT_HOST,BarzerClient.DEFAULT_PORT)
	new_barz = bc.query(querymap[id].query.decode("utf-8"), querymap[id].user)
	new_barz = etree.fromstring(new_barz)
	if (new_barz.tag == "error"):
		print "query #" + str(id) + ": BarzerClient: " + new_barz.text
		return
	new_barz.set("id", str(id))
	old_barz = xml_root.find('./*[@id="'+str(id)+'"]')
	if old_barz is not None:
		xml_root.replace(old_barz, new_barz)
		print "Xml with id #" + str(id)+ " has been replaced"
	else:
		xml_root.append(new_barz)
		print "Xml with id #" + str(id)+ " was created"

def update():
	counter = 0
	for (i,q) in querymap.items():
		if q.active:
			counter+=1
			update_one(i)
	print str(counter) + " tests has been updated"

def check_and_update():
	counter = 0
	for (i,q) in querymap.items():
		if q.active:
			counter+=1
			xml_query = xml_root.find('./*[@id="'+str(i)+'"]/query')
			if xml_query is not None:
				if q.query.decode("utf-8")!=xml_query.text:
					print "ole query: " + q.query + "\nnew query: " + xml_query.text
					update_one(i)
			else:
				update_one(i)
	print str(counter) + " tests has been updated"

def bulk_update(ids):
	for id in ids.split(","):
		update_one(id)

def append(user, query):
	max_id += 1
	querymap[max_id] = Query(user, query)
	return max_id - 1

def do_test(barz):
	user = barz.get("u")
	id = int(barz.get("id"))
	bc = BarzerClient.BarzerClient(BarzerClient.DEFAULT_HOST,BarzerClient.DEFAULT_PORT)
	new_xml = bc.query(querymap[id].query.decode("utf-8"), user)
	resultxml = bc.match_xml(new_xml, etree.tostring(barz, encoding='utf-8'))
	resultxml = etree.fromstring(resultxml)
	if (resultxml.tag == "error"):
		print "query #" + str(id) + ": BarzerClient: " + resultxml.text
		return
	resultmap[int(id)] = int(resultxml[0].text)

def test():
	def worker():
	    while True:
	        barz = q.get()
	        do_test(barz)
	        q.task_done()
	q = Queue()
	for i in range(num_worker_threads):
	    t = Thread(target=worker)
	    t.setDaemon(True)
	    t.start()
	for t in xml_root.iter("barz"):
		q.put(t)
	q.join()

def test_one(id):
	barz = xml_root.find('./*[@id="'+str(id)+'"]')
	if barz is not None:
		do_test(barz)
	else:
		print "query #" + str(id) + ": No such barz test.\nMaybe you need to run: -a update -id "+ str(id)

def bulk_test(ids):
	for id in ids.split(','):
		test_one(id)

def disable_user(ids):
	for userid in ids.split(','):
		for (i,q) in querymap.items():
			if (q.user == userid):
				q.active = False
	print "Queries with user id " + ids + " have been disabled"

def enable_user(ids):
	for userid in ids.split(','):
		for (i,q) in querymap.items():
			if (q.user == userid):
				q.active = True
	print "Queries with user id " + ids + " have been enabled"

def enable_all():
	counter = 0
	for (i,q) in querymap.items():
		q.active = True
		counter +=1
	print "All "+str(counter) + " queries have been enabled"


def unknow_action():
	print "Unknow action"

def show_max_id():
	print "The biggest id used is " + str(max_id)

def report():
	passed = 0
	not_passed = 0
	for (i,v) in resultmap.items():
		if v==0:
			passed+=1
		else:
			not_passed+=1
	if passed + not_passed != 0:
		for i in sorted(resultmap.iteritems(), key=operator.itemgetter(1)):
			print "#" + str(i[0]) + "\t:" + str(i[1])
		print "----------------------" 
		print "Passed: " + str(passed)
		print "Not passed: " + str(not_passed)
def main():
	parser = argparse.ArgumentParser(description='Barzer autotesting script',
	 epilog="""Note that update action synchronizes query file with test file,
	 			 while full_update regenerates the test whole file.
	 			 max_id acrion shows the biggest id used in query file""")
	parser.add_argument('-a', action="store", help="Action to do", choices=('test', 'update','full_update','disable_user', 'max_id','enable_user'), required = True ) 
	parser.add_argument('-id', action='store', default="all", help='Action arguments can be id or id1,id2,id3. Makes sense only for test and update actions')
	parser.add_argument('-q','--query_fname', default="queries", help='File with queries in format like\nid<<<user_id<<<query')
	parser.add_argument('-x', '--xml_fname', default="answers.xml", help='File with barz xmls in format      <tests><barz id="">...<barz>....</tests>')
	args = parser.parse_args(sys.argv[1:])
#	print args.a, args.id
#	return 
	try:
		load_data(args.query_fname, args.xml_fname)
	except Exception as e:
		print e
		return
	if args.id == "all":
		{'test': test,
		 'update': check_and_update,
		 'full_update':update,
		 'max_id':show_max_id
		}.get(args.a,unknow_action)()
	else:
		{'test': bulk_test,
		 'update': bulk_update,
		 'disable_user':disable_user,
		 'enable_user':enable_user 
		}.get(args.a,unknow_action)(args.id)
	report()
	save_data(args.query_fname, args.xml_fname)
	
if __name__ == "__main__":
    main()
