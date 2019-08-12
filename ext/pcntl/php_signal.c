
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

/* $Id: php_signal.c,v 1.2 2001/07/30 20:51:57 jason Exp $ */

#include "php_signal.h"

/* php_signal using sigaction is taken verbatim from Advanced Programing
 * in the Unix Environment by W. Richard Stevens p 298. */
Sigfunc *php_signal(int signo, Sigfunc *func)
{
 
	struct sigaction act,oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART; /* SVR4, 4.3+BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;
 
	return oact.sa_handler;
}

