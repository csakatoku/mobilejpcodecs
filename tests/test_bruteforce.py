# -*- coding: utf-8 -*-
import os
yamlfile = lambda x: file(os.path.join('dat', x))

try:
    import syck
    yaml_load = lambda filename: syck.load(yamlfile(filename).read())
except ImportError:
    import yaml
    yaml_load = lambda filename: yaml.load(yamlfile(filename))

import binascii
import mobilejpcodecs

def _test_func(encoding, input, expected):
    res = input.decode(encoding)
    assert res == expected, res
    assert input == res.encode(encoding)

def test_docomo_bruteforce():
    for x in yaml_load('docomo-table.yaml'):
        input     = binascii.unhexlify(x['sjis'])
        expected  = unichr(int(x['unicode'], 16))
        yield _test_func, 'x_sjis_docomo', input, expected

def test_softbank_bruteforce():
    for x in yaml_load('softbank-table.yaml'):
        sjis_auto = x['sjis_auto']
        if sjis_auto:
            input     = binascii.unhexlify(sjis_auto)
            expected  = unichr(int(x['unicode'], 16))
            yield _test_func, 'x_sjis_softbank', input, expected

def test_softbank_webcode_bruteforce():
    def func(encoding, input, expected, sjis_raw):
        res = input.decode(encoding)
        assert res == expected, res
        assert res.encode(encoding) == sjis_raw, res

    for x in yaml_load('softbank-table.yaml'):
        sjis = x['sjis']
        sjis_auto = x['sjis_auto']
        if sjis and sjis_auto:
            input     = binascii.unhexlify(sjis)
            sjis_raw  = binascii.unhexlify(sjis_auto)
            expected  = unichr(int(x['unicode'], 16))
            yield func, 'x_sjis_softbank', input, expected, sjis_raw

def test_kddi_bruteforce():
    data = yaml_load('kddi-table.yaml')
    for x in data:
        input     = binascii.unhexlify(x['sjis'])
        expected  = unichr(int(x['unicode'], 16))
        yield _test_func, 'x_sjis_kddi_raw', input, expected

    for x in data:
        input     = binascii.unhexlify(x['sjis'])
        expected  = unichr(int(x['unicode_auto'], 16))
        yield _test_func, 'x_sjis_kddi', input, expected

