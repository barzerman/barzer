#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import hunspell
import re
import locale
import sys
import getopt

def tokenize(query):
	return re.findall(r"[a-zA-ZÀ-ÿ]+|['.\-,!?;]|[0-9]+",query.encode("utf-8"),flags=re.UNICODE)

def spellcorrect(wordlist, hunspellobj):
	spellcorrected = []
	for s in wordlist:
		if hunspellobj.spell(s):
			spellcorrected.append(s)
			continue

		if len(s) > 4:
			suggested = hunspellobj.suggest(s)
			if suggested[0].find(" ") >=0 :
				spellcorrected.extend(suggested[0].split(" "))
			else:
				spellcorrected.append( suggested[0] if len(suggested) > 1 else s )
		else:
			spellcorrected.append(s)
	return spellcorrected


def stem(wordlist, hunspellobj):
	stemmed = []
	for s in wordlist:
		if len(s) > 4:
			r = hunspellobj.stem(s)
			stemmed.append(r[0] if len(r) > 1 else s)
		else:
			stemmed.append(s)
	return stemmed
def barzIT(strlist, user_num):
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



