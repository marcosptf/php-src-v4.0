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

/* $Id: php_output.h,v 1.30 2001/08/05 15:55:43 sas Exp $ */

#ifndef PHP_OUTPUT_H
#define PHP_OUTPUT_H

#include "php.h"

typedef void (*php_output_handler_func_t)(char *output, uint output_len, char **handled_output, uint *handled_output_len, int mode TSRMLS_DC);

PHPAPI void php_output_startup(void);
PHPAPI void php_output_activate(TSRMLS_D);
PHPAPI void php_output_set_status(zend_bool status TSRMLS_DC);
void php_output_register_constants(TSRMLS_D);
PHPAPI int  php_body_write(const char *str, uint str_length TSRMLS_DC);
PHPAPI int  php_header_write(const char *str, uint str_length TSRMLS_DC);
PHPAPI int php_start_ob_buffer(zval *output_handler, uint chunk_size TSRMLS_DC);
PHPAPI void php_end_ob_buffer(zend_bool send_buffer, zend_bool just_flush TSRMLS_DC);
PHPAPI void php_end_ob_buffers(zend_bool send_buffer TSRMLS_DC);
PHPAPI int php_ob_get_buffer(zval *p TSRMLS_DC);
PHPAPI int php_ob_get_length(zval *p TSRMLS_DC);
PHPAPI void php_start_implicit_flush(TSRMLS_D);
PHPAPI void php_end_implicit_flush(TSRMLS_D);
PHPAPI char *php_get_output_start_filename(TSRMLS_D);
PHPAPI int php_get_output_start_lineno(TSRMLS_D);
PHPAPI void php_ob_set_internal_handler(php_output_handler_func_t internal_output_handler, uint buffer_size TSRMLS_DC);

PHP_FUNCTION(ob_start);
PHP_FUNCTION(ob_end_flush);
PHP_FUNCTION(ob_end_clean);
PHP_FUNCTION(ob_get_contents);
PHP_FUNCTION(ob_get_length);
PHP_FUNCTION(ob_implicit_flush);

typedef struct _php_ob_buffer {
	char *buffer;
	uint size;
	uint text_length;
	int block_size;
	uint chunk_size;
	int status;
	zval *output_handler;
	php_output_handler_func_t internal_output_handler;
	char *internal_output_handler_buffer;
	uint internal_output_handler_buffer_size;
} php_ob_buffer;

typedef struct _php_output_globals {
	int (*php_body_write)(const char *str, uint str_length TSRMLS_DC);		/* string output */
	int (*php_header_write)(const char *str, uint str_length TSRMLS_DC);	/* unbuffer string output */
	php_ob_buffer active_ob_buffer;
	unsigned char implicit_flush;
	char *output_start_filename;
	int output_start_lineno;
	zend_stack ob_buffers;
	int ob_nesting_level;
	zend_bool ob_lock;
	zend_bool disable_output;
} php_output_globals;


#ifdef ZTS
#define OG(v) TSRMG(output_globals_id, php_output_globals *, v)
ZEND_API extern int output_globals_id;
#else
#define OG(v) (output_globals.v)
ZEND_API extern php_output_globals output_globals;
#endif

#define PHP_OUTPUT_HANDLER_START		(1<<0)
#define PHP_OUTPUT_HANDLER_CONT			(1<<1)
#define PHP_OUTPUT_HANDLER_END			(1<<2)

#endif /* PHP_OUTPUT_H */
