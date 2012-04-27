import threading, zipfile, socket, sys, time, random, string
# -*- coding: utf-8 -*-
lens = [0,1,2,3,4,5,6,7,10,1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4,5,6,7,10,50,100,500,1000]
a = string.ascii_letters + string.digits

def randstring(n):
    return ''.join([random.choice(a) for i in range(n)])

def send(s):
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.connect(("localhost", 5667))
	sock.send("<autoc u=\"48\">%s</autoc>\r\n.\r\n" % s)
	data = sock.recv(1024)
	string = ""
	while len(data):
		string = string + data
		data = sock.recv(1024)
	#print "*",
	sock.close()	

class AsyncAutoc(threading.Thread):
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

Nthreads = 50
NQueries = 1000000


A = []
for x in range(Nthreads):
   A.append(AsyncAutoc(NQueries))

for x in A:
   x.start()

print 'Waiting'

for x in A:
   x.join() 
for x in A:
	print "%s q in sec" % str(int(NQueries/x.t))