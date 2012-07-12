#! /usr/bin/python2.7 -i

import sys
sys.path.append('.')

import pybarzer

barz = pybarzer.Barzer()
barz.init(["shell", "-home", "/usr/share/barzer", "-cfg", "/usr/share/barzer/site/alpha/config.xml"])
barz.universe("34")

str1 = """<barz u="34">
<bead n="1">
    <token>je</token>
    <srctok>je</srctok>
</bead>
<bead n="2">
    <token>suis</token>
    <srctok>suis</srctok>
</bead>
<bead n="3">
    <token>francais</token>
    <srctok>francais</srctok>
</bead>
</barz>"""
str2 = """<barz u="34">
<bead n="1">
    <token>je</token>
    <srctok>je</srctok>
</bead>
<bead n="2">
    <token>suis</token>
    <srctok>suis</srctok>
</bead>
<bead n="3">
    <token>francais</token>
    <srctok>francais</srctok>
</bead>
</barz>"""
str3 = """<barz u="34">
<bead n="1">
    <token>je</token>
    <srctok>je</srctok>
</bead>
<bead n="2">
    <token>suis</token>
    <srctok>suis</srctok>
</bead>
<bead n="3">
    <token>italien</token>
    <srctok>italien</srctok>
</bead>
</barz>"""

defSettings = pybarzer.CompareSettings()
print(barz.matchXML(str1, str2, defSettings))
print('\n')
print(barz.matchXML(str1, str3, defSettings))