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
   | Authors:  Jason Greene <jason@inetgurus.net>                         |
   +----------------------------------------------------------------------+
 */

/* $Id: php_pcntl.h,v 1.5 2001/07/30 20:51:57 jason Exp $ */

#ifndef PHP_PCNTL_H
#define PHP_PCNTL_H

#include <sys/wait.h>
#include "php_signal.h"
#include "zend_extensions.h"
extern zend_module_entry pcntl_module_entry;
#define phpext_pcntl_ptr &pcntl_module_entry

#ifdef PHP_WIN32
#define PHP_PCNTL_API __declspec(dllexport)
#else
#define PHP_PCNTL_API
#endif

PHP_MINIT_FUNCTION(pcntl);
PHP_MSHUTDOWN_FUNCTION(pcntl);
PHP_RINIT_FUNCTION(pcntl);
PHP_RSHUTDOWN_FUNCTION(pcntl);
PHP_MINFO_FUNCTION(pcntl);

PHP_FUNCTION(pcntl_fork);	
PHP_FUNCTION(pcntl_waitpid);
PHP_FUNCTION(pcntl_wifexited);
PHP_FUNCTION(pcntl_wifstopped);
PHP_FUNCTION(pcntl_wifsignaled);
PHP_FUNCTION(pcntl_wexitstatus);
PHP_FUNCTION(pcntl_wtermsig);
PHP_FUNCTION(pcntl_wstopsig);
PHP_FUNCTION(pcntl_signal);

static void pcntl_signal_handler(int);

/* Zend extension prototypes */ 
int pcntl_zend_extension_startup(zend_extension *extension);
void pcntl_zend_extension_shutdown(zend_extension *extension);
void pcntl_zend_extension_activate(void);
void pcntl_zend_extension_deactivate(void);
void pcntl_zend_extension_statement_handler(zend_op_array *op_array);


ZEND_BEGIN_MODULE_GLOBALS(pcntl)
	HashTable php_signal_table;
	zend_llist php_signal_queue;
	int signal_queue_ready;
	int processing_signal_queue;
ZEND_END_MODULE_GLOBALS(pcntl)
#ifdef ZTS
#define PCNTL_G(v) TSRMG(pcntl_globals_id, zend_pcntl_globals *, v)
#else
#define PCNTL_G(v)	(pcntl_globals.v)
#endif

#endif	/* PHP_PCNTL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
