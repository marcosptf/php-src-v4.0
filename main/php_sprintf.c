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
   | Authors: Jaakko Hyv�tti <jaakko.hyvatti@iki.fi>                      |
   +----------------------------------------------------------------------+
 */

/* $Id: php_sprintf.c,v 1.9 2001/06/06 13:05:53 rasmus Exp $ */

#include <stdio.h>
#include <stdarg.h>
#include "php_config.h"

#if PHP_BROKEN_SPRINTF

int
php_sprintf (char*s, const char* format, ...)
{
  va_list args;
  char *ret;

  va_start (args, format);
  s[0] = '\0';
  ret = vsprintf (s, format, args);
  va_end (args);
  if (!ret)
    return -1;
  return strlen (s);
}

#endif /* BROKEN_SPRINTF */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
