#!/usr/bin/env python

import sys, subprocess, os, time, signal, atexit
from os import path
import xml.etree.ElementTree as ET


children = []
ROOT=False

def kill_children():
    print "killing children"
    for c in children:
        os.kill(c, signal.SIGTERM)


def sigint_handler(signal, frame):
    if not ROOT:
        return
    print "ctrl-c recieved. exiting"
    sys.exit(0)

def run(cmd, out):
    with open(out, 'wb') as fh:
        print "running", ' '.join(cmd)
        subprocess.call(cmd, stderr=fh, stdout=fh)


def runbarzer(barzer_dir, cfg, instId, port):
    barzer_path = path.join(barzer_dir, "barzer.exe")
    cmd = [barzer_path, 'server', port, '-inst', instId, '-cfg', cfg,
            '-home', barzer_dir]
    out = "barzer.{}.out".format(instId)
    run(cmd, out)


def runhub(hub_dir, cfg):
    hub_path = path.join(hub_dir, 'barzer_hub')
    cmd = [hub_path, '-v', '-cfg', cfg]
    subprocess.call(cmd)
    #run(cmd, "hub.out")
    

def main():
    if len(sys.argv) < 2:
        print >>sys.stderr, sys.argv[0], "<barzer config>"
        sys.exit(1)
    scriptname, cfg = sys.argv[:2]
    hub_dir = path.dirname(scriptname)
    barzer_dir = path.abspath(path.join(hub_dir, os.pardir))

    signal.signal(signal.SIGINT, sigint_handler)

    doc = ET.parse(cfg)
    for c in doc.findall('instances/instance'):
        pid = os.fork()
        if not pid:
            return runbarzer(barzer_dir, cfg,  c.get('id'), c.get('port'))
        children.append(pid)
    global ROOT
    ROOT = True
    atexit.register(kill_children)
    signal.signal(signal.SIGTERM, sigint_handler)
    runhub(hub_dir, cfg)
    
    
if __name__ == '__main__':
    sys.exit(main())