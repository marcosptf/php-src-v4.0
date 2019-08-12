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

/* $Id: pcntl.c,v 1.11 2001/07/30 20:51:57 jason Exp $ */

#define PCNTL_DEBUG 0

#if PCNTL_DEBUG
#define DEBUG_OUT printf("DEBUG: ");printf
#else
#define DEBUG_OUT  
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_pcntl.h"
#include "zend_extensions.h"

ZEND_DECLARE_MODULE_GLOBALS(pcntl)

static int le_pcntl;
static int pcntl_zend_extension_active;

function_entry pcntl_functions[] = {
	PHP_FE(pcntl_fork,	NULL)	
	PHP_FE(pcntl_waitpid,	second_arg_force_ref)	
	PHP_FE(pcntl_signal,	NULL)
	PHP_FE(pcntl_wifexited, NULL)
	PHP_FE(pcntl_wifstopped, NULL)
	PHP_FE(pcntl_wifsignaled, NULL)
	PHP_FE(pcntl_wexitstatus, NULL)
	PHP_FE(pcntl_wtermsig, NULL)
	PHP_FE(pcntl_wstopsig, NULL)
	{NULL, NULL, NULL}	
};

zend_module_entry pcntl_module_entry = {
	"pcntl",	pcntl_functions,
	PHP_MINIT(pcntl),
	PHP_MSHUTDOWN(pcntl),
	NULL,
	NULL,
	PHP_MINFO(pcntl),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PCNTL
ZEND_GET_MODULE(pcntl)
#define PCNTL_ZEND_EXT  ZEND_DLEXPORT
#else
#define PCNTL_ZEND_EXT
#endif
  
PCNTL_ZEND_EXT zend_extension pcntl_extension_entry = {
	"pcntl",
	 "1.0",
 	"Jason Greene",
	"http://www.php.net",
	"2001",
 	pcntl_zend_extension_startup,
 	pcntl_zend_extension_shutdown,
	pcntl_zend_extension_activate,
	pcntl_zend_extension_deactivate,
        NULL,
 	NULL,
	pcntl_zend_extension_statement_handler,
        NULL,
 	NULL,
	NULL,
	NULL,
	NULL
};


  
void php_register_signal_constants(INIT_FUNC_ARGS)
{
   
	/* Wait Constants */
#ifdef WNOHANG
	REGISTER_LONG_CONSTANT("WNOHANG",  (long) WNOHANG, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef WUNTRACED
	REGISTER_LONG_CONSTANT("WUNTRACED",  (long) WUNTRACED, CONST_CS | CONST_PERSISTENT);
#endif

	/* Signal Constants */
	REGISTER_LONG_CONSTANT("SIG_IGN",  (long) SIG_IGN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIG_DFL",  (long) SIG_DFL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIG_ERR",  (long) SIG_ERR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGHUP",   (long) SIGHUP,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGINT",   (long) SIGINT,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGQUIT",  (long) SIGQUIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGILL",   (long) SIGILL,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTRAP",  (long) SIGTRAP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGABRT",  (long) SIGABRT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGIOT",   (long) SIGIOT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGBUS",   (long) SIGBUS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGFPE",   (long) SIGFPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGKILL",  (long) SIGKILL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGUSR1",  (long) SIGUSR1, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGSEGV",  (long) SIGSEGV, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGUSR2",  (long) SIGUSR2, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGPIPE",  (long) SIGPIPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGALRM",  (long) SIGALRM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTERM",  (long) SIGTERM, CONST_CS | CONST_PERSISTENT);
#ifdef SIGSTKFLT
	REGISTER_LONG_CONSTANT("SIGSTKFLT",(long) SIGSTKFLT, CONST_CS | CONST_PERSISTENT);
#endif 
#ifdef SIGCLD
	REGISTER_LONG_CONSTANT("SIGCLD",   (long) SIGCLD, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef SIGCHLD
	REGISTER_LONG_CONSTANT("SIGCHLD",  (long) SIGCHLD, CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SIGCONT",  (long) SIGCONT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGSTOP",  (long) SIGSTOP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTSTP",  (long) SIGTSTP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTTIN",  (long) SIGTTIN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGTTOU",  (long) SIGTTOU, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGURG",   (long) SIGURG , CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGXCPU",  (long) SIGXCPU, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGXFSZ",  (long) SIGXFSZ, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGVTALRM",(long) SIGVTALRM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGPROF",  (long) SIGPROF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SIGWINCH", (long) SIGWINCH, CONST_CS | CONST_PERSISTENT);
#ifdef SIGPOLL
	REGISTER_LONG_CONSTANT("SIGPOLL",  (long) SIGPOLL, CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SIGIO",    (long) SIGIO, CONST_CS | CONST_PERSISTENT);
#ifdef SIGPWR
	REGISTER_LONG_CONSTANT("SIGPWR",   (long) SIGPWR, CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SIGSYS",   (long) SIGSYS, CONST_CS | CONST_PERSISTENT);
}

static void php_pcntl_init_globals(zend_pcntl_globals *pcntl_globals) 
{
	zend_hash_init(&pcntl_globals->php_signal_table, 16, NULL, NULL, 1);

	/* Just in case ... */
	memset(&pcntl_globals->php_signal_queue,0,sizeof(pcntl_globals->php_signal_queue));
   
	zend_llist_init(&pcntl_globals->php_signal_queue, sizeof (long),  NULL, 1);
	pcntl_globals->signal_queue_ready=0;
	pcntl_globals->processing_signal_queue=0;
}

static void php_pcntl_shutdown_globals(zend_pcntl_globals *pcntl_globals) 
{
	zend_hash_destroy(&pcntl_globals->php_signal_table);
	zend_llist_destroy(&pcntl_globals->php_signal_queue);
}

PHP_MINIT_FUNCTION(pcntl)
{
   	php_register_signal_constants(INIT_FUNC_ARGS_PASSTHRU);
	ZEND_INIT_MODULE_GLOBALS(pcntl, php_pcntl_init_globals, php_pcntl_shutdown_globals);
	if (zend_register_extension(&pcntl_extension_entry, 0)==FAILURE) 
		return FAILURE;
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(pcntl)
{

	zend_hash_destroy(&PCNTL_G(php_signal_table));
	zend_llist_destroy(&PCNTL_G(php_signal_queue));
	return SUCCESS;
}




PHP_MINFO_FUNCTION(pcntl)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "pcntl support", "enabled");
	php_info_print_table_end();
}

/* {{{ proto long pcntl_fork()
   Forks the currently running process following the same behavior as the UNIX fork() system call*/
PHP_FUNCTION(pcntl_fork)
{
	pid_t id;

	id=fork();
	if (id == -1) {
		php_error(E_ERROR, "Error %d in %s", errno, get_active_function_name(TSRMLS_C));
	}
	
	RETURN_LONG((long) id);
}
/* }}} */

/* {{{ proto long pcntl_waitpid(long pid, long status, long options)
      Waits on or returns the status of a forked child as defined by the waitpid() system call */
PHP_FUNCTION(pcntl_waitpid)
{
	zval **pid, **status, **options;
	int temp_options, temp_status=0;
	pid_t temp_id;

	if (ZEND_NUM_ARGS() > 3 || ZEND_NUM_ARGS() < 2 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &pid, &status, &options) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(pid);
	convert_to_long_ex(options);   
	convert_to_long_ex(status);

        if (ZEND_NUM_ARGS()==2) temp_options=0;
		else temp_options=Z_LVAL_PP(options);
		
	temp_id = waitpid((pid_t) Z_LVAL_PP(pid), &temp_status, temp_options);
	Z_LVAL_PP(status)=temp_status;
	RETURN_LONG((long) temp_id);
}
/* }}} */

/* {{{ proto bool pcntl_wifexited(long status) 
   Returns true if the child status code represents a successful exit */
PHP_FUNCTION(pcntl_wifexited)
{
#ifdef WIFEXITED
	zval **status;
	int status_word;
   
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &status) == FAILURE){
		WRONG_PARAM_COUNT;
	}
   
	status_word = (int) Z_LVAL_PP(status);
	
 	if (WIFEXITED(status_word)) RETURN_TRUE;
#endif
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool pcntl_wifstopped(long status) 
    Returns true if the child status code represents a stopped process (WUNTRACED must have been used with waitpid) */
PHP_FUNCTION(pcntl_wifstopped)
{
#ifdef WIFSTOPPED
	zval **status;
	int status_word;
   
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &status) == FAILURE){
		WRONG_PARAM_COUNT;
	}
   
	status_word = (int) Z_LVAL_PP(status);
	
 	if (WIFSTOPPED(status_word)) RETURN_TRUE;
#endif
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool pcntl_wifsignaled(long status) 
   Returns true if the child status code represents a process that was terminated due to a signal */
PHP_FUNCTION(pcntl_wifsignaled)
{
#ifdef WIFSIGNALED
	zval **status;
	int status_word;
   
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &status) == FAILURE){
		WRONG_PARAM_COUNT;
	}
   
	status_word = (int) Z_LVAL_PP(status);
	
 	if (WIFSIGNALED(status_word)) RETURN_TRUE;
#endif
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto long pcntl_wexitstatus(long status) 
   Returns the status code of a child's exit */
PHP_FUNCTION(pcntl_wexitstatus)
{
#ifdef WEXITSTATUS
	zval **status;
	int status_word;
   
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &status) == FAILURE){
		WRONG_PARAM_COUNT;
	}
   
	status_word = (int) Z_LVAL_PP(status);

	/* WEXITSTATUS only returns 8 bits so we *MUST* cast this to signed char
	   if you want to have valid negative exit codes */
 	RETURN_LONG((signed char) WEXITSTATUS(status_word));
#else
	RETURN_FALSE;
#endif
}
/* }}} */

/* {{{ proto long pcntl_wtermsig(long status) 
   Returns the number of the signal that terminated the process who's status code is passed  */
PHP_FUNCTION(pcntl_wtermsig)
{
#ifdef WTERMSIG
	zval **status;
	int status_word;
   
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &status) == FAILURE){
		WRONG_PARAM_COUNT;
	}
   
	status_word = (int) Z_LVAL_PP(status);

 	RETURN_LONG(WTERMSIG(status_word));
#else
	RETURN_FALSE;
#endif
}
/* }}} */

/* {{{ proto long pcntl_wstopsig(long status) 
   Returns the number of the signal that caused the process to stop who's status code is passed  */
PHP_FUNCTION(pcntl_wstopsig)
{
#ifdef WSTOPSIG
	zval **status;
	int status_word;
   
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &status) == FAILURE){
		WRONG_PARAM_COUNT;
	}
   
	status_word = (int) Z_LVAL_PP(status);

 	RETURN_LONG(WSTOPSIG(status_word));
#else
	RETURN_FALSE;
#endif
}
/* }}} */

/* {{{ proto bool pcntl_signal(long signo, mixed handle)
      Assigns a system signal handler to a php function  */
PHP_FUNCTION(pcntl_signal)
{
	zval **signo, **handle;
   	char *func_name;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &signo, &handle) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(signo);

	/* Special long value case for SIG_DFL and SIG_IGN */
	if (Z_TYPE_PP(handle)==IS_LONG) {
 		if (Z_LVAL_PP(handle)!= (long) SIG_DFL && Z_LVAL_PP(handle) != (long) SIG_IGN) {
 			php_error(E_WARNING, "Invalid value for handle argument specifEied in %s", get_active_function_name(TSRMLS_C));
		}
		if (php_signal(Z_LVAL_PP(signo), (Sigfunc *) Z_LVAL_PP(handle))==SIG_ERR) {
			php_error(E_WARNING, "Error assigning singal in %s", get_active_function_name(TSRMLS_C));
		   	RETURN_FALSE;
		}
		RETURN_TRUE;
	} 
	
        if (Z_TYPE_PP(handle)!=IS_STRING) {
		php_error(E_WARNING, "Invalid type specified for handle argument in %s", get_active_function_name(TSRMLS_C));
		RETURN_FALSE;
	}

	convert_to_string_ex(handle); /* Just in case */
	if (!zend_is_callable(*handle, 0, &func_name)) {
		php_error(E_WARNING, "%s: %s is not a callable function name error", get_active_function_name(TSRMLS_C), func_name);
	   	efree(func_name);
		RETURN_FALSE;
	}
   	efree(func_name);

        /* Add the function name to our signal table */
	zend_hash_index_update(&PCNTL_G(php_signal_table), Z_LVAL_PP(signo), Z_STRVAL_PP(handle),  (Z_STRLEN_PP(handle) + 1) * sizeof(char), NULL);
	
	if (php_signal(Z_LVAL_PP(signo), pcntl_signal_handler)==SIG_ERR) {
 			php_error(E_WARNING, "Error assigning singal in %s", get_active_function_name(TSRMLS_C));
		   	RETURN_FALSE;
	}
	RETURN_TRUE;	
}
/* }}} */

/* Note Old */
static void old_pcntl_signal_handler(int signo)
{
	char *func_name;
	zval *param, *call_name, *retval;
	TSRMLS_FETCH();

	DEBUG_OUT("Caught signal: %d\n", signo); 
	if (zend_hash_index_find(&PCNTL_G(php_signal_table), (long) signo, (void *) &func_name)==FAILURE) {
		DEBUG_OUT("Signl handler not fount"); 
		return;
	}
	/* DEBUG_OUT("Signal handler found, Calling %s\n", func_name); */	
	MAKE_STD_ZVAL(param);
	MAKE_STD_ZVAL(call_name);
	MAKE_STD_ZVAL(retval);
	ZVAL_LONG(param, signo);
 	ZVAL_STRING(call_name, func_name, 1);

	/* Call php singal handler - Note that we do not report errors, and we ignore the return value */ 
	call_user_function(EG(function_table), NULL, call_name, retval, 1, &param TSRMLS_CC);
	
 	zval_dtor(call_name);
	efree(call_name);
	efree(param);  
	efree(retval);

	return;
}

/* Our custom signal handler that calls the appropriate php_function */
static void pcntl_signal_handler(int signo)
{
	long signal_num=signo;
	TSRMLS_FETCH();
 
	DEBUG_OUT("Caught signo %d\n", signo); 
	if (! PCNTL_G(processing_signal_queue) && pcntl_zend_extension_active ) {
		zend_llist_add_element(&PCNTL_G(php_signal_queue), &signal_num);
		PCNTL_G(signal_queue_ready)=1;
		DEBUG_OUT("Added queue entry\n"); 
	}
	return;
}
   

/* Pcntl Zend Extension Hooks */

int pcntl_zend_extension_startup(zend_extension *extension)
{
	TSRMLS_FETCH();
   
	DEBUG_OUT("Statup Called\n");
	pcntl_zend_extension_active=1;
	CG(extended_info) = 1;
	return SUCCESS;
}

void pcntl_zend_extension_shutdown(zend_extension *extension)
{
	DEBUG_OUT("Shutdown Called\n");
	return;
}

void pcntl_zend_extension_activate(void)
{
	TSRMLS_FETCH();
   
	DEBUG_OUT("Activate Called\n");
	pcntl_zend_extension_active=1;
	CG(extended_info) = 1;
	return;
}

void pcntl_zend_extension_deactivate(void)
{
	DEBUG_OUT("Deactivate Called\n");
   	pcntl_zend_extension_active=0;
	return;
}

/* Custom hook to ensure signals only get called at a safe poing in Zend's execute process */
void pcntl_zend_extension_statement_handler(zend_op_array *op_array)
{
	zend_llist_element *element;
	zval *param, *call_name, *retval;
	char *func_name;
	TSRMLS_FETCH();

	/* Bail if the queue is empty or if we are already playing the queue*/
	if (! PCNTL_G(signal_queue_ready) || PCNTL_G(processing_signal_queue))
		return;

   	/* Mark our queue empty */
	PCNTL_G(signal_queue_ready)=0;
   
	/* If for some reason our signal queue is empty then return */
	if (zend_llist_count(&PCNTL_G(php_signal_queue)) <= 0) {
		return;
	}
       
	/* Disable queue so this function is not infinate */
	PCNTL_G(processing_signal_queue)=1;	
   
	/* Allocate */
	MAKE_STD_ZVAL(param);
	MAKE_STD_ZVAL(call_name);
	MAKE_STD_ZVAL(retval);

	/* Traverse through our signal queue and call the appropriate php functions */
	for (element=(&PCNTL_G(php_signal_queue))->head; element; element=element->next) {
		long *signal_num=(long *)&element->data;
		if (zend_hash_index_find(&PCNTL_G(php_signal_table), *signal_num, (void *) &func_name)==FAILURE) {
			continue;
		}
		convert_to_long_ex(&param);
		convert_to_string_ex(&call_name);
		ZVAL_LONG(param, *signal_num);
		ZVAL_STRING(call_name, func_name, 0);
	   
		/* Call php singal handler - Note that we do not report errors, and we ignore the return value */ 
		call_user_function(EG(function_table), NULL, call_name, retval, 1, &param TSRMLS_CC);
	}
	/* Clear */
	zend_llist_clean(&PCNTL_G(php_signal_queue));

	/* Re-enable queue */
	PCNTL_G(processing_signal_queue)=0;

	/* Clean up */
	efree(param);
	efree(call_name);
	efree(retval);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
