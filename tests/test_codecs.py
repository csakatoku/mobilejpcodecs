# -*- coding: utf-8 -*-
import binascii
import codecs
import mobilejpcodecs

CARRIERS = ('docomo', 'kddi', 'kddi_raw', 'softbank')

def test_docomo_encode():
    docomo = codecs.lookup('x_sjis_docomo')

    res = docomo.encode(u"\uE63E")
    assert res == ("\xF8\x9F", 1), res

    res = docomo.encode(u"\uec47")
    assert res == ('[\xb6\xd2]', 1), res

    res = docomo.encode(u"\ue05a")
    assert res == ("\x81\xac", 1), res

    s = docomo.encode(u"\uecb6")[0]
    s100 = docomo.encode(u"\uecb6" * 100)[0]
    assert s * 100 == s100

def test_kddi_encode():
    codec = codecs.lookup('x_sjis_softbank')
    s = codec.encode(u"\ue6ad")[0]
    s100 = codec.encode(u"\ue6ad" * 100)[0]
    assert s * 100 == s100

def test_softbank_encode():
    codec = codecs.lookup('x_sjis_softbank')
    s = codec.encode(u"\ue6ad")[0]
    s100 = codec.encode(u"\ue6ad" * 100)[0]
    assert s * 100 == s100


def test_docomo_utf8_encode():
    from mobilejpcodecs import docomo_utf8
    codec = docomo_utf8.Codec()

    # docomo
    res = codec.encode(u"\ue63e")
    assert res == ("\xee\x98\xbe", 1), res

    # softbank
    res = codec.encode(u"\ue001")
    assert res == ("\xee\x9b\xb0", 1), res

    # kddi
    res = codec.encode(u"\uec47")
    assert res == ('[\xef\xbd\xb6\xef\xbe\x92]', 1), res

def test_docomo_utf8_decode():
    from mobilejpcodecs import docomo_utf8
    codec = docomo_utf8.Codec()

    def func(input, expected):
        res = codec.decode(input)
        assert isinstance(res[0], unicode)
        assert res[0] == expected

    for i, o in (('\xee\x98\xbe', u'\ue63e'),
                 ('\xee\x9d\x97', u'\ue757')):
        yield func, i, o

def test_kddi_utf8_decode():
    from mobilejpcodecs import docomo_utf8
    codec = docomo_utf8.Codec()

    def func(input, expected):
        res = codec.decode(input)
        assert isinstance(res[0], unicode)
        assert res[0] == expected

    for i, o in (('\xee\xbd\x99', u'\uef59'),
                 ('\xee\xb6\x8d', u'\ued8d')):
        yield func, i, o

def test_softbank_utf8_decode():
    from mobilejpcodecs import docomo_utf8
    codec = docomo_utf8.Codec()

    def func(input, expected):
        res = codec.decode(input)
        assert isinstance(res[0], unicode)
        assert res[0] == expected

    for i, o in (('\xee\x80\x81', u'\ue001'),
                 ('\xee\x94\xb7', u'\ue537')):
        yield func, i, o

def test_kddi_utf8_encode():
    from mobilejpcodecs import kddi_utf8
    codec = kddi_utf8.Codec()

    # docomo
    res = codec.encode(u"\ue63e")
    assert res == ("\xee\xbd\xa0", 1), res

    # softbank
    res = codec.encode(u"\ue001")
    assert res == ("\xee\xbf\x95", 1), res

    # kddi
    res = codec.encode(u"\uec47")
    assert res == ('\xee\xb1\x87', 1), res

def test_softbank_utf8_encode():
    from mobilejpcodecs import softbank_utf8
    codec = softbank_utf8.Codec()

    # docomo
    res = codec.encode(u"\ue63e")
    assert res == ("\xee\x81\x8a", 1), res

    # softbank
    res = codec.encode(u"\ue001")
    assert res == ("\xee\x80\x81", 1), res

    # kddi
    res = codec.encode(u"\uec47")
    assert res == ('[\xef\xbd\xb6\xef\xbe\x92]', 1), res

def test_utf8_convert():
    from mobilejpcodecs import softbank_utf8
    codec = softbank_utf8.Codec()
    assert (codec.encode(u'\ue74b')[0]).decode("utf8") == u'\ue30b'

def test_min_unicode():
    def func(carrier):
        c = codecs.lookup('x_sjis_%s' % carrier)
        res = c.encode(u"\ue001")

        res = c.encode(u"\ue000")
        assert res[0] == u"\ue000".encode('cp932')

    for carrier in (CARRIERS):
        yield func, carrier

def test_max_unicode():
    def func(carrier):
        c = codecs.lookup('x_sjis_%s' % carrier)
        res = c.encode(u"\uf0fc")

    for carrier in (CARRIERS):
        yield func, carrier

def test_encode_error():
    def func(carrier):
        c = codecs.lookup('x_sjis_%s' % carrier)
        try:
            res = c.encode(u"\uf0fd")
        except UnicodeEncodeError:
            pass
        else:
            assert False

    for carrier in (CARRIERS):
        yield func, carrier

def test_softbank_webcode():
    softbank = codecs.lookup('x_sjis_softbank')

    def func(input, expected):
        output = softbank.decode(input)
        assert output == (expected, len(input)), '%r expected, actual %r' % ((expected, len(input)),
                                                                             output)

    for input, expected in (("\x1b$G.\x0f",
                             u"\uE00E"),
                            ("\x1b$GB.\x0f",
                             u"\uE022\uE00E"),
                            ("\x1b$G.B\x0f",
                             u"\uE00E\uE022"),
                            ("\x1b$P=3\x0f",
                             u"\uE41D\uE413"),
                            ("\x1B\x24Gj\x0F" + "\x1B\x24F.\x0F" + "\x1B\x24Ec\x0F",
                             u"\uE04A\uE20E\uE143"),
                            ):
        yield func, input, expected

def test_softbank_obsolete():
    softbank = codecs.lookup('x_sjis_softbank')

    def func(input, expected):
        output = softbank.decode(binascii.unhexlify(input))
        assert output[0] == expected, output

    for input, expected in (("1b2446740f", u"\uE254"), # 284:?
                            ("1b2446750f", u"\uE255"), # 285:SkyWeb
                            ("1b2446760f", u"\uE256"), # 286:SkyWalker
                            ("1b2446770f", u"\uE257"), # 287:SkyMelody
                            ("1b2446780f", u"\ue258"), # 288:?
                            ("1b2446790f", u"\ue259"), # 289:?
                            ("1b24467a0f", u"\ue25a"), # 290:?
                            ("1b2451580f", u"\ue538"), # 556
                            ("1b2451590f", u"\ue539"), # 557
                            ("1b24515a0f", u"\ue53a"), # 558
                            ("1b24515b0f", u"\ue53b"), # 559
                            ("1b24515c0f", u"\ue53c"), # 560:"[v"
                            ("1b24515d0f", u"\ue53d"), # 561:"oda"
                            ("1b24515e0f", u"\ue53e"), # 562:"fone]"
                            ):
        yield func, input, expected

def test_iso2022_jp_kddi_decode():
    def func(value, expected):
        result = binascii.unhexlify(value.replace(' ', '')).decode('x_iso2022_jp_kddi')
        assert isinstance(result, unicode)
        assert result == expected, "%r expected, actual %r" % (expected, result)

    for v, e in (('1b 24 42 75 41 1b 28 42',
                  u'\uEF60'),
                 ('1b 24 42 76 3b 1b 28 42 0a 1b 24 42 76 3b 1b 28 42',
                  u'\uEFB9\n\uEFB9'),
                 ('1b 24 42 25 46 25 39 25 48 78 70 1b 28 42',
                  u'\u30c6\u30b9\u30c8\uF0EE'),
                 ('1b 24 42 25 46 25 39 25 48 78 70 25 46 25 39 25 48 1b 28 42',
                  u'\u30c6\u30b9\u30c8\uF0EE\u30c6\u30b9\u30c8'),
                 ('1b 24 42 75 41 1b 28 42 73 70 61 6d 1b 24 42 76 3b 1b 28 42 65 67 67 1b 24 42 75 31 1b 28 42 0a',
                  u'\uEF60spam\uEFB9egg\uEF50\n'),
                 ):
        yield (func, v, e)

