"""Talking to barzer's TCP server."""

import socket, sys
from collections import defaultdict

from os import path

from lxml import etree
from lxml.etree import tostring #@UnresolvedImport
from lxml.builder import E

DEFAULT_HOST = 'localhost'
DEFAULT_PORT = 5666
END_TOKEN = "\r\n.\r\n"

MAX_INT = '2147483647'

ALPHA = False
ALPHA_PORT = 0

class Socket(socket.socket):
    """Convenience socket wrapper."""

    def __init__(self, address):
        self.address = address
        super(self.__class__,self).__init__(socket.AF_INET, socket.SOCK_STREAM)

    def __enter__(self):
        self.connect(self.address);
        return self

    def __exit__(self, typ, val, traceback):
        self.close()

    def readall(self):
        r = ""
        while 1:
            s = self.recv(1024)
            if not s: break
            r += s
        return r
    
    """
    def sendall(self, s, *args):
        sys.stderr.write(s)
        return socket.socket.sendall(self, s, *args)
    """
    def send_xml(self, xml):
        s = tostring(xml, encoding='UTF-8') #@UndefinedVariable
        #print >>sys.stderr, s
        self.sendall(s)
        self.sendall(END_TOKEN)
        return self.readall()

class InvalidTopicException(Exception):
    pass

class BarzerClient:
    "Client for barzer tcp server."

    def __init__(self, host=DEFAULT_HOST, port=DEFAULT_PORT):
        self.address = (host, port)

    def get_socket(self):
        return Socket(self.address)

    def query(self, data, user_id=None, topic_info=None, **kwargs):
        "Returns query result as returned by barzer."
        try:
            with self.get_socket() as sock:
                #print 'BarzerClient::query self.get_socket'
                attrs = kwargs
                if user_id: attrs['u'] = str(user_id)
                node = E.query(data)
                if topic_info:
                    try:
                        q_node = node
                        node = E.qblock(attrs)
                        for c, s, i in topic_info:
                            node.append(E.topic(c=str(c), s=str(s), i=i))
                        node.append(q_node)
                    except ValueError:
                        raise InvalidTopicException()
                else:
                    node.attrib.update(attrs)
                return sock.send_xml(node)
        except socket.error:
            print 'Connectivity error for :', self.address
            return '<barz error="Connectivity error"/>'

 
    def autoc(self, data, user_id, **kwargs):
        with self.get_socket() as s:
            args = {'u': str(user_id)}
            args.update(kwargs)
            return s.send_xml(E.autoc(data, args))

    def add_user(self, user_id):
        with self.get_socket() as s:
            c = E.cmd({'name': 'add'}, E.user({'id': str(user_id)}))
            return s.send_xml(c)

    def clear_user(self, user_id):
        with self.get_socket() as s:
            s.sendall('!!CLEAR_USER:')
            s.sendall(str(user_id))
            s.sendall(END_TOKEN)
            return s.readall()
    
    def load_usercfg(self, path):
        with self.get_socket() as s:
            s.sendall('!!LOAD_USRCFG:')
            s.sendall(path)
            s.sendall(END_TOKEN)
            return s.readall()

    def add_rulefile(self, file_name, trie_class=None, trie_name=None):
        with self.get_socket() as s:
            c = E.cmd({'name': 'add'})
            f = etree.SubElement(c, "rulefile") #@UndefinedVariable
            f.text = file_name
            if trie_class and trie_name:
                f.attrib.update({'class':trie_class, 'name':trie_name})
            return s.send_xml(c)

    def add_trie(self, user_id, trie_class, trie_name):
        with self.get_socket() as s:
            t = E.trie({'u': str(user_id), 'class': trie_class, 'name': trie_name})
            c = E.cmd({'name':'add'}, t)
            return s.send_xml(c)

    def clear_trie(self, trie_class, trie_name):
        with self.get_socket() as s:
            s.sendall("!!CLEAR_TRIE:")
            s.sendall("{0}|{1}".format(trie_class, trie_name))
            s.sendall(END_TOKEN)
            return s.readall()
    
    def load_config(self, path):
        with self.get_socket() as s:
            s.sendall('!!LOAD_CONFIG:')
            s.sendall(path)
            s.sendall(END_TOKEN)
            return s.readall()
    
    def add_stmset(self, user_id, trie_class, trie_name, text):
        with self.get_socket() as s:
            s.sendall('!!ADD_STMSET:')
            s.sendall('{0}|{1}|{2}|'.format(user_id, trie_class, trie_name))
            s.sendall(text)
            s.sendall(END_TOKEN)
            return s.readall()
    
    def emit(self, s):
        """emits a ruleset"""
        #print "\n\n\n sending to port: %s" % str(self.address)
        with self.get_socket() as sock:
            sock.sendall("!!EMIT:")
            sock.sendall(s)
            sock.sendall(END_TOKEN)
            return sock.readall()

            
