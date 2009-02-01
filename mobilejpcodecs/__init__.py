# -*- coding: utf-8 -*-
import codecs

def search_function(encoding):
    import docomo, docomo_utf8
    import kddi, kddi_raw, kddi_utf8, iso2022_jp_kddi
    import softbank, softbank_utf8

    if encoding == 'x_sjis_docomo':
        return docomo.getregentry()
    elif encoding == 'x_utf8_docomo':
        return docomo_utf8.getregentry()
    elif encoding == 'x_sjis_kddi':
        return kddi.getregentry()
    elif encoding == 'x_sjis_kddi_raw':
        return kddi_raw.getregentry()
    elif encoding == 'x_utf8_kddi':
        return kddi_utf8.getregentry()
    elif encoding == 'x_sjis_softbank':
        return softbank.getregentry()
    elif encoding == 'x_utf8_softbank':
        return softbank_utf8.getregentry()
    elif encoding == 'x_iso2022_jp_kddi':
        return iso2022_jp_kddi.getregentry()
    else:
        return None

codecs.register(search_function)
