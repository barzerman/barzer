#! env python

import sys
from subprocess import call

if len(sys.argv) < 3:
	print ("Usage: " + sys.argv[0] + " phrasefile.txt outfile.txt")
	sys.exit(1)

call(["./subtractor", sys.argv[1], sys.argv[1] + ".sub"])
f = open(sys.argv[2], "w")
call(["./Main", sys.argv[1] + ".sub"], stdout=f)
