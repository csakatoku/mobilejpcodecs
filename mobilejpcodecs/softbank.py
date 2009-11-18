# -*- coding: utf-8 -*-
import re
import _codecs_jp, codecs
import _codecs_mobilejp

codec = _codecs_jp.getcodec('cp932')
codec_mobilejp = _codecs_mobilejp.getcodec('x_sjis_softbank')

WEBCODE_RE = re.compile(r'\x1b\x24([GEFOPQ])([\x20-\x7F]+)\x0f')

def _decode_webcode(matcher):
    high, chars = matcher.groups()
    high_bit = { 'G': 0xE000,
                 'E': 0xE100,
                 'F': 0xE200,
                 'O': 0xE300,
                 'P': 0xE400,
                 'Q': 0xE500,
                 }[high]
    buf = []
    for char in chars:
        buf.append(unichr(high_bit | ord(char) - 32).encode('cp932'))
    return ''.join(buf)

class Codec(codecs.Codec):
    encode = codec_mobilejp.encode

    def decode(self, input, errors='strict'):
        webcode = WEBCODE_RE.sub(_decode_webcode, input)
        if webcode == input:
            return codec_mobilejp.decode(input, errors)
        else:
            # TODO
            res = codec.decode(webcode, errors)
            return (res[0], len(input))

def getregentry():
    return codecs.CodecInfo(
        name='x_sjis_softbank',
        encode=Codec().encode,
        decode=Codec().decode,
        )
