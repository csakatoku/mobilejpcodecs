# -*- coding: utf-8 -*-
import re
import codecs
from math import ceil

import _codecs_mobilejp

codec = codecs.lookup('iso2022_jp_2')
mobilejp_codec = _codecs_mobilejp.getcodec('x_sjis_kddi')

ISO2022JP_SPLIT_RE = re.compile(r'(\x1b\x24\x42.+?\x1b\x28\x42)')

def _each_two_byte(value):
    for i in xrange(0, len(value), 2):
        part = value[i:i+2]
        if len(part) == 2:
            yield part


def _jis2sjis_one(x):
    return (_xy(x) << 8) + _zu(x)

def _xy(x):
    pq  = x >> 8
    t   = int(ceil(pq / 2.0)) + 0x70
    if t <= 0x9f:
        ans = t
    else:
        ans = t + 0x40

    if 0xed == ans or ans == 0xee:
        return ans + 0x06
    elif 0xeb == ans or ans == 0xec:
        return ans + 0x0b
    else:
        return ans

def _zu(x):
    pq = x >> 8
    rs = x & 0xff

    if pq % 2:
        t = rs + 0x20
        if t > 0x7f:
            return t
        else:
            return t - 1
    else:
        return rs + 0x7e

class Codec(codecs.Codec):
    encode = codec.encode

    def decode(self, input, errors="strict"):
        buf = []
        for x in ISO2022JP_SPLIT_RE.split(input):
            if x.startswith('\x1b\x24\x42') and x.endswith('\x1b\x28\x42'):
                for bytes in _each_two_byte(x[3:-3]):
                    try:
                        tmp = codec.decode('\x1b\x24\x42%s\x1b\x28\x42' % bytes)[0]
                    except UnicodeDecodeError:
                        sjis = _jis2sjis_one((ord(bytes[0]) << 8) + ord(bytes[1]))
                        tmp = mobilejp_codec.decode(chr(sjis >> 8) + chr(sjis & 0xff), errors)[0]
                    buf.append(tmp)
            else:
                buf.append(x)
        return (u''.join(buf), len(input))

def getregentry():
    return codecs.CodecInfo(
        name='x_iso2022_jp_kddi',
        encode=Codec().encode,
        decode=Codec().decode,
        )


