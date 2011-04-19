#!/usr/bin/python

import sys
import random
import string

pat_list = []

def gen_random_str():
    len = random.randint(1, 10)
    return ''.join(random.choice(string.ascii_letters) for x in range(len))

def gen_tok(n):
    return "<t>%s</t>" % gen_random_str()

def gen_num(n):
    type = random.randint(0, 2)
    if type == 0:
        return '<n />'
    elif type == 1:
        ns = [random.random(),random.random()]
        ns.sort()
        return '<n h="%f" l="%f" r="true" />' % tuple(ns)
    else:
        ns = [random.randint(0, 32535), random.randint(0, 32535)]
        ns.sort()
        return '<n h="%d" l="%d" />' % tuple(ns)

def gen_punct(n):
    return "<p>%s</p>" % random.choice("!\"#$%\\'()*+,-./:;=?@[\\]^_`{|}~")

def gen_wcls(n):
    return "<wcls>%s</wcls>" % random.choice(["verb","noun","adjective"])


def gen_seq(tag, num):
    if num > 3: return ""
    ret = "\n<%s>\n" % tag
    for i in range(random.randint(2, 6)):
        ret += (random.choice(pat_list)(num+1))
    ret += "\n</%s>\n" % tag
    return ret


def gen_list(num = 0):
    return gen_seq("list", num)
  
def gen_opt(num = 0):
    return gen_seq("opt", num)

def gen_perm(num = 0):
    return gen_seq("perm", num)
        
def gen_any(num = 0):
    return gen_seq("any", num)


pat_list += [gen_tok, gen_num, gen_punct, gen_list, gen_any, gen_perm, gen_opt]

def gen_pat():
    print "<pat>",
    for i in range(random.randint(2, 6)):
        print (random.choice(pat_list)(0)),
    print "</pat>"


def gen_ltrl():
    return "<ltrl>%s</ltrl>" % gen_random_str()
def gen_rn():
    return '<rn v="%d" />' % random.randint(0, 32535)
def gen_var():
    return "<var>%s</var>" % gen_random_str()


tran_list = [gen_ltrl, gen_rn, gen_var]
def gen_tran():
    print "<tran>",
    for i in range(5):
        print (random.choice(tran_list)()),
    print "</tran>"


def main():
    print '<?xml version="1.0" encoding="UTF-8"?>'
    print '<stmset xmlns:xsi="http://www.w3.org/2000/10/XMLSchema-instance" xmlns="http://www.barzer.net/barzel/0.1">'
    no = int(sys.argv[1])
    for i in xrange(1, no):
        print "<stmt>"
        gen_pat()
        gen_tran()
        print "</stmt>"

    print "</stmset>"


if __name__ == "__main__":
    main()

