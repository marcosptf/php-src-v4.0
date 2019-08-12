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
   | Authors: Andrei Zmievski <andrei@ispi.net>                           |
   +----------------------------------------------------------------------+
 */

/* $Id: php_wddx.h,v 1.8 2001/06/19 21:25:53 sbergmann Exp $ */

#ifndef PHP_WDDX_H
#define PHP_WDDX_H

#if HAVE_WDDX

#include <expat.h>

extern zend_module_entry wddx_module_entry;
#define wddx_module_ptr &wddx_module_entry

PHP_FUNCTION(wddx_serialize_value);
PHP_FUNCTION(wddx_serialize_vars);
PHP_FUNCTION(wddx_packet_start);
PHP_FUNCTION(wddx_packet_end);
PHP_FUNCTION(wddx_add_vars);
PHP_FUNCTION(wddx_deserialize);

#else

#define wddx_module_ptr NULL

#endif /* HAVE_WDDX */

#define phpext_wddx_ptr wddx_module_ptr

#endif /* !PHP_WDDX_H */
