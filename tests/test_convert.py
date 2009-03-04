# -*- coding; utf-8 -*-
import mobilejpcodecs

import re
import os
yamlfile = lambda x: file(os.path.join('dat', x))

try:
    import syck
    yaml_load = lambda filename: syck.load(yamlfile(filename).read())
except ImportError:
    import yaml
    yaml_load = lambda filename: yaml.load(yamlfile(filename))

UNICODE_RE = re.compile(r'([0-9a-fA-F]{4})')

def unicode_and_type(params):
    if params['type'] == 'pictogram':
        return UNICODE_RE.sub(lambda m: unichr(int(m.group(1), 16)), params['unicode']), 'pictogram'
    else:
        return params['unicode'].encode('utf8'), 'type'

def test_convert():
    def func_docomo(docomo, kddi, softbank):
        d_value, d_type = unicode_and_type(docomo)
        k_value, k_type = unicode_and_type(kddi)
        s_value, s_type = unicode_and_type(softbank)

        if k_type == 'pictogram':
            a = d_value.encode('x_sjis_kddi')
            b = k_value.encode('x_sjis_kddi')
            assert a == b, (d_value, k_value, a, b)

        if s_type == 'pictogram':
            a = d_value.encode('x_sjis_softbank')
            b = s_value.encode('x_sjis_softbank')
            assert a == b, (d_value, s_value, a, b)

    def func_kddi(docomo, kddi, softbank):
        d_value, d_type = unicode_and_type(docomo)
        k_value, k_type = unicode_and_type(kddi)
        s_value, s_type = unicode_and_type(softbank)

        if d_type == 'pictogram':
            b = k_value.encode('x_sjis_docomo')
            a = d_value.encode('x_sjis_docomo')
            assert a == b, (d_value, k_value, a, b)

        if s_type == 'pictogram':
            a = k_value.encode('x_sjis_softbank')
            b = s_value.encode('x_sjis_softbank')
            assert a == b, (k_value, s_value, a, b)

    def func_softbank(docomo, kddi, softbank):
        d_value, d_type = unicode_and_type(docomo)
        k_value, k_type = unicode_and_type(kddi)
        s_value, s_type = unicode_and_type(softbank)

        if d_type == 'pictogram':
            b = s_value.encode('x_sjis_docomo')
            a = d_value.encode('x_sjis_docomo')
            assert a == b, (d_value, s_value, a, b)

        if k_type == 'pictogram':
            b = s_value.encode('x_sjis_kddi')
            a = k_value.encode('x_sjis_kddi')
            assert a == b, (k_value, s_value, a, b)

    for carrier, table in yaml_load('convert-map-utf8.yaml').items():
        for code, rows in table.items():
            params = {}
            params[carrier] = { 'type'   : 'pictogram',
                                'unicode': code,
                                }
            params.update(rows)

            if carrier == 'docomo':
                yield func_docomo, params['docomo'], params['kddi'], params['softbank']
            elif carrier == 'kddi':
                yield func_kddi, params['docomo'], params['kddi'], params['softbank']
            elif carrier == 'softbank':
                yield func_softbank, params['docomo'], params['kddi'], params['softbank']
