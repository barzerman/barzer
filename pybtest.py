#!/opt/local/bin/python -i
# -*- coding: utf-8 -*-

import pybarzer
b=pybarzer.Barzer()
b.init(['shell','-home','/usr/share/barzer','-cfg','/usr/share/barzer/site/alpha/config.xml'])
bproc=b.mkProcessor()
