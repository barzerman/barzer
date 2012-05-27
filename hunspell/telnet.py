#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

import   socket, sys, string

def send(s):
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.connect(("localhost", 5666))
	sock.send(s)
	sock.send("\r\n.\r\n")
	data = sock.recv(1024)
	string = ""
	while len(data):
		string = string + data
		data = sock.recv(1024)
	sock.close()
	return string
