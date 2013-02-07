class QState(object):
    status = 0
    result = ''
    def __init__(self, uid):
        self.uid = uid
        
        
## content

def reconcile(responses, r_type):
    def process_json(ts):
        return '[{}]'.format(',\n'.join(ts))
    def process_xml(ts):
        return '<barz-list>{}</barz-list>'.format('\n'.join(ts))
    return locals()['process_'+r_type](r.result for r in responses)

def error_xml(s):
    return "<error>{}</error>".format(s)
def error_json(s):
    return '{{ "error": "{}" }}'.format(s) 
def error_html(s):
    return '<h3 style="color:red">{}</h3>'.format(s)