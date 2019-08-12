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
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: php_browscap.h,v 1.7 2001/07/30 01:56:37 zeev Exp $ */

#ifndef PHP_BROWSCAP_H
#define PHP_BROWSCAP_H

PHP_MINIT_FUNCTION(browscap);
PHP_MSHUTDOWN_FUNCTION(browscap);

PHP_FUNCTION(get_browser);

#endif /* PHP_BROWSCAP_H */
