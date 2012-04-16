#!/usr/bin/python
# -*- coding: utf-8 -*-
import os

#
# The input format is:
#	line:	data:
#	1 		class subclass
#	2 		ID_base|name1|name2| ...|1 1
#	3 		ID1|name11|name12| ... |amount_of_BASE_in_one_ID1_units
#	4  		ID2|name21|name22| ... |amount_of_BASE_in_one_ID2_units
#	5 		...

# If line starts with '#' sign it'll be ignored
#

# Example:
#	1 4
#	m|meter|метр|1
#	cm|centimeter|0.01 


HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<stmset xmlns:xsi="http://www.w3.org/2000/10/XMLSchema-instance" xmlns="http://www.barzer.net/barzel/0.1">\n"""

MACRO = """<stmt m="translate_into" n="0"><pat>
<opt>
<any><t>в</t><t>in</t><t>через</t><t>into</t>
</any>
</opt>
</pat>
</stmt>
"""
BASE_TO_BASE = """
<pat>
  <any v="in">
    <erc c="%(class)s" s="%(sclass)s" t="%(base)s"/>
  </any>
  <expand n="translate_into"/>
  <ent c="%(class)s" s="%(sclass)s" t="%(base)s"/>
</pat>
<tran u="y">
  <var n="in"/>
</tran>
</stmt>
"""

TO_BASE = """
<pat>
  <list v="in">
      <erc c="%(class)s" s="%(sclass)s" t="%(in_id)s"/>
  </list>
  <expand n="translate_into"/>
  <list v="end">
    <ent c="%(class)s" s="%(sclass)s"/>
  </list>
</pat>
<tran>
  <func name="set">
    <func name="opMult">
      <var n="in"/>
      <rn v="%(scale)s"/>
    </func>
    <mkent c="%(class)s" s="%(sclass)s" i="%(base)s"/>
  </func>
  <var n="end"/>
</tran>
"""

FROM_BASE = """<pat>
  <list v="in">
    <erc c="%(class)s" s="%(sclass)s" t="%(base)s"/>
  </list>
  <expand n="translate_into"/>
  <ent c="%(class)s" s="%(sclass)s" t="%(out_id)s"/>
</pat>
<tran>
  <func name="set">
    <func name="opDiv">
      <var n="in"/>
      <rn v="%(scale)s"/>
    </func>
    <mkent c="%(class)s" s="%(sclass)s" i="%(out_id)s"/>
  </func>
</tran>"""

MK_ERC = """<pat>
  <list v="num">
      <n r="y"/>
  </list>
  <list v="e">
    <ent c="1" />
  </list>
</pat>
<tran>
  <func name="mkERC">
    <var n="e"/>
    <func name="mkRange">
      <var n="num"/>
    </func>
  </func>
</tran>
"""

STMT = """\n<stmt tags="" n="%(counter)s" name="%(name)s" >"""

class Unit:
	def __init__(self, s, cls, scls):
		s = s.split("|")
		self.names = s[:-1]
		self.scale = float(s[-1].strip())
		self.cls = cls
		self.scls = scls
        def __repr__(self):
		result = str(self.cls) + ":" + str(self.scls) + " | "
		for s in self.names:
			result += s.strip() + " "
		result+= " | " + self.atox + " " + self.xtoa + "\n"
		return result
	def mkEnt(self, counter):
		result=""
		result+= '\n<stmt tags="" n="' + str(counter) + '" name="mkent('+self.names[0] + ')">\n<pat><any>'
		for s in self.names:
			if s.strip().count(" ") > 0:
				result += "<list>"
				for parts in s.split(" "):
					result += '<t>' + parts + '</t>'
				result += "</list>"
			else:
				result += '<t>' + s + '</t>'
		result+='</any></pat><tran>'
		result+='<mkent c="' + str(self.cls) + '" s="' + str(self.scls) + '" i="' + self.names[0] + '"/></tran>\n</stmt>\n'
		counter[0]+=1
		return result
	names=[]
	scale = long(1.0)
	cls=1
	scls=1

class Unitset:
	def __init__(self, filename):
		f = open(filename)
		lines = f.readlines()
		self.cls = int(lines[0].split(' ')[0])
		self.scls = int(lines[0].split(' ')[1])
		self.units = []
		for l in lines[1:]:
			if (l[0] != '#'):
				print "\t* adding: " + l[:-1]
				u = Unit(l, self.cls, self.scls)
				self.units.append(u)
	def xml(self, counter):
		result = ""
		result += STMT % {'counter': counter[0], 'name':self.units[0].names[0] + " into " + self.units[0].names[0]}
		result+= BASE_TO_BASE % {'class': self.units[0].cls,'sclass':self.units[0].scls,'base':self.units[0].names[0]}
		counter[0]+=1
		result += self.units[0].mkEnt(counter)
		counter[0] +=1
		for u in self.units[1:]:
			result += u.mkEnt(counter)
			counter[0]+=1
			params = {'class': u.cls,'sclass':u.scls,'in_id':u.names[0],'base':self.units[0].names[0],'out_id':u.names[0],'scale':u.scale};
			result += STMT % {'counter': counter[0], 'name':u.names[0] + " to " + str(self.units[0].names[0])} + TO_BASE % params + '</stmt>'
			counter[0]+=1
			result += STMT % {'counter': counter[0], 'name':self.units[0].names[0] +" to " + u.names[0]} + FROM_BASE % params + '</stmt>'
			counter[0]+=1
		return result
	cls=1
	scls=1
	#units=[]

class Ruleset:
	def __init__(self, directory, counter_start = 0):
		self.counter = counter_start
		listing = os.listdir(directory)
		for infile in listing:
			if (not infile.endswith("~")):
				print directory +"/"+infile
				u = Unitset(directory + "/" +infile)
				self.unitsets.append(u)
	def produce(self):
		result = ""
		result += HEADER + MACRO 
		result  += STMT % {'counter': 1, 'name':"mkerc"} + MK_ERC + '</stmt>'
		for us in self.unitsets:
			result += '\n' + us.xml(self.count)
		result+= '</stmset>'
		print "\n" + str(self.count[0] + 1) + " rules was generated!"
		return result
	count = [2]  #wrapper for passing by reference
	unitsets= []


r = Ruleset('/home/nix/barzer-git/btesting/python-testing/aut/data')
f = file('/home/nix/barzer-git/btesting/python-testing/aut/result.xml','w')
f.write(r.produce())