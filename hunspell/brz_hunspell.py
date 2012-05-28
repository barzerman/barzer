#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import hunspell
import re
import locale
import sys
import getopt

class brzHunspell:
	def __init__(self, dic, aff):
		self.h = hunspell.HunSpell(dic, aff)
		self.regexp = re.compile(r"[a-zA-ZÀ-ÿ]+|['\"$\%&()\*\+=\-:;,\.\\/<>!?@\[\]^_{|}~]|[0-9]+",flags=re.UNICODE | re.IGNORECASE)

	def tokenize(self,query):
		return self.regexp.findall(query)

	def spellcorrect(self,wordlist):
		spellcorrected = []
		for s in wordlist:
			if self.h.spell(s):
				spellcorrected.append(s)
				continue
			if len(s) > 3:
				suggested = self.h.suggest(s)
				if suggested[0].find(" ") >=0 :
					spellcorrected.extend(suggested[0].split(" "))
				else:
					spellcorrected.append( suggested[0] if len(suggested) > 1 else s )
			else:
				spellcorrected.append(s)
		return spellcorrected

	def stem(self,wordlist):
		stemmed = []
		for s in wordlist:
			if len(s) > 4:
				r = self.h.stem(s)
				stemmed.append(r[0] if len(r) > 1 else s)
			else:
				stemmed.append(s)
		return stemmed

	def barzIT(self,strlist, user_num):
		n = 0
		r = ['<barz u="', str(user_num), '">']
		for s in strlist:
			r.extend(['<bead n="', str(n), '">\n'])
			if s.isdigit():
				r.extend(['\t<num t="int">', s, '</num>\n'])
			else:
				r.extend(['\t<token>', s, '</token>\n'])
			r.extend(['\t<srctok>', s, '</srctok>\n'])
			r.append('</bead>\n')
			n+=1
		r.append('</barz>')
		return ''.join(r)

	def processIT(self, str, userID):
		return self.barzIT(self.stem(self.spellcorrect(self.tokenize(str))), userID)



