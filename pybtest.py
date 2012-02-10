#!/opt/local/bin/python -i

import pybarzer
b=pybarzer.Barzer()
b.init(['shell','-cfg','/usr/share/barzer/site/alpha/config.xml'])
bproc=b.mkProcessor()

