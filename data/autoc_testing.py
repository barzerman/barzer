import threading, zipfile, socket, sys, time, random, string
# -*- coding: utf-8 -*-

letters = string.ascii_letters + string.digits

def randstring(n):
	return ''.join([random.choice(letters) for i in range(n)])

def send(s):
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.connect(("localhost", 5666))
	sock.send("<autoc u=\"48\">%s</autoc>\r\n.\r\n" % s)
	data = sock.recv(1024)
	string = ""
	while len(data):
		string = string + data
		data = sock.recv(1024)
	#print "*",
	sock.close()	

class AsyncStressAutoc(threading.Thread):
	def __init__(self, n):
		threading.Thread.__init__(self)
		self.n = n

	def run(self):
		t1 = time.time()
		for i in range(self.n):
			send(randstring(random.choice(lens)))
		t2 = time.time()
		self.t =  (t2-t1)
	t = 0.
	n = 1

	
def StressTestingRun(Nthreads, NQueries):
	lens = [0,1,2,3,4,5,6,7,10,1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4,5,6,7,10,50,100,500,1000]
	A = []
	for x in range(Nthreads):
		A.append(AsyncStressAutoc(NQueries))

	for x in A:
		x.start()

	print 'Waiting'

	for x in A:
		x.join() 
	for x in A:
		print "%s q in sec" % str(int(NQueries/x.t))

def NormalTestRun(filename, NQueries):
	f = open(filename, 'r')
	lines = f.readlines()
	for i in range(NQueries):
		time.sleep(0.7)
		question = random.choice(lines)
		print question[:-1] 
		for j in range(len(question)/2):
			time.sleep(0.2)
			send(question[:2*(j + 1)])

NormalTestRun("/home/nix/git/btesting/autoc/rusnames.txt", 10000)