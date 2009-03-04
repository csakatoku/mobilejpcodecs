#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
import pprint
import optparse
import binascii

import syck
from jinja2 import Template
from mobilejpcodecs import emojitable

TMPL = Template("""static const struct emoji_t __{{ carrier }}_decmap[] = {
{% for s, l, code in rows -%}
  {% if s %}emoji_string("{{ s }}"){% else %}{ NULL, 0 }{% endif %}{% if not loop.last %},{% endif %} /* {{ code }} */
{% endfor -%}
};""")

def make_unicode_to_sjis_dict(filename, unicode_key, sjis_key):
    data = syck.load(file(filename).read())
    res = {}
    for x in data:
        uni  = unichr(int(x[unicode_key], 16))
        sjis = x[sjis_key]
        if not sjis:
            # SKYWEB(E255)
            # SKYWALKER(E256)
            # SKYMELODY(E257)
            res[uni] = ''
        else:
            res[uni] = binascii.unhexlify(sjis)
    return res


def make_header(carrier, data, filename):
    rows = []
    for code in xrange(0xe001, 0xf0fc + 1):
        value = data.get(unichr(code))
        if value:
            buf = []
            for x in value:
                buf.append(r'\x%x' % ord(x))
            s = ''.join(buf)
            rows.append((s, len(value), '%x' % code))
        else:
            rows.append((None, 0, '%x' % code))

    fp = file(filename, 'wb')
    fp.write(TMPL.render(dict(carrier=carrier,
                              rows=rows)))
    fp.write('\n')
    fp.close()


def make_header_docomo(input_dir, output_dir):
    filename = os.path.join(input_dir, 'docomo-table.yaml')
    data = make_unicode_to_sjis_dict(filename, 'unicode', 'sjis')

    res = {}
    for uni, row in emojitable.TABLE.items():
        translated  = row['d']['unicode']
        is_pictgram = row['d']['type'] == 'pictogram'

        if is_pictgram:
            buf = []
            for x in translated:
                try:
                    buf.append(data[x])
                except KeyError:
                    print repr(uni), repr(translated), uni == translated, is_pictgram
                    pass
            sjis = ''.join(buf)
        else:
            sjis = translated.encode('cp932')

        res[uni] = sjis

    make_header('docomo', res, os.path.join(output_dir, 'mappings_docomo.h'))


def make_header_kddi(input_dir, output_dir):
    filename = os.path.join(input_dir, 'kddi-table.yaml')
    data = make_unicode_to_sjis_dict(filename, 'unicode_auto', 'sjis')

    res = {}
    for uni, row in emojitable.TABLE.items():
        translated  = row['k']['unicode']
        is_pictgram = row['k']['type'] == 'pictogram'

        if is_pictgram:
            buf = []
            for x in translated:
                try:
                    buf.append(data[x])
                except KeyError:
                    pass
                    print repr(uni), repr(translated), uni == translated, is_pictgram
            sjis = ''.join(buf)
        else:
            sjis = translated.encode('cp932')

        res[uni] = sjis

    make_header('kddi', res, os.path.join(output_dir, 'mappings_kddi.h'))


def make_header_softbank(input_dir, output_dir):
    filename = os.path.join(input_dir, 'softbank-table.yaml')
    data = make_unicode_to_sjis_dict(filename, 'unicode', 'sjis_auto')

    res = {}
    for uni, row in emojitable.TABLE.items():
        translated  = row['s']['unicode']
        is_pictgram = row['s']['type'] == 'pictogram'

        if is_pictgram:
            buf = []
            for x in translated:
                try:
                    buf.append(data[x])
                except KeyError:
                    print repr(uni), repr(translated), uni == translated, is_pictgram
                    pass
            sjis = ''.join(buf)
        else:
            sjis = translated.encode('cp932')

        res[uni] = sjis

    make_header('softbank', res, os.path.join(output_dir, 'mappings_softbank.h'))


def main():
    parser = optparse.OptionParser()
    parser.add_option("-i", "--input", dest="input_dir",
                      help="path to YAML directory")
    parser.add_option("-o", "--output", dest="output_dir",
                      help="path to output directory")
    opts, args = parser.parse_args()

    if opts.input_dir is None or opts.output_dir is None:
        parser.print_help()
        sys.exit(1)

    make_header_docomo(opts.input_dir, opts.output_dir)
    make_header_kddi(opts.input_dir, opts.output_dir)
    make_header_softbank(opts.input_dir, opts.output_dir)

if __name__ == '__main__':
    main()
