/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Jaakko Hyv�tti <jaakko.hyvatti@iki.fi>                      |
   |          Wez Furlong <wez@thebrainroom.com>                          |
   +----------------------------------------------------------------------+
*/

/* $Id: html.c,v 1.32 2001/08/11 17:03:37 zeev Exp $ */

#include "php.h"
#include "reg.h"
#include "html.h"

#if HAVE_LOCALE_H
#include <locale.h>
#endif
#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif

/* This must be fixed to handle the input string according to LC_CTYPE.
   Defaults to ISO-8859-1 for now. */

enum entity_charset { cs_terminator, cs_8859_1, cs_cp1252,
	cs_8859_15, cs_utf_8 };
typedef const char * entity_table_t;

/* codepage 1252 is a Windows extension to iso-8859-1. */
static entity_table_t ent_cp_1252[] = {
	NULL, NULL, "sbquo", "fnof", "bdquo", "hellip", "dagger",
	"Dagger", "circ", "permil", "Scaron", "lsaquo", "OElig",
	NULL, NULL, NULL, NULL, "lsquo", "rsquo", "ldquo", "rdquo",
	"bull", "ndash", "mdash", "tilde", "trade", "scaron", "rsaquo",
	"oelig", NULL, NULL, "Yuml" 
};

static entity_table_t ent_iso_8859_1[] = {
	"nbsp", "iexcl", "cent", "pound", "curren", "yen", "brvbar",
	"sect", "uml", "copy", "ordf", "laquo", "not", "shy", "reg",
	"macr", "deg", "plusmn", "sup2", "sup3", "acute", "micro",
	"para", "middot", "cedil", "sup1", "ordm", "raquo", "frac14",
	"frac12", "frac34", "iquest", "Agrave", "Aacute", "Acirc",
	"Atilde", "Auml", "Aring", "AElig", "Ccedil", "Egrave",
	"Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc",
	"Iuml", "ETH", "Ntilde", "Ograve", "Oacute", "Ocirc", "Otilde",
	"Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml",
	"Yacute", "THORN", "szlig", "agrave", "aacute", "acirc",
	"atilde", "auml", "aring", "aelig", "ccedil", "egrave",
	"eacute", "ecirc", "euml", "igrave", "iacute", "icirc",
	"iuml", "eth", "ntilde", "ograve", "oacute", "ocirc", "otilde",
	"ouml", "divide", "oslash", "ugrave", "uacute", "ucirc",
	"uuml", "yacute", "thorn", "yuml"
};

static entity_table_t ent_iso_8859_15[] = {
	"nbsp", "iexcl", "cent", "pound", "euro", "yen", "Scaron",
	"sect", "scaron", "copy", "ordf", "laquo", "not", "shy", "reg",
	"macr", "deg", "plusmn", "sup2", "sup3", NULL, /* Zcaron */
	"micro", "para", "middot", NULL, /* zcaron */ "sup1", "ordm",
	"raquo", "OElig", "oelig", "Yuml", "iquest", "Agrave", "Aacute",
	"Acirc", "Atilde", "Auml", "Aring", "AElig", "Ccedil", "Egrave",
	"Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc",
	"Iuml", "ETH", "Ntilde", "Ograve", "Oacute", "Ocirc", "Otilde",
	"Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml",
	"Yacute", "THORN", "szlig", "agrave", "aacute", "acirc",
	"atilde", "auml", "aring", "aelig", "ccedil", "egrave",
	"eacute", "ecirc", "euml", "igrave", "iacute", "icirc",
	"iuml", "eth", "ntilde", "ograve", "oacute", "ocirc", "otilde",
	"ouml", "divide", "oslash", "ugrave", "uacute", "ucirc",
	"uuml", "yacute", "thorn", "yuml"
};

struct html_entity_map {
	enum entity_charset charset;	/* charset identifier */
	unsigned short basechar;			/* char code at start of table */
	unsigned short endchar;			/* last char code in the table */
	entity_table_t * table;			/* the table of mappings */
};

static const struct html_entity_map entity_map[] = {
	{ cs_cp1252, 	0x80, 0x9f, ent_cp_1252 },
	{ cs_cp1252, 	0xa0, 0xff, ent_iso_8859_1 },
	{ cs_8859_1, 		0xa0, 0xff, ent_iso_8859_1 },
	{ cs_8859_15, 		0xa0, 0xff, ent_iso_8859_15 },
	{ cs_utf_8, 		0xa0, 0xff, ent_iso_8859_1 },
	{ cs_terminator }
};

static const struct {
	const char * codeset;
	enum entity_charset charset;
} charset_map[] = {
	{ "ISO-8859-1", 	cs_8859_1 },
	{ "ISO-8859-15", 	cs_8859_15 },
	{ "utf-8", 			cs_utf_8 },
	{ "cp1252", 		cs_cp1252 },
	{ NULL }
};

/* {{{ get_next_char
 */
inline static unsigned short get_next_char(enum entity_charset charset,
		unsigned char * str,
		int * newpos,
		unsigned char * mbseq,
		int * mbseqlen
)
{
	int pos = *newpos;
	int mbpos = 0;
	unsigned short this_char = str[pos++];
	
	mbseq[mbpos++] = (unsigned char)this_char;
	
	if (charset == cs_utf_8)	{
		unsigned long utf = 0;
		int stat = 0;
		int more = 1;

		/* unpack utf-8 encoding into a wide char.
		 * Code stolen from the mbstring extension */
		
		do {
			if (this_char < 0x80)	{
				more = 0;
				break;
			}
			else if (this_char < 0xc0)	{
				switch(stat)	{
					case 0x10:	/* 2, 2nd */
					case 0x21:	/* 3, 3rd */
					case 0x32:	/* 4, 4th */
					case 0x43:	/* 5, 5th */
					case 0x54:	/* 6, 6th */
						/* last byte in sequence */
						more = 0;
						utf |= (this_char & 0x3f);
						this_char = (unsigned short)utf;
						break;
					case 0x20:	/* 3, 2nd */
					case 0x31:	/* 4, 3rd */
					case 0x42:	/* 5, 4th */
					case 0x53:	/* 6, 5th */
						/* penultimate char */
						utf |= ((this_char & 0x3f) << 6);
						stat++;
						break;
					case 0x30:	/* 4, 2nd */
					case 0x41:	/* 5, 3rd */
					case 0x52:	/* 6, 4th */
						utf |= ((this_char & 0x3f) << 12);
						stat++;
						break;
					case 0x40:	/* 5, 2nd */
					case 0x51:
						utf |= ((this_char & 0x3f) << 18);
						stat++;
						break;
					case 0x50:	/* 6, 2nd */
						utf |= ((this_char & 0x3f) << 24);
						stat++;
					default:
						/* invalid */
						more = 0;
				}
			}
			/* lead byte */
			else if (this_char < 0xe0) {
				stat = 0x10;	/* 2 byte */
				utf = (this_char & 0x1f) << 6;
			} else if (this_char < 0xf0)	{
				stat = 0x20;	/* 3 byte */
				utf = (this_char & 0xf) << 12;
			} else if (this_char < 0xf8) {
				stat = 0x30;	/* 4 byte */
				utf = (this_char & 0x7) << 18;
			} else if (this_char < 0xfc)	{
				stat = 0x40;	/* 5 byte */
				utf = (this_char & 0x3) << 24;
			} else if (this_char < 0xfe)	{
				stat = 0x50;	/* 6 byte */
				utf = (this_char & 0x1) << 30;
			}
			else	{
				/* invalid; bail */
				more = 0;
				break;
			}
			if (more)
			{
				this_char = str[pos++];
				mbseq[mbpos++] = (unsigned char)this_char;
			}
		} while(more);
	}
	*newpos = pos;
	mbseq[mbpos] = '\0';
	*mbseqlen = mbpos;
	return this_char;
}
/* }}} */

/* {{{ entity_charset determine_charset
 * returns the charset identifier based on current locale or a hint.
 * defaults to iso-8859-1 */
static enum entity_charset determine_charset(char * charset_hint)
{
	int i;
	enum entity_charset charset = cs_8859_1;
	int len;

	/* Guarantee default behaviour */
	if (charset_hint == NULL)
		return cs_8859_1;

	if (strlen(charset_hint) == 0)	{
		/* try to detect the charset for the locale */
#if HAVE_NL_LANGINFO && HAVE_LOCALE_H && defined(CODESET)
		charset_hint = nl_langinfo(CODESET);
#endif
#if HAVE_LOCALE_H
		if (charset_hint == NULL)
		{
			/* try to figure out the charset from the locale */
			char * localename;
			char * dot, * at;

			/* lang[_territory][.codeset][@modifier] */
			localename = setlocale(LC_CTYPE, NULL);

			dot = strchr(localename, '.');
			if (dot)	{
				dot++;
				/* locale specifies a codeset */
				at = strchr(dot, '@');
				if (at)
					len = at - dot;
				else
					len = strlen(dot);
				charset_hint = dot;
			}
			else	{
				/* no explicit name; see if the name itself
				 * is the charset */
				charset_hint = localename;
				len = strlen(charset_hint);
			}
		}
		else
			len = strlen(charset_hint);
#else
		if (charset_hint)
			len = strlen(charset_hint);
#endif
	}
	if (charset_hint)	{
		/* now walk the charset map and look for the codeset */
		for (i = 0; charset_map[i].codeset; i++)	{
			if (strncasecmp(charset_hint, charset_map[i].codeset, len) == 0)	{
				charset = charset_map[i].charset;
				break;
			}
		}
	}
	return charset;
}
/* }}} */

/* {{{ php_escape_html_entities
 */
PHPAPI char *php_escape_html_entities(unsigned char *old, int oldlen, int *newlen, int all, int quote_style, char * hint_charset)
{
	int i, maxlen, len;
	char *new;
	enum entity_charset charset = determine_charset(hint_charset);

	maxlen = 2 * oldlen;
	if (maxlen < 128)
		maxlen = 128;
	new = emalloc (maxlen);
	len = 0;

	i = 0;
	while (i < oldlen) {
		int mbseqlen;
		unsigned char mbsequence[16];	/* allow up to 15 characters
													in a multibyte sequence
													it should be more than enough.. */
		unsigned short this_char = get_next_char(charset, old, &i, mbsequence, &mbseqlen);
		int matches_map = 0;
		
		if (len + 9 > maxlen)
			new = erealloc (new, maxlen += 128);
		
		if (all)	{
			/* look for a match in the maps for this charset */
			int j;
			unsigned char * rep;
		
			for (j=0; entity_map[j].charset != cs_terminator; j++)	{
				if (entity_map[j].charset == charset
						&& this_char >= entity_map[j].basechar
						&& this_char <= entity_map[j].endchar)
				{
					rep = (unsigned char*)entity_map[j].table[this_char - entity_map[j].basechar];
					if (rep == NULL)	{
						/* there is no entity for this position; fall through and
						 * just output the character itself */
						break;
					}
					
					matches_map = 1;
					break;
				}
			}

			if (matches_map)	{
				new[len++] = '&';
				strcpy(new + len, rep);
				len += strlen(rep);
				new[len++] = ';';
			}
		}
		if (!matches_map)	{	
			if (38 == this_char) {
				memcpy (new + len, "&amp;", 5);
				len += 5;
			} else if (34 == this_char && !(quote_style&ENT_NOQUOTES)) {
				memcpy (new + len, "&quot;", 6);
				len += 6;
			} else if (39 == this_char && (quote_style&ENT_QUOTES)) {
				memcpy (new + len, "&#039;", 6);
				len += 6;
			} else if (60 == this_char) {
				memcpy (new + len, "&lt;", 4);
				len += 4;
			} else if (62 == this_char) {
				memcpy (new + len, "&gt;", 4);
				len += 4;
			} else if (this_char > 0xff)	{
				/* a wide char without a named entity; pass through the original sequence */
				memcpy(new + len, mbsequence, mbseqlen);
				len += mbseqlen;
			} else {
				new [len++] = (unsigned char)this_char;
			}
		}
	}
	new [len] = '\0';
	*newlen = len;

	return new;


}
/* }}} */

/* {{{ php_html_entities
 */
static void php_html_entities(INTERNAL_FUNCTION_PARAMETERS, int all)
{
	zval **arg, **quotes, **charset;
	int len, quote_style = ENT_COMPAT;
	int ac = ZEND_NUM_ARGS();
	char *hint_charset = NULL;
	char *new;

	if (ac < 1 || ac > 3 || zend_get_parameters_ex(ac, &arg, &quotes, &charset) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(arg);
	if(ac==2) {
		convert_to_long_ex(quotes);
		quote_style = (*quotes)->value.lval;
	}
	if (ac == 3)	{
		convert_to_string_ex(charset);
		hint_charset = Z_STRVAL_PP(charset);
	}
		

	new = php_escape_html_entities((*arg)->value.str.val, (*arg)->value.str.len, &len, all, quote_style, hint_charset);
	RETVAL_STRINGL(new, len, 0);
}
/* }}} */

#define HTML_SPECIALCHARS 	0
#define HTML_ENTITIES	 	1

/* {{{ register_html_constants
 */
void register_html_constants(INIT_FUNC_ARGS)
{
	REGISTER_LONG_CONSTANT("HTML_SPECIALCHARS", HTML_SPECIALCHARS, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("HTML_ENTITIES", HTML_ENTITIES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("ENT_COMPAT", ENT_COMPAT, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("ENT_QUOTES", ENT_QUOTES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("ENT_NOQUOTES", ENT_NOQUOTES, CONST_PERSISTENT|CONST_CS);
}
/* }}} */

/* {{{ proto string htmlspecialchars(string string [, int quote_style][, string charset])
   Convert special characters to HTML entities */
PHP_FUNCTION(htmlspecialchars)
{
	php_html_entities(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto string htmlentities(string string [, int quote_style][, string charset])
   Convert all applicable characters to HTML entities */
PHP_FUNCTION(htmlentities)
{
	php_html_entities(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto array get_html_translation_table([int table [, int quote_style][, string charset]])
   Returns the internal translation table used by htmlspecialchars and htmlentities */
PHP_FUNCTION(get_html_translation_table)
{
	zval **whichone, **quotes;
	int which = HTML_SPECIALCHARS, quote_style = ENT_COMPAT;
	int ac = ZEND_NUM_ARGS();
	int i, j;
	char ind[ 2 ];
	enum entity_charset charset = determine_charset(NULL);

	if (ac < 0 || ac > 2 || zend_get_parameters_ex(ac, &whichone, &quotes) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (ac > 0) {
		convert_to_long_ex(whichone);
		which = (*whichone)->value.lval;
	} 
	if (ac == 2) {
		convert_to_long_ex(quotes);
		quote_style = (*quotes)->value.lval;
	}

	array_init(return_value);

	ind[1] = 0;

	switch (which) {
		case HTML_ENTITIES:
			for (j=0; entity_map[j].charset != cs_terminator; j++)	{
				if (entity_map[j].charset != charset)
					continue;
				for (i = 0; i < entity_map[j].endchar - entity_map[j].basechar; i++)
				{
					char buffer[16];

					if (entity_map[j].table[i] == NULL)
						continue;
					/* what about wide chars here ?? */
					ind[0] = i + entity_map[j].basechar;
					sprintf(buffer, "&%s;", entity_map[j].table[i]);
					add_assoc_string(return_value, ind, buffer, 1);

				}
			}
			/* break thru */

		case HTML_SPECIALCHARS:
			ind[0]=38; add_assoc_string(return_value, ind, "&amp;", 1);
			if(quote_style&ENT_QUOTES) {
				ind[0]=39; add_assoc_string(return_value, ind, "&#039;", 1);
			}
			if(!(quote_style&ENT_NOQUOTES)) {
				ind[0]=34; add_assoc_string(return_value, ind, "&quot;", 1); 
			}
			ind[0]=60; add_assoc_string(return_value, ind, "&lt;", 1);
			ind[0]=62; add_assoc_string(return_value, ind, "&gt;", 1);
			break;
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
