import uwsgi, traceback, sys #@UnresolvedImport
from uwsgi import cache_set, cache_get, cache_exists, cache_update #@UnresolvedImport
from urlparse import unquote
from xml.sax.saxutils import escape, quoteattr
from os.path import exists
from render import QState, reconcile, error_xml, error_json

#copypasted from uwsgidecorators
def get_free_signal():
    for signum in xrange(0, 256):
        if not uwsgi.signal_registered(signum):
            return signum
    raise Exception("No free uwsgi signal available")

BARZER_SOCKET = uwsgi.opt.get('barzer-socket') or "127.0.0.1:5767"
KEY_FILE = uwsgi.opt.get('key_file')
key_ix = {}

def read_keys(num):
    global key_ix
    if not exists(KEY_FILE):
        print KEY_FILE, "not found"
        return
    with open(KEY_FILE) as f:
        print "reading keys from", KEY_FILE
        try:
            new_ix = {}
            for l in f:
                l = l.strip()
                if not l: continue
                k, v = l.split('|', 1)
                lst = new_ix[k] = []
                if v[0] == '|':
                    for sub in v[1:].split('|'):
                        i, s, a = sub.split(',', 2)
                        lst.append((i, int(s), a))
                else:
                    for i in v.split(','):
                        lst.append((i, 0, ''))
            key_ix = new_ix
        except Exception as e:
            traceback.print_exc(file=sys.stderr)
            print e

def set_mon():
    num = get_free_signal()
    uwsgi.register_signal(num, 'workers', read_keys)
    uwsgi.add_file_monitor(num, KEY_FILE)
    read_keys(num)

if KEY_FILE is not None:
    set_mon()

def resolve_key(key, uids):
    for val in key_ix.get(key, []):
        uids.append(val[0])


## socket operations

def register_all(fdmap): 
    """ Apparently you need to reregister all waits after every resume
        to be compatible with wsgi *shrug*"""
    for fd, st in fdmap.iteritems():
        if st.status:
            uwsgi.wait_fd_read(fd, 10)
        else:
            uwsgi.wait_fd_write(fd, 10)
    return ''


def application(env, start_response):
    typ = env['PATH_INFO'].rsplit('/', 1)[-1] #@ReservedAssignment
    if typ in ('json', 'sjson'):
        add = ' ret="{}"'.format(typ)
        ct, error, r_type = 'application/json', error_json, 'json'
    else:
        ct, error, r_type, add = 'text/xml', error_xml, 'xml', ''
    start_response('200 OK', [('Content-Type', ct + '; charset=utf-8')])

    query = ''
    uids = []
    ver = 1.0
    qs = env['QUERY_STRING']
    for kv in (s2 for s1 in qs.split('&') for s2 in s1.split(';')):
        parts = kv.split('=', 1)
        if len(parts) != 2:
            continue
        k, v = parts
        if k == 'query':
            query = escape(unquote(v.replace('+', ' ')))
        elif k == 'key':
            resolve_key(v, uids)
        elif k == 'ver':
            try:
                ver = float(v)
            except ValueError:
                yield error("Invalid `ver' parameter")
                return
        elif k == 'now':
            add += ' now='+ quoteattr(unquote(v.replace('+', ' ')))
            
    if not uids:
        yield error("Invalid key")
        return
    elif not query:
        yield error("Invalid query")
        return
    elif int(ver) != 1:
        yield error("Version not supported")
        return

    fdmap = {}
    responses = []

    for i in uids:
        fd = uwsgi.async_connect('127.0.0.1:5767')
        fdmap[fd] = QState(i)

    while fdmap:
        yield register_all(fdmap)
        fd = env['uwsgi.ready_fd']
        st = fdmap[fd]
        if not uwsgi.is_connected(fd):
            st.status = -1
            st.result = error("Unable to connect") #fallthrough
        elif env['x-wsgiorg.fdevent.timeout']:
            st.status = -2
            st.result = error("Connection timed out") #fallthrough
        elif st.status == 0:
            qry = '<query u="{}"{}>{}</query>\r\n.\r\n'.format(st.uid, add, query)
            uwsgi.send(fd, qry)
            st.status = 1
            continue
        elif st.status == 1:
            data = uwsgi.recv(fd)
            if data:
                st.result += data
                continue
            #fallthrough
        
        uwsgi.close(fd)        
        del fdmap[fd]
        responses.append(st)


    if len(responses) > 1:
        yield reconcile(responses, r_type)
    else:
        yield responses[0].result
                

