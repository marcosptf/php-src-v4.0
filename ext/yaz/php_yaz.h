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
   | Authors: Adam Dickmeiss <adam@indexdata.dk>                          |
   +----------------------------------------------------------------------+
 */

/* $Id: php_yaz.h,v 1.7 2001/08/10 12:53:06 dickmeiss Exp $ */

#ifndef PHP_YAZ_H
#define PHP_YAZ_H

#if HAVE_YAZ

#ifdef ZTS
#include "TSRM.h"
#endif

extern zend_module_entry yaz_module_entry;
#define yaz_module_ptr &yaz_module_entry

PHP_FUNCTION(yaz_connect);
PHP_FUNCTION(yaz_close);
PHP_FUNCTION(yaz_search);
PHP_FUNCTION(yaz_wait);
PHP_FUNCTION(yaz_errno);
PHP_FUNCTION(yaz_error);
PHP_FUNCTION(yaz_addinfo);
PHP_FUNCTION(yaz_hits);
PHP_FUNCTION(yaz_record);
PHP_FUNCTION(yaz_syntax);
PHP_FUNCTION(yaz_element);
PHP_FUNCTION(yaz_range);
PHP_FUNCTION(yaz_itemorder);
PHP_FUNCTION(yaz_scan);
PHP_FUNCTION(yaz_scan_result);
PHP_FUNCTION(yaz_present);
PHP_FUNCTION(yaz_ccl_conf);
PHP_FUNCTION(yaz_ccl_parse);
PHP_FUNCTION(yaz_database);
PHP_FUNCTION(yaz_sort);

ZEND_BEGIN_MODULE_GLOBALS(yaz)
    int assoc_seq;
ZEND_END_MODULE_GLOBALS(yaz)

#ifdef ZTS
# define YAZSLS_D	zend_mysql_globals *yaz_globals
# define YAZSLS_DC	, YAZSLS_D
# define YAZSLS_C	yaz_globals
# define YAZSLS_CC , YAZSLS_C
# define YAZSG(v) (yaz_globals->v)
# define YAZSLS_FETCH()	zend_yaz_globals *yaz_globals = ts_resource(yaz_globals_id)
#else
# define YAZSLS_D
# define YAZSLS_DC
# define YAZSLS_C
# define YAZSLS_CC
# define YAZSG(v) (yaz_globals.v)
# define YAZSLS_FETCH()
#endif

#else

#define yaz_module_ptr NULL
#endif

#define phpext_yaz_ptr yaz_module_ptr
#endif
