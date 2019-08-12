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
   | Authors: Frank M. Kromann <fmk@swwwing.com>                          |
   +----------------------------------------------------------------------+
*/
/* $Id: setup.h,v 1.4 2001/08/15 12:38:33 phanto Exp $ */

#ifndef PHP_IIS_SETUP_H
#define PHP_IIS_SETUP_H

#ifdef PHP_WIN32

#define HAVE_IISFUNC

extern zend_module_entry iisfunc_module_entry;
#define iisfunc_module_ptr &iisfunc_module_entry

BEGIN_EXTERN_C()

PHP_MINIT_FUNCTION(iisfunc);
PHP_MSHUTDOWN_FUNCTION(iisfunc);
PHP_RINIT_FUNCTION(iisfunc);
PHP_RSHUTDOWN_FUNCTION(iisfunc);
PHP_MINFO_FUNCTION(iisfunc);

PHP_FUNCTION(iis_getserverbypath);
PHP_FUNCTION(iis_getserverbycomment);
PHP_FUNCTION(iis_addserver);
PHP_FUNCTION(iis_removeserver);
PHP_FUNCTION(iis_setdirsecurity);
PHP_FUNCTION(iis_getdirsecurity);
PHP_FUNCTION(iis_setserverright);
PHP_FUNCTION(iis_getserverright);
PHP_FUNCTION(iis_startserver);
PHP_FUNCTION(iis_stopserver);
PHP_FUNCTION(iis_setscriptmap);
PHP_FUNCTION(iis_getscriptmap);
PHP_FUNCTION(iis_setappsettings);
PHP_FUNCTION(iis_stopservice);
PHP_FUNCTION(iis_startservice);
PHP_FUNCTION(iis_getservicestate);

END_EXTERN_C()

ZEND_BEGIN_MODULE_GLOBALS(iis)
	void * pIMeta;
ZEND_END_MODULE_GLOBALS(iis)

#ifdef ZTS

int iis_globals_id;
#define IISG(v) TSRMG(iis_globals_id, zend_iis_globals *, v)

#else

#define IISG(v) (iis_globals.v)

#endif


#else

#define iisfunc_module_ptr NULL

#endif /* PHP_WIN32 */

#define phpext_iisfunc_ptr iisfunc_module_ptr

#endif /* PHP_IIS_SETUP_H */
