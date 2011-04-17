#!/usr/bin/python

import sys
import random
import string

def gen_random_str():
    return ''.join(random.choice(string.ascii_letters) for x in range(5))

def gen_tok():
    return "<t>%s</t>" % gen_random_str()

def gen_num():
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

def gen_punct():
    return "<p>%s</p>" % random.choice("!\"#$%\\'()*+,-./:;=?@[\\]^_`{|}~")

def gen_wcls():
    return "<wcls>%s</wcls>" % random.choice(["verb","noun","adjective"])

pat_list = [gen_tok, gen_num, gen_punct, gen_wcls]

def gen_pat():
    print "<pat>",
    for i in range(5):
        print (random.choice(pat_list)()),
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
    print "<stmset>"

    no = int(sys.argv[1])
    for i in xrange(1, no):
        print "<stmt>"
        gen_pat()
        gen_tran()
        print "</stmt>"

    print "</stmset>"


if __name__ == "__main__":
    main()

