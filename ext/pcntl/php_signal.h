/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jason Greene <jason@inetgurus.net>                          |
   +----------------------------------------------------------------------+
*/

/* $Id: php_signal.h,v 1.2 2001/07/30 20:51:57 jason Exp $ */

#include <signal.h>
#ifndef PHP_SIGNAL_H
#define PHP_SIGNAL_H

typedef void Sigfunc(int);
Sigfunc *php_signal(int signo, Sigfunc *func);

#endif
