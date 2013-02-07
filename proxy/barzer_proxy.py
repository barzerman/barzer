import uwsgi #@UnresolvedImport
from uwsgi import cache_set, cache_get, cache_exists, cache_update #@UnresolvedImport
from urlparse import unquote
from xml.sax.saxutils import escape, quoteattr
from os.path import exists

#copypasted from uwsgidecorators
def get_free_signal():
    for signum in xrange(0, 256):
        if not uwsgi.signal_registered(signum):
            return signum
    raise Exception("No free uwsgi signal available")

BARZER_SOCKET = uwsgi.opt.get('barzer-socket') or "127.0.0.1:5767"
KEY_FILE = uwsgi.opt.get('key_file')
def read_keys(num):
    print "reading keys from", KEY_FILE
    if not exists(KEY_FILE):
        print KEY_FILE, "not found"
        return
    with open(KEY_FILE) as f:
        uwsgi.cache_clear()
        for l in f:
            l = l.strip()
            if not l: continue
            k, v = l.split('|', 1)
            if ',' in v:
                vs = v.split(',')
                cache_set(k, '^'+str(len(vs)))
                for i, c in enumerate(vs):
                    cache_set(k+str(i), c)
            else:
                cache_set(k, v)

def set_mon():
    num = get_free_signal()
    uwsgi.register_signal(num, 'mule1', read_keys)
    uwsgi.add_file_monitor(num, KEY_FILE)
    read_keys(num)

if KEY_FILE is not None:
    set_mon()

def resolve_key(key, uids):
    val = cache_get(key)
    if not val: return
    elif val[0] == '^':
        for i in xrange(int(val[1:])):
            v = cache_get(key + str(i))
            if v:
                uids.append(v)
            else:
                print >>sys.stderr, "Error retrieving key {}{}".format(key, i)
    else:
        uids.append(val)
    
## socket operations
class QState(object):
    status = 0
    result = ''
    def __init__(self, uid):
        self.uid = uid

def register_all(fdmap): 
    """ Apparently you need to reregister all waits after every resume
        to be compatible with wsgi *shrug*"""
    for fd, st in fdmap.iteritems():
        if st.status:
            uwsgi.wait_fd_read(fd, 10)
        else:
            uwsgi.wait_fd_write(fd, 10)
    return ''

def close_all(fdmap):
    for k in fdmap:
        uwsgi.close(k)    

## content

def reconcile(responses, r_type):
    def process_json():
        return '[{}]'.format(',\n'.join(responses))
    def process_xml():
        return '<barz-list>{}</barz-list>'.format('\n'.join(responses))
    return locals()['process_'+r_type]()

def error_xml(s):
    #return '<error>' + s + '</error>'
    return "<error>{}</error>".format(s)
def error_json(s):
    return '{{ "error": "{}" }}'.format(s) 
def error_html(s):
    return '<h3 style="color:red">{}</h3>'.format(s)

def application(env, start_response):
    typ = env['PATH_INFO'].rsplit('/', 1)[-1] #@ReservedAssignment
    r_type = None 
    if typ in ('json', 'sjson'):
        ct, error, add = 'application/json', error_json, ' ret="{}"'.format(typ)
        r_type = 'json' 
    else:
        ct, error, add = 'text/xml', error_xml, ''
        r_type = 'xml'
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

    cnt = len(uids)
    while cnt:
        yield register_all(fdmap)
        fd = env['uwsgi.ready_fd']
        if not uwsgi.is_connected(fd):
            close_all(fdmap)
            yield error("Unable to connect")
            return        
        elif env['x-wsgiorg.fdevent.timeout']:
            close_all(fdmap)
            yield error("Connection timed out")
            return        

        st = fdmap[fd]
        if st.status == 0:
            qry = '<query u="{}"{}>{}</query>\r\n.\r\n'.format(st.uid, add, query)
            uwsgi.send(fd, qry)
            st.status = 1
        elif st.status == 1:
            data = uwsgi.recv(fd)
            if data:
                st.result += data
            else:
                responses.append(st.result)
                del fdmap[fd]
                uwsgi.close(fd)
                cnt -= 1
    if len(responses) > 1:
        yield reconcile(responses, r_type)
    else:
        yield responses[0]
                

