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
        for l in f:
            l = l.strip()
            if not l: continue
            k, v = l.split('|', 1)
            if cache_exists(k):
                cache_update(k, v)
            else:
                cache_set(k, v)

def set_mon():
    num = get_free_signal()
    uwsgi.register_signal(num, 'mule1', read_keys)
    uwsgi.add_file_monitor(num, KEY_FILE)    
    read_keys(num)

if KEY_FILE is not None:
    set_mon()
    

def error_xml(s):
    #return '<error>' + s + '</error>'
    return "<error>{}</error>".format(s)
def error_json(s):
    return '{{ "error": "{}" }}'.format(s) 
def error_html(s):
    return '<h3 style="color:red">{}</h3>'.format(s)

def application(env, start_response):
    typ = env['PATH_INFO'].rsplit('/', 1)[-1] #@ReservedAssignment
    if typ in ('json', 'sjson'):
        ct, error, add = 'application/json', error_json, ' ret="{}"'.format(typ)
    else:
        ct, error, add = 'text/xml', error_xml, ''
    start_response('200 OK', [('Content-Type', ct + '; charset=utf-8')])

    query = ''
    uid = 0
    ver = 1.0
    qs = env['QUERY_STRING']
    user_set = False
    for kv in (s2 for s1 in qs.split('&') for s2 in s1.split(';')):
        parts = kv.split('=', 1)
        if len(parts) != 2:
            continue
        k, v = parts
        if k == 'query':
            query = escape(unquote(v.replace('+', ' ')))
        elif k == 'key':
            uid = cache_get(v)
            user_set = bool(uid)
        elif k == 'ver':
            try:
                ver = float(v)
            except ValueError:
                yield error("Invalid `ver' parameter")
                return
        elif k in {'now', 'beni', 'zurch', 'flag', 'route', 'extra', 'u', 'uname'}:
            if k in ('u', 'uname'):
                user_set = True
            add += ' {}={}'.format(k, quoteattr(unquote(v.replace('+', ' '))))
            
    if not user_set:
        yield error("Invalid user")
        return
    elif not query:
        yield error("Invalid query")
        return
    elif int(ver) != 1:
        yield error("Version not supported")
        return

    fd = uwsgi.async_connect(BARZER_SOCKET)
    yield uwsgi.wait_fd_write(fd, 5)
    
    if env['x-wsgiorg.fdevent.timeout']:
        yield error("Connection timed out")
        uwsgi.close(fd)
        return
    elif not uwsgi.is_connected(fd):
        yield error("Unable to connect")
        uwsgi.close(fd)
        return
    qry = '<query u="{}"{}>{}</query>\r\n.\r\n'.format(uid, add, query)
    #print qry
    uwsgi.send(fd, qry)
    while True:
        yield uwsgi.wait_fd_read(fd, 10)
        if env['x-wsgiorg.fdevent.timeout']:
            yield error("Connection timed out")
            uwsgi.close(fd)
            return

        data = uwsgi.recv(fd)
        if not data:
            break
        yield data
    uwsgi.close(fd)

