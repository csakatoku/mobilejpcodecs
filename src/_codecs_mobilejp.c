/*
 * _codecs_jp.c: Codecs collection for Japanese encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */
/*#define EMPBASE 0x20000*/

#define USING_IMPORTED_MAPS

#include "cjkcodecs.h"

#include "mobilejpcodecs.h"
#include "mappings_docomo.h"
#include "mappings_kddi.h"
#include "mappings_softbank.h"

#define EMOJI_UNICODE_MIN 0xe001
#define EMOJI_UNICODE_MAX 0xf0fc

#define maybe_emoji(c) (c >= EMOJI_UNICODE_MIN && c <= EMOJI_UNICODE_MAX)

/* decode only */
static const struct dbcs_index *jisx0208_decmap;

/* encode only */
static const struct unim_index *jisxcommon_encmap;

/* encode/decode */
static const struct dbcs_index *cp932ext_decmap;
static const struct unim_index *cp932ext_encmap;

/*
 * CP932 codec
 */

static Py_ssize_t
encode_mobilejp(MultibyteCodec_State *state, const void *config,
		const Py_UNICODE **inbuf, Py_ssize_t inleft,
		unsigned char **outbuf, Py_ssize_t outleft, int flags, const struct emoji_t *table)
{
	while (inleft > 0) {
		Py_UNICODE c = IN1;
		DBCHAR code;
		unsigned char c1, c2;
		const struct emoji_t *emoji;

		if (maybe_emoji(c)) {
		  emoji = &table[c - EMOJI_UNICODE_MIN];
		  if (emoji->length) {
		    REQUIRE_OUTBUF(emoji->length);

		    memcpy(*outbuf, emoji->data, emoji->length);
		    *outbuf += emoji->length;
		    outleft -= emoji->length;

		    NEXT_IN(1)
		    continue;
		  }
		}

		if (c <= 0x80) {
			WRITE1((unsigned char)c)
			NEXT(1, 1)
			continue;
		}
		else if (c >= 0xff61 && c <= 0xff9f) {
			WRITE1(c - 0xfec0)
			NEXT(1, 1)
			continue;
		}
		else if (c >= 0xf8f0 && c <= 0xf8f3) {
			/* Windows compatibility */
			REQUIRE_OUTBUF(1)
			if (c == 0xf8f0)
				OUT1(0xa0)
			else
				OUT1(c - 0xfef1 + 0xfd)
			NEXT(1, 1)
			continue;
		}

		UCS4INVALID(c)
		REQUIRE_OUTBUF(2)

		TRYMAP_ENC(cp932ext, code, c) {
			OUT1(code >> 8)
			OUT2(code & 0xff)
		}
		else TRYMAP_ENC(jisxcommon, code, c) {
			if (code & 0x8000) /* MSB set: JIS X 0212 */
				return 1;

			/* JIS X 0208 */
			c1 = code >> 8;
			c2 = code & 0xff;
			c2 = (((c1 - 0x21) & 1) ? 0x5e : 0) + (c2 - 0x21);
			c1 = (c1 - 0x21) >> 1;
			OUT1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1)
			OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
		}
		else if (c >= 0xe000 && c < 0xe758) {
			/* User-defined area */
			c1 = (Py_UNICODE)(c - 0xe000) / 188;
			c2 = (Py_UNICODE)(c - 0xe000) % 188;
			OUT1(c1 + 0xf0)
			OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
		}
		else
			return 1;

		NEXT(1, 2)
	}

	return 0;
}

static Py_ssize_t
decode_mobilejp(MultibyteCodec_State *state, const void *config,
		const unsigned char **inbuf, Py_ssize_t inleft,
		Py_UNICODE **outbuf, Py_ssize_t outleft,
		void *decode_func)
{
	while (inleft > 0) {
		unsigned char c = IN1, c2;
		ucs2_t uni;

		REQUIRE_OUTBUF(1)
		if (c <= 0x80) {
			OUT1(c)
			NEXT(1, 1)
			continue;
		}
		else if (c >= 0xa0 && c <= 0xdf) {
			if (c == 0xa0)
				OUT1(0xf8f0) /* half-width katakana */
			else
				OUT1(0xfec0 + c)
			NEXT(1, 1)
			continue;
		}
		else if (c >= 0xfd/* && c <= 0xff*/) {
			/* Windows compatibility */
			OUT1(0xf8f1 - 0xfd + c)
			NEXT(1, 1)
			continue;
		}

		REQUIRE_INBUF(2)
		c2 = IN2;

		if ((uni = ((ucs2_t (*)(unsigned char, unsigned char))decode_func)(c, c2))) {
		  OUT1(uni)
		  NEXT(2, 1)
		  continue;
		}

		TRYMAP_DEC(cp932ext, **outbuf, c, c2);
		else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)){
			if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
				return 2;

			c = (c < 0xe0 ? c - 0x81 : c - 0xc1);
			c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
			c = (2 * c + (c2 < 0x5e ? 0 : 1) + 0x21);
			c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

			TRYMAP_DEC(jisx0208, **outbuf, c, c2);
			else return 2;
		}
		else if (c >= 0xf0 && c <= 0xf9) {
			if ((c2 >= 0x40 && c2 <= 0x7e) ||
			    (c2 >= 0x80 && c2 <= 0xfc))
				OUT1(0xe000 + 188 * (c - 0xf0) +
				     (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41))
			else
				return 2;
		}
		else
			return 2;

		NEXT(2, 1)
	}

	return 0;
}


/*
 * x-sjis-docomo codec based on CP932 codec
 */

ENCODER_INIT(x_sjis_docomo)
{
  return 0;
}

ENCODER_RESET(x_sjis_docomo)
{
  return 0;
}

static Py_ssize_t
x_sjis_docomo_encode(MultibyteCodec_State *state, const void *config,
		     const Py_UNICODE **inbuf, Py_ssize_t inleft,
		     unsigned char **outbuf, Py_ssize_t outleft, int flags)
{
  return encode_mobilejp(state, config, inbuf, inleft, outbuf, outleft, flags, __docomo_decmap);
}

DECODER_INIT(x_sjis_docomo)
{
  return 0;
}

DECODER_RESET(x_sjis_docomo)
{
  return 0;
}

DECODER(x_sjis_docomo)
{
  /* not implemented. use cp932 codec instead */
  return 1;
}

/*
 * x-sjis-kddi codec based on CP932 codec
 */

ENCODER_INIT(x_sjis_kddi)
{
  return 0;
}

ENCODER_RESET(x_sjis_kddi)
{
  return 0;
}

static Py_ssize_t
x_sjis_kddi_encode(MultibyteCodec_State *state, const void *config,
		   const Py_UNICODE **inbuf, Py_ssize_t inleft,
		   unsigned char **outbuf, Py_ssize_t outleft, int flags)
{
  return encode_mobilejp(state, config, inbuf, inleft, outbuf, outleft, flags, __kddi_decmap);
}

DECODER_INIT(x_sjis_kddi)
{
  return 0;
}

DECODER_RESET(x_sjis_kddi)
{
  return 0;
}

static ucs2_t
kddi_decoder(unsigned char c, unsigned char c2)
{
  if ((c == 0xf3 && c2 >= 0x40) || (c > 0xf3 && c < 0xf8)) {
    return ((c << 8) + c2 - 0x0700);
  } else {
    return 0;
  }
}

static Py_ssize_t
x_sjis_kddi_decode(MultibyteCodec_State *state, const void *config,
		   const unsigned char **inbuf, Py_ssize_t inleft,
		   Py_UNICODE **outbuf, Py_ssize_t outleft)
{
  return decode_mobilejp(state, config, inbuf, inleft, outbuf, outleft, (void *)&kddi_decoder);
}

static ucs2_t
kddi_raw_decoder(unsigned char c, unsigned char c2)
{
  ucs2_t code;

  code = (c << 8) + c2;
  if      (code >= 0xF340 && code <= 0xF352) { return code - 3443; } /* 0x0d73 */
  else if (code >= 0xF353 && code <= 0xF37E) { return code - 2259; } /* 0x08d3 */
  else if (code >= 0xF380 && code <= 0xF3CE) { return code - 2260; } /* 0x08d4 */
  else if (code >= 0xF3CF && code <= 0xF3FC) { return code - 2241; } /* 0x08c1 */
  else if (code >= 0xF440 && code <= 0xF47E) { return code - 2308; } /* 0x0904 */
  else if (code >= 0xF480 && code <= 0xF48D) { return code - 2309; } /* 0x0905 */
  else if (code >= 0xF640 && code <= 0xF67E) { return code - 4568; } /* 0x11d8 */
  else if (code >= 0xF680 && code <= 0xF6FC) { return code - 4569; } /* 0x11d9 */
  else if (code >= 0xF740 && code <= 0xF77E) { return code - 4636; } /* 0x121c */
  else if (code >= 0xF780 && code <= 0xF7d1) { return code - 4637; } /* 0x121d */
  else if (code >= 0xF7d2 && code <= 0xF7e4) { return code - 3287; } /* 0x0cd7 */
  else if (code >= 0xF7E5 && code <= 0xF7FC) { return code - 4656; } /* 0x1230 */
  else { return 0; }
}

DECODER_INIT(x_sjis_kddi_raw)
{
  return 0;
}

DECODER_RESET(x_sjis_kddi_raw)
{
  return 0;
}

static Py_ssize_t
x_sjis_kddi_raw_decode(MultibyteCodec_State *state, const void *config,
		       const unsigned char **inbuf, Py_ssize_t inleft,
		       Py_UNICODE **outbuf, Py_ssize_t outleft)
{
  return decode_mobilejp(state, config, inbuf, inleft, outbuf, outleft, (void *)&kddi_raw_decoder);
}


/*
 * x-sjis-kddi-raw codec based on CP932 codec
 */

ENCODER_INIT(x_sjis_kddi_raw)
{
  return 0;
}

ENCODER_RESET(x_sjis_kddi_raw)
{
  return 0;
}

ENCODER(x_sjis_kddi_raw)
{
	while (inleft > 0) {
		Py_UNICODE c = IN1;
		DBCHAR code;
		unsigned char c1, c2;
		const struct emoji_t *emoji;

		if ((c + 3443) >= 0xF340 && (c + 3433) <= 0xF352) { OUT1((c + 3443) >> 8) OUT2((c + 3443) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 2259) >= 0xF353 && (c + 2259) <= 0xF37E) { OUT1((c + 2259) >> 8) OUT2((c + 2259) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 2260) >= 0xF380 && (c + 2260) <= 0xF3CE) { OUT1((c + 2260) >> 8) OUT2((c + 2260) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 2241) >= 0xF3CF && (c + 2241) <= 0xF3FC) { OUT1((c + 2241) >> 8) OUT2((c + 2241) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 2308) >= 0xF440 && (c + 2308) <= 0xF47E) { OUT1((c + 2308) >> 8) OUT2((c + 2308) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 2309) >= 0xF480 && (c + 2309) <= 0xF48D) { OUT1((c + 2309) >> 8) OUT2((c + 2309) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 4568) >= 0xF640 && (c + 4568) <= 0xF67E) { OUT1((c + 4568) >> 8) OUT2((c + 4568) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 4569) >= 0xF680 && (c + 4569) <= 0xF6FC) { OUT1((c + 4569) >> 8) OUT2((c + 4569) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 4636) >= 0xF740 && (c + 4636) <= 0xF77E) { OUT1((c + 4636) >> 8) OUT2((c + 4636) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 4637) >= 0xF780 && (c + 4637) <= 0xF7d1) { OUT1((c + 4637) >> 8) OUT2((c + 4637) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 3287) >= 0xF7d2 && (c + 3287) <= 0xF7e4) { OUT1((c + 3287) >> 8) OUT2((c + 3287) & 0xff) NEXT(1, 2) continue; }
		else if ((c + 4656) >= 0xF7E5 && (c + 4656) <= 0xF7FC) { OUT1((c + 4656) >> 8) OUT2((c + 4656) & 0xff) NEXT(1, 2) continue; }

		/* 0xe001 - 0xf7fc */
		if (maybe_emoji(c)) {
		  emoji = &__kddi_decmap[c - EMOJI_UNICODE_MIN];
		  if (emoji->length) {
		    REQUIRE_OUTBUF(emoji->length);

		    memcpy(*outbuf, emoji->data, emoji->length);
		    *outbuf += emoji->length;
		    outleft -= emoji->length;

		    NEXT_IN(1)
		    continue;
		  }
		}

		if (c <= 0x80) {
			WRITE1((unsigned char)c)
			NEXT(1, 1)
			continue;
		}
		else if (c >= 0xff61 && c <= 0xff9f) {
			WRITE1(c - 0xfec0)
			NEXT(1, 1)
			continue;
		}
		else if (c >= 0xf8f0 && c <= 0xf8f3) {
			/* Windows compatibility */
			REQUIRE_OUTBUF(1)
			if (c == 0xf8f0)
				OUT1(0xa0)
			else
				OUT1(c - 0xfef1 + 0xfd)
			NEXT(1, 1)
			continue;
		}

		UCS4INVALID(c)
		REQUIRE_OUTBUF(2)

		TRYMAP_ENC(cp932ext, code, c) {
			OUT1(code >> 8)
			OUT2(code & 0xff)
		}
		else TRYMAP_ENC(jisxcommon, code, c) {
			if (code & 0x8000) /* MSB set: JIS X 0212 */
				return 1;

			/* JIS X 0208 */
			c1 = code >> 8;
			c2 = code & 0xff;
			c2 = (((c1 - 0x21) & 1) ? 0x5e : 0) + (c2 - 0x21);
			c1 = (c1 - 0x21) >> 1;
			OUT1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1)
			OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
		}
		else if (c >= 0xe000 && c < 0xe758) {
			/* User-defined area */
			c1 = (Py_UNICODE)(c - 0xe000) / 188;
			c2 = (Py_UNICODE)(c - 0xe000) % 188;
			OUT1(c1 + 0xf0)
			OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
		}
		else
			return 1;

		NEXT(1, 2)
	}

	return 0;
}

/*
 * x-sjis-softbank codec based on CP932 codec
 */

ENCODER_INIT(x_sjis_softbank)
{
  return 0;
}

ENCODER_RESET(x_sjis_softbank)
{
  return 0;
}

static Py_ssize_t
x_sjis_softbank_encode(MultibyteCodec_State *state, const void *config,
		   const Py_UNICODE **inbuf, Py_ssize_t inleft,
		   unsigned char **outbuf, Py_ssize_t outleft, int flags)
{
  return encode_mobilejp(state, config, inbuf, inleft, outbuf, outleft, flags, __softbank_decmap);
}

DECODER_INIT(x_sjis_softbank)
{
  return 0;
}

DECODER_RESET(x_sjis_softbank)
{
  return 0;
}

static ucs2_t
softbank_decoder(unsigned char high, unsigned char low)
{
  ucs2_t code, base;

  if (high == 0xf7) {
    base = low < 0xa0 ? 0xe100 : 0xe200;
  } else if (high == 0xf9) {
    base = low < 0xa0 ? 0xe000 : 0xe300;
  } else if (high == 0xfb) {
    base = low < 0xa0 ? 0xe400 : 0xe500;
  } else {
    /* not softbank emoji */
    return 0;
  }

  if (low < 0x80) {
    code = base + (low - 0x40);
  } else if (low > 0xa0) {
    code = base + (low - 0xa0);
  } else {
    code = base + (low - 0x41);
  }

  return code;
}

static Py_ssize_t
x_sjis_softbank_decode(MultibyteCodec_State *state, const void *config,
		   const unsigned char **inbuf, Py_ssize_t inleft,
		   Py_UNICODE **outbuf, Py_ssize_t outleft)
{
  return decode_mobilejp(state, config, inbuf, inleft, outbuf, outleft, (void *)&softbank_decoder);
}

/*
 * utf8 encoding
 */

static PyObject* empty_unicode;

static PyObject* docomo_table=NULL;
static PyObject* kddi_table=NULL;
static PyObject* softbank_table=NULL;

static int
init_emoji_table(PyObject *table)
{
  PyObject *key, *value, *emoji, *item, *unicode;
  Py_ssize_t pos = 0;

  if ((docomo_table = PyDict_New()) == NULL ||
      (kddi_table = PyDict_New()) == NULL ||
      (softbank_table = PyDict_New()) == NULL) {
    goto error;
  }

  while (PyDict_Next(table, &pos, &key, &value)) {
    if (key == NULL || value == NULL) {
      goto error;
    }

    /* docomo */
    if (!(emoji = PyDict_GetItemString(value, "d"))) {
      goto error;
    }
    if (!(item = PyDict_GetItemString(emoji, "unicode"))) {
      goto error;
    }
    if ((unicode = PyUnicode_FromObject(item)) == NULL ) {
      goto error;
    }
    if (PyDict_SetItem(docomo_table, key, unicode) < 0) {
      goto error;
    }

    /* kddi */
    if (!(emoji = PyDict_GetItemString(value, "k"))) {
      goto error;
    }
    if (!(item = PyDict_GetItemString(emoji, "unicode"))) {
      goto error;
    }
    if ((unicode = PyUnicode_FromObject(item)) == NULL ) {
      goto error;
    }
    if (PyDict_SetItem(kddi_table, key, unicode) < 0) {
      goto error;
    }

    /* softbank */
    if (!(emoji = PyDict_GetItemString(value, "s"))) {
      goto error;
    }
    if (!(item = PyDict_GetItemString(emoji, "unicode"))) {
      goto error;
    }
    if ((unicode = PyUnicode_FromObject(item)) == NULL ) {
      goto error;
    }
    if (PyDict_SetItem(softbank_table, key, unicode) < 0) {
      goto error;
    }
  }

  return 0;

 error:
  Py_XDECREF(docomo_table);
  Py_XDECREF(kddi_table);
  Py_XDECREF(softbank_table);
  return 1;
}

/*
 * helper function copied from python source
 * see Module/_codecsmodule.c
 */
static
PyObject *codec_tuple(PyObject *unicode,
		      Py_ssize_t len)
{
    PyObject *v;
    if (unicode == NULL)
        return NULL;
    v = Py_BuildValue("On", unicode, len);
    Py_DECREF(unicode);
    return v;
}

/*
 * unicode decode function
 */

static PyObject*
x_utf_8_mobilejp_encode(PyObject *table, PyObject *str, const char *errors)
{
  PyObject *result_str = NULL;
  PyObject *result = NULL;
  PyObject *unistr, *buf, *trans;
  Py_UNICODE *c;
  int i;

  str = PyUnicode_FromObject(str);
  if (str == NULL) {
    return NULL;
  }

  if ((buf = PyList_New(0)) == NULL) {
    Py_DECREF(str);
    return NULL;
  }

  c = PyUnicode_AS_UNICODE(str);
  for (i = 0; i < PyUnicode_GET_SIZE(str); i++, c++) {
    if ((unistr = PyUnicode_FromUnicode(c, 1)) == NULL) {
      goto finally;
    }

    if (maybe_emoji(*c)) {
      trans = PyDict_GetItem(table, unistr);
      if (trans && PyUnicode_CheckExact(trans)) {
	Py_DECREF(unistr);

	if (PyList_Append(buf, trans)) {
	  /* append failed */
	  goto finally;
	}

	/* OK */
	continue;
      }
    }

    if (PyList_Append(buf, unistr)) {
      Py_DECREF(unistr);
      goto finally;
    }

    /* OK, not emoji or no translation found in mapping table */
    Py_DECREF(unistr);
  }

  result_str = PyUnicode_Join(empty_unicode, buf);
  if (result_str) {
    result = codec_tuple(PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(result_str),
					      PyUnicode_GET_SIZE(result_str),
					      errors),
			 PyUnicode_GET_SIZE(str));
  }

 finally:
  Py_XDECREF(result_str);
  Py_DECREF(buf);

  return result;
}

/*
 * utf8 encode function for docomo
 */

static PyObject*
x_utf_8_docomo_encode(PyObject *self, PyObject *args)
{
  PyObject *str;
  const char *errors = NULL;

  if (!PyArg_ParseTuple(args, "O|z:x_utf_8_docomo_encode",
			&str, &errors))
    return NULL;

  return x_utf_8_mobilejp_encode(docomo_table, str, errors);
}

/*
 * utf8 encode function for kddi
 */

static PyObject*
x_utf_8_kddi_encode(PyObject *self, PyObject *args)
{

  PyObject *str;
  const char *errors = NULL;

  if (!PyArg_ParseTuple(args, "O|z:x_utf_8_kddi_encode",
			&str, &errors))
    return NULL;

  return x_utf_8_mobilejp_encode(kddi_table, str, errors);
}

/*
 * utf8 encode function for softbank
 */

static PyObject*
x_utf_8_softbank_encode(PyObject *self, PyObject *args)
{
  PyObject *str;
  const char *errors = NULL;

  if (!PyArg_ParseTuple(args, "O|z:x_utf_8_softbank_encode",
			&str, &errors))
    return NULL;

  return x_utf_8_mobilejp_encode(softbank_table, str, errors);
}

/*
 * initialize module
 */

BEGIN_CODECS_LIST
  CODEC_STATEFUL(x_sjis_docomo)
  CODEC_STATEFUL(x_sjis_kddi)
  CODEC_STATEFUL(x_sjis_kddi_raw)
  CODEC_STATEFUL(x_sjis_softbank)
END_CODECS_LIST

static struct PyMethodDef __methods[] = {
	{"getcodec", (PyCFunction)getcodec, METH_O, ""},
	{"x_utf_8_docomo_encode", (PyCFunction)x_utf_8_docomo_encode, METH_VARARGS, ""},
	{"x_utf_8_kddi_encode", (PyCFunction)x_utf_8_kddi_encode, METH_VARARGS, ""},
	{"x_utf_8_softbank_encode", (PyCFunction)x_utf_8_softbank_encode, METH_VARARGS, ""},
	{NULL, NULL},
};

void
init_codecs_mobilejp(void)
{
  PyObject *module, *table_module;
  PyObject *emoji_table;

  if (IMPORT_MAP(jp, jisx0208, NULL, &jisx0208_decmap) < 0 ||
      IMPORT_MAP(jp, jisxcommon, &jisxcommon_encmap, NULL) < 0 ||
      IMPORT_MAP(jp, cp932ext, &cp932ext_encmap, &cp932ext_decmap) < 0) {
    return;
  }

  if ((empty_unicode = PyUnicode_FromUnicode(NULL, 0)) == NULL) {
    return;
  }

  if ((table_module = PyImport_ImportModule("mobilejpcodecs.emojitable")) == NULL ||
      (emoji_table = PyObject_GetAttrString(table_module, "TABLE")) == NULL) {
    return;
  }

  if (init_emoji_table(emoji_table)) {
    goto finally;
  }

  module = Py_InitModule("_codecs_mobilejp", __methods);

 finally:
  Py_DECREF(emoji_table);
  Py_DECREF(table_module);
}
