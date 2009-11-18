# -*- coding: utf-8 -*-
import _codecs_jp, codecs
import _codecs_mobilejp

codec = _codecs_jp.getcodec('cp932')
codec_mobilejp = _codecs_mobilejp.getcodec('x_sjis_docomo')

class Codec(codecs.Codec):
    decode = codec.decode
    encode = codec_mobilejp.encode

def getregentry():
    return codecs.CodecInfo(
        name='x_sjis_docomo',
        encode=Codec().encode,
        decode=Codec().decode,
        )


