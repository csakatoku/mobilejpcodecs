# -*- coding: utf-8 -*-
import codecs
import _codecs_mobilejp

class Codec(codecs.Codec):
    def decode(self, input, errors='strict'):
        return codecs.utf_8_decode(input, errors)

    def encode(self, input, errors='strict'):
        return _codecs_mobilejp.x_utf_8_softbank_encode(input, errors)


def getregentry():
    return codecs.CodecInfo(
        name='x_utf8_softbank',
        encode=Codec().encode,
        decode=Codec().decode,
        )


