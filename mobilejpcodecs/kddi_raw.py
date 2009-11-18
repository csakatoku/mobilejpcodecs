# -*- coding: utf-8 -*-
import codecs
import _codecs_mobilejp

codec = _codecs_mobilejp.getcodec('x_sjis_kddi_raw')

class Codec(codecs.Codec):
    decode = codec.decode
    encode = codec.encode

def getregentry():
    return codecs.CodecInfo(
        name='x_sjis_kddi_raw',
        encode=Codec().encode,
        decode=Codec().decode,
        )


