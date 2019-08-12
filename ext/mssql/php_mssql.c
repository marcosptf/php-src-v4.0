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
   | Authors: Frank M. Kromann <frank@frontbase.com>                      |
   +----------------------------------------------------------------------+
 */

/* $Id: php_mssql.c,v 1.67 2001/08/13 16:13:17 andi Exp $ */

#ifdef COMPILE_DL_MSSQL
#define HAVE_MSSQL 1
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#include "php_mssql.h"
#include "php_ini.h"

#if HAVE_MSSQL
#define SAFE_STRING(s) ((s)?(s):"")

#define MSSQL_ASSOC		1<<0
#define MSSQL_NUM		1<<1
#define MSSQL_BOTH		(MSSQL_ASSOC|MSSQL_NUM)

static int le_result, le_link, le_plink, le_statement;

static void php_mssql_get_column_content_with_type(mssql_link *mssql_ptr,int offset,zval *result, int column_type);
static void php_mssql_get_column_content_without_type(mssql_link *mssql_ptr,int offset,zval *result, int column_type);

static void _mssql_bind_hash_dtor(void *data);
static unsigned char a3_arg_force_ref[] = { 3, BYREF_NONE, BYREF_NONE, BYREF_FORCE };

function_entry mssql_functions[] = {
	PHP_FE(mssql_connect,				NULL)
	PHP_FE(mssql_pconnect,				NULL)
	PHP_FE(mssql_close,					NULL)
	PHP_FE(mssql_select_db,				NULL)
	PHP_FE(mssql_query,					NULL)
	PHP_FE(mssql_fetch_batch,			NULL)
	PHP_FE(mssql_rows_affected,			NULL)
	PHP_FE(mssql_free_result,			NULL)
	PHP_FE(mssql_get_last_message,		NULL)
	PHP_FE(mssql_num_rows,				NULL)
	PHP_FE(mssql_num_fields,			NULL)
	PHP_FE(mssql_fetch_field,			NULL)
	PHP_FE(mssql_fetch_row,				NULL)
	PHP_FE(mssql_fetch_array,			NULL)
	PHP_FE(mssql_fetch_object,			NULL)
	PHP_FE(mssql_field_length,			NULL)
	PHP_FE(mssql_field_name,			NULL)
	PHP_FE(mssql_field_type,			NULL)
	PHP_FE(mssql_data_seek,				NULL)
	PHP_FE(mssql_field_seek,			NULL)
	PHP_FE(mssql_result,				NULL)
	PHP_FE(mssql_next_result,			NULL)
	PHP_FE(mssql_min_error_severity,	NULL)
	PHP_FE(mssql_min_message_severity,	NULL)
 	PHP_FE(mssql_init,					NULL)
 	PHP_FE(mssql_bind,					a3_arg_force_ref)
 	PHP_FE(mssql_execute,				NULL)
 	PHP_FE(mssql_guid_string,			NULL)
	{NULL, NULL, NULL}
};

zend_module_entry mssql_module_entry = 
{
	"mssql", 
	mssql_functions, 
	PHP_MINIT(mssql), 
	PHP_MSHUTDOWN(mssql), 
	PHP_RINIT(mssql), 
	PHP_RSHUTDOWN(mssql), 
	PHP_MINFO(mssql), 
	STANDARD_MODULE_PROPERTIES
};

ZEND_DECLARE_MODULE_GLOBALS(mssql)

#ifdef COMPILE_DL_MSSQL
ZEND_GET_MODULE(mssql)
#endif

#define CHECK_LINK(link) { if (link==-1) { php_error(E_WARNING,"MS SQL:  A link to the server could not be established"); RETURN_FALSE; } }

static PHP_INI_DISP(display_text_size)
{
	char *value;
	TSRMLS_FETCH();
	
    if (type == PHP_INI_DISPLAY_ORIG && ini_entry->modified) {
		value = ini_entry->orig_value;
	} else if (ini_entry->value) {
		value = ini_entry->value;
	} else {
		value = NULL;
	}

	if (atoi(value) == -1) {
		PUTS("Server default");
	} else {
		php_printf("%s", value);
	}
}

PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("mssql.allow_persistent",		"1",	PHP_INI_SYSTEM,	OnUpdateBool,	allow_persistent,			zend_mssql_globals,		mssql_globals)
	STD_PHP_INI_ENTRY_EX("mssql.max_persistent",		"-1",	PHP_INI_SYSTEM,	OnUpdateInt,	max_persistent,				zend_mssql_globals,		mssql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY_EX("mssql.max_links",				"-1",	PHP_INI_SYSTEM,	OnUpdateInt,	max_links,					zend_mssql_globals,		mssql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY_EX("mssql.min_error_severity",	"10",	PHP_INI_ALL,	OnUpdateInt,	cfg_min_error_severity,		zend_mssql_globals,		mssql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY_EX("mssql.min_message_severity",	"10",	PHP_INI_ALL,	OnUpdateInt,	cfg_min_message_severity,	zend_mssql_globals,		mssql_globals,	display_link_numbers)
	STD_PHP_INI_BOOLEAN("mssql.compatability_mode",		"0",	PHP_INI_ALL,	OnUpdateBool,	compatability_mode,			zend_mssql_globals,		mssql_globals)
	STD_PHP_INI_ENTRY_EX("mssql.connect_timeout",    	"5",	PHP_INI_ALL,	OnUpdateInt,	connect_timeout,			zend_mssql_globals,		mssql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY_EX("mssql.timeout",      			"60",	PHP_INI_ALL,	OnUpdateInt,	timeout,					zend_mssql_globals,		mssql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY_EX("mssql.textsize",   			"-1",	PHP_INI_ALL,	OnUpdateInt,	textsize,					zend_mssql_globals,		mssql_globals,	display_text_size)
	STD_PHP_INI_ENTRY_EX("mssql.textlimit",   			"-1",	PHP_INI_ALL,	OnUpdateInt,	textlimit,					zend_mssql_globals,		mssql_globals,	display_text_size)
	STD_PHP_INI_ENTRY_EX("mssql.batchsize",   			"0",	PHP_INI_ALL,	OnUpdateInt,	batchsize,					zend_mssql_globals,		mssql_globals,	display_link_numbers)
PHP_INI_END()

/* error handler */
static int php_mssql_error_handler(DBPROCESS *dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	TSRMLS_FETCH();

	if (severity >= MS_SQL_G(min_error_severity)) {
		php_error(E_WARNING,"MS SQL error:  %s (severity %d)", dberrstr, severity);
	}
	return INT_CANCEL;  
}

/* message handler */
static int php_mssql_message_handler(DBPROCESS *dbproc, DBINT msgno,int msgstate, int severity,char *msgtext,char *srvname, char *procname,DBUSMALLINT line)
{
	TSRMLS_FETCH();

	if (severity >= MS_SQL_G(min_message_severity)) {
		php_error(E_WARNING,"MS SQL message:  %s (severity %d)", msgtext, severity);
	}
	STR_FREE(MS_SQL_G(server_message));
	MS_SQL_G(server_message) = estrdup(msgtext);
	return 0;
}

static int _clean_invalid_results(list_entry *le TSRMLS_DC)
{
	if (le->type == le_result) {
		mssql_link *mssql_ptr = ((mssql_result *) le->ptr)->mssql_ptr;
		
		if (!mssql_ptr->valid) {
			return 1;
		}
	}
	return 0;
}

static void _free_result(mssql_result *result, int free_fields) 
{
	int i,j;

	if (result->data) {
		for (i=0; i<result->num_rows; i++) {
			if (result->data[i]) {
				for (j=0; j<result->num_fields; j++) {
					zval_dtor(&result->data[i][j]);
				}
				efree(result->data[i]);
			}
		}
		efree(result->data);
		result->data = NULL;
		result->blocks_initialized = 0;
	}
	
	if (free_fields && result->fields) {
		for (i=0; i<result->num_fields; i++) {
			STR_FREE(result->fields[i].name);
			STR_FREE(result->fields[i].column_source);
		}
		efree(result->fields);
	}
}

static void _free_mssql_statement(mssql_statement *statement)
{
	if (statement->binds) {
		zend_hash_destroy(statement->binds);
		efree(statement->binds);
	}
	
	efree(statement);
}

static void _free_mssql_result(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	mssql_result *result = (mssql_result *)rsrc->ptr;

	_free_result(result, 1);
	efree(result);
}

static void php_mssql_set_default_link(int id TSRMLS_DC)
{
	if (MS_SQL_G(default_link)!=-1) {
		zend_list_delete(MS_SQL_G(default_link));
	}
	MS_SQL_G(default_link) = id;
	zend_list_addref(id);
}

static void _close_mssql_link(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	mssql_link *mssql_ptr = (mssql_link *)rsrc->ptr;

	mssql_ptr->valid = 0;
	zend_hash_apply(&EG(regular_list),(apply_func_t) _clean_invalid_results TSRMLS_CC);
	dbclose(mssql_ptr->link);
	dbfreelogin(mssql_ptr->login);
	efree(mssql_ptr);
	MS_SQL_G(num_links)--;
}


static void _close_mssql_plink(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	mssql_link *mssql_ptr = (mssql_link *)rsrc->ptr;

	dbclose(mssql_ptr->link);
	dbfreelogin(mssql_ptr->login);
	free(mssql_ptr);
	MS_SQL_G(num_persistent)--;
	MS_SQL_G(num_links)--;
}

static void _mssql_bind_hash_dtor(void *data)
{
	mssql_bind *bind= (mssql_bind *) data;

   	zval_ptr_dtor(&(bind->zval));
}

static void php_mssql_init_globals(zend_mssql_globals *mssql_globals)
{
	long compatability_mode;

	mssql_globals->num_persistent = 0;
	if (cfg_get_long("mssql.compatability_mode", &compatability_mode) == SUCCESS) {
		if (compatability_mode) {
			mssql_globals->get_column_content = php_mssql_get_column_content_without_type;	
		} else {
			mssql_globals->get_column_content = php_mssql_get_column_content_with_type;
		}
	}
}

PHP_MINIT_FUNCTION(mssql)
{
	ZEND_INIT_MODULE_GLOBALS(mssql, php_mssql_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	le_statement = register_list_destructors(_free_mssql_statement, NULL);
	le_result = zend_register_list_destructors_ex(_free_mssql_result, NULL, "mssql result", module_number);
	le_link = zend_register_list_destructors_ex(_close_mssql_link, NULL, "mssql link", module_number);
	le_plink = zend_register_list_destructors_ex(NULL, _close_mssql_plink, "mssql link persistent", module_number);
	mssql_module_entry.type = type;

	if (dbinit()==FAIL) {
		return FAILURE;
	}

	/* BEGIN MSSQL data types for mssql_bind */
	REGISTER_LONG_CONSTANT("MSSQL_ASSOC", MSSQL_ASSOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSSQL_NUM", MSSQL_NUM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSSQL_BOTH", MSSQL_BOTH, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SQLTEXT",SQLTEXT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLVARCHAR",SQLVARCHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLCHAR",SQLCHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLINT1",SQLINT1, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLINT2",SQLINT2, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLINT4",SQLINT4, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLBIT",SQLBIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLFLT8",SQLFLT8, CONST_CS | CONST_PERSISTENT);
	/* END MSSQL data types for mssql_sp_bind */

	dberrhandle((DBERRHANDLE_PROC) php_mssql_error_handler);
	dbmsghandle((DBMSGHANDLE_PROC) php_mssql_message_handler);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(mssql)
{
	UNREGISTER_INI_ENTRIES();
	dbexit();
	return SUCCESS;
}

PHP_RINIT_FUNCTION(mssql)
{
	MS_SQL_G(default_link) = -1;
	MS_SQL_G(num_links) = MS_SQL_G(num_persistent);
	MS_SQL_G(appname) = estrndup("PHP 4.0",7);
	MS_SQL_G(server_message) = empty_string;
	MS_SQL_G(min_error_severity) = MS_SQL_G(cfg_min_error_severity);
	MS_SQL_G(min_message_severity) = MS_SQL_G(cfg_min_message_severity);
	if (MS_SQL_G(connect_timeout) < 1) MS_SQL_G(connect_timeout) = 1;
	dbsetlogintime(MS_SQL_G(connect_timeout));
	if (MS_SQL_G(timeout) < 0) MS_SQL_G(timeout) = 60;
	dbsettime(MS_SQL_G(timeout));

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(mssql)
{
	STR_FREE(MS_SQL_G(appname));
	if (MS_SQL_G(server_message)) {
		STR_FREE(MS_SQL_G(server_message));
	}
	return SUCCESS;
}

PHP_MINFO_FUNCTION(mssql)
{
	char buf[32];

	php_info_print_table_start();
	php_info_print_table_header(2, "MSSQL Support", "enabled");

	sprintf(buf, "%ld", MS_SQL_G(num_persistent));
	php_info_print_table_row(2, "Active Persistent Links", buf);
	sprintf(buf, "%ld", MS_SQL_G(num_links));
	php_info_print_table_row(2, "Active Links", buf);

	php_info_print_table_row(2, "Library version", MSSQL_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();

}

static void php_mssql_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
	char *user, *passwd, *host;
	char *hashed_details;
	int hashed_details_length;
	mssql_link mssql, *mssql_ptr;
	char buffer[32];

	switch(ZEND_NUM_ARGS()) {
		case 0: /* defaults */
			host=user=passwd=NULL;
			hashed_details_length=5+3;
			hashed_details = (char *) emalloc(hashed_details_length+1);
			strcpy(hashed_details,"mssql___");
			break;
		case 1: {
				zval **yyhost;
				
				if (zend_get_parameters_ex(1, &yyhost)==FAILURE) {
					WRONG_PARAM_COUNT;
				}
				convert_to_string_ex(yyhost);
				host = (*yyhost)->value.str.val;
				user=passwd=NULL;
				hashed_details_length = (*yyhost)->value.str.len+5+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"mssql_%s__",(*yyhost)->value.str.val);
			}
			break;
		case 2: {
				zval **yyhost,**yyuser;
				
				if (zend_get_parameters_ex(2, &yyhost, &yyuser)==FAILURE) {
					WRONG_PARAM_COUNT;
				}
				convert_to_string_ex(yyhost);
				convert_to_string_ex(yyuser);
				host = (*yyhost)->value.str.val;
				user = (*yyuser)->value.str.val;
				passwd=NULL;
				hashed_details_length = (*yyhost)->value.str.len+(*yyuser)->value.str.len+5+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"mssql_%s_%s_",(*yyhost)->value.str.val,(*yyuser)->value.str.val);
			}
			break;
		case 3: {
				zval **yyhost,**yyuser,**yypasswd;
			
				if (zend_get_parameters_ex(3, &yyhost, &yyuser, &yypasswd) == FAILURE) {
					WRONG_PARAM_COUNT;
				}
				convert_to_string_ex(yyhost);
				convert_to_string_ex(yyuser);
				convert_to_string_ex(yypasswd);
				host = (*yyhost)->value.str.val;
				user = (*yyuser)->value.str.val;
				passwd = (*yypasswd)->value.str.val;
				hashed_details_length = (*yyhost)->value.str.len+(*yyuser)->value.str.len+(*yypasswd)->value.str.len+5+3;
				hashed_details = (char *) emalloc(hashed_details_length+1);
				sprintf(hashed_details,"mssql_%s_%s_%s",(*yyhost)->value.str.val,(*yyuser)->value.str.val,(*yypasswd)->value.str.val); /* SAFE */
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	if (hashed_details == NULL) {
		php_error(E_WARNING, "Out of memory");
		RETURN_FALSE;
	}

	/* set a DBLOGIN record */	
	if ((mssql.login = dblogin()) == NULL) {
		php_error(E_WARNING,"MS SQL:  Unable to allocate login record");
		RETURN_FALSE;
	}
	
	if (user) {
		DBSETLUSER(mssql.login,user);
	}
	if (passwd) {
		DBSETLPWD(mssql.login,passwd);
	}
	DBSETLAPP(mssql.login,MS_SQL_G(appname));
	mssql.valid = 1;

	DBSETLVERSION(mssql.login, DBVER60);
/*	DBSETLTIME(mssql.login, TIMEOUT_INFINITE); */

	if (!MS_SQL_G(allow_persistent)) {
		persistent=0;
	}
	if (persistent) {
		list_entry *le;

		/* try to find if we already have this link in our persistent list */
		if (zend_hash_find(&EG(persistent_list), hashed_details, hashed_details_length + 1, (void **) &le)==FAILURE) {  /* we don't */
			list_entry new_le;

			if (MS_SQL_G(max_links) != -1 && MS_SQL_G(num_links) >= MS_SQL_G(max_links)) {
				php_error(E_WARNING,"MS SQL:  Too many open links (%d)",MS_SQL_G(num_links));
				efree(hashed_details);
				dbfreelogin(mssql.login);
				RETURN_FALSE;
			}
			if (MS_SQL_G(max_persistent) != -1 && MS_SQL_G(num_persistent) >= MS_SQL_G(max_persistent)) {
				php_error(E_WARNING,"MS SQL:  Too many open persistent links (%d)",MS_SQL_G(num_persistent));
				efree(hashed_details);
				dbfreelogin(mssql.login);
				RETURN_FALSE;
			}
			/* create the link */
			if ((mssql.link = dbopen(mssql.login, host)) == FAIL) {
				php_error(E_WARNING,"MS SQL:  Unable to connect to server:  %s", host);
				efree(hashed_details);
				dbfreelogin(mssql.login);
				RETURN_FALSE;
			}

			if (dbsetopt(mssql.link, DBBUFFER, "2")==FAIL) {
				efree(hashed_details);
				dbfreelogin(mssql.login);
				dbclose(mssql.link);
				RETURN_FALSE;
			}

			if (MS_SQL_G(textlimit) != -1) {
				sprintf(buffer, "%li", MS_SQL_G(textlimit));
				if (dbsetopt(mssql.link, DBTEXTLIMIT, buffer)==FAIL) {
					efree(hashed_details);
					dbfreelogin(mssql.login);
					RETURN_FALSE;
				}
			}
			if (MS_SQL_G(textsize) != -1) {
				sprintf(buffer, "SET TEXTSIZE %li", MS_SQL_G(textsize));
				dbcmd(mssql.link, buffer);
				dbsqlexec(mssql.link);
				dbresults(mssql.link);
			}

			/* hash it up */
			mssql_ptr = (mssql_link *) malloc(sizeof(mssql_link));
			memcpy(mssql_ptr, &mssql, sizeof(mssql_link));
			new_le.type = le_plink;
			new_le.ptr = mssql_ptr;
			if (zend_hash_update(&EG(persistent_list), hashed_details, hashed_details_length + 1, &new_le, sizeof(list_entry), NULL)==FAILURE) {
				free(mssql_ptr);
				efree(hashed_details);
				dbfreelogin(mssql.login);
				RETURN_FALSE;
			}
			MS_SQL_G(num_persistent)++;
			MS_SQL_G(num_links)++;
		} else {  /* we do */
			if (le->type != le_plink) {
#if BROKEN_MSSQL_PCONNECTS
				log_error("PHP/MS SQL:  Hashed persistent link is not a MS SQL link!",php_rqst->server);
#endif
				php_error(E_WARNING,"MS SQL:  Hashed persistent link is not a MS SQL link!");
				RETURN_FALSE;
			}
			
			mssql_ptr = (mssql_link *) le->ptr;
			/* test that the link hasn't died */
			if (DBDEAD(mssql_ptr->link) == TRUE) {
#if BROKEN_MSSQL_PCONNECTS
				log_error("PHP/MS SQL:  Persistent link died, trying to reconnect...",php_rqst->server);
#endif
				if ((mssql_ptr->link=dbopen(mssql_ptr->login,host))==FAIL) {
#if BROKEN_MSSQL_PCONNECTS
					log_error("PHP/MS SQL:  Unable to reconnect!",php_rqst->server);
#endif
					php_error(E_WARNING,"MS SQL:  Link to server lost, unable to reconnect");
					zend_hash_del(&EG(persistent_list), hashed_details, hashed_details_length+1);
					efree(hashed_details);
					RETURN_FALSE;
				}
#if BROKEN_MSSQL_PCONNECTS
				log_error("PHP/MS SQL:  Reconnect successful!",php_rqst->server);
#endif
				if (dbsetopt(mssql_ptr->link, DBBUFFER, "2")==FAIL) {
#if BROKEN_MSSQL_PCONNECTS
					log_error("PHP/MS SQL:  Unable to set required options",php_rqst->server);
#endif
					zend_hash_del(&EG(persistent_list), hashed_details, hashed_details_length + 1);
					efree(hashed_details);
					RETURN_FALSE;
				}
			}
		}
		ZEND_REGISTER_RESOURCE(return_value, mssql_ptr, le_plink);
	} else { /* non persistent */
		list_entry *index_ptr, new_index_ptr;
		
		/* first we check the hash for the hashed_details key.  if it exists,
		 * it should point us to the right offset where the actual mssql link sits.
		 * if it doesn't, open a new mssql link, add it to the resource list,
		 * and add a pointer to it with hashed_details as the key.
		 */
		if (zend_hash_find(&EG(regular_list), hashed_details, hashed_details_length + 1,(void **) &index_ptr)==SUCCESS) {
			int type,link;
			void *ptr;

			if (index_ptr->type != le_index_ptr) {
				RETURN_FALSE;
			}
			link = (int) index_ptr->ptr;
			ptr = zend_list_find(link,&type);   /* check if the link is still there */
			if (ptr && (type==le_link || type==le_plink)) {
				zend_list_addref(link);
				return_value->value.lval = link;
				php_mssql_set_default_link(link TSRMLS_CC);
				return_value->type = IS_RESOURCE;
				efree(hashed_details);
				return;
			} else {
				zend_hash_del(&EG(regular_list), hashed_details, hashed_details_length + 1);
			}
		}
		if (MS_SQL_G(max_links) != -1 && MS_SQL_G(num_links) >= MS_SQL_G(max_links)) {
			php_error(E_WARNING,"MS SQL:  Too many open links (%d)",MS_SQL_G(num_links));
			efree(hashed_details);
			RETURN_FALSE;
		}
		
		if ((mssql.link=dbopen(mssql.login, host))==NULL) {
			php_error(E_WARNING,"MS SQL:  Unable to connect to server:  %s", host);
			efree(hashed_details);
			RETURN_FALSE;
		}

		if (dbsetopt(mssql.link, DBBUFFER,"2")==FAIL) {
			efree(hashed_details);
			dbfreelogin(mssql.login);
			dbclose(mssql.link);
			RETURN_FALSE;
		}

		if (MS_SQL_G(textlimit) != -1) {
			sprintf(buffer, "%li", MS_SQL_G(textlimit));
			if (dbsetopt(mssql.link, DBTEXTLIMIT, buffer)==FAIL) {
				efree(hashed_details);
				dbfreelogin(mssql.login);
				RETURN_FALSE;
			}
		}
		if (MS_SQL_G(textsize) != -1) {
			sprintf(buffer, "SET TEXTSIZE %li", MS_SQL_G(textsize));
			dbcmd(mssql.link, buffer);
			dbsqlexec(mssql.link);
			dbresults(mssql.link);
		}

		/* add it to the list */
		mssql_ptr = (mssql_link *) emalloc(sizeof(mssql_link));
		memcpy(mssql_ptr, &mssql, sizeof(mssql_link));
		ZEND_REGISTER_RESOURCE(return_value, mssql_ptr, le_link);
		
		/* add it to the hash */
		new_index_ptr.ptr = (void *) return_value->value.lval;
		new_index_ptr.type = le_index_ptr;
		if (zend_hash_update(&EG(regular_list), hashed_details, hashed_details_length + 1,(void *) &new_index_ptr, sizeof(list_entry),NULL)==FAILURE) {
			efree(hashed_details);
			RETURN_FALSE;
		}
		MS_SQL_G(num_links)++;
	}
	efree(hashed_details);
	php_mssql_set_default_link(return_value->value.lval TSRMLS_CC);
}


static int php_mssql_get_default_link(INTERNAL_FUNCTION_PARAMETERS)
{
	if (MS_SQL_G(default_link)==-1) { /* no link opened yet, implicitly open one */
		ht = 0;
		php_mssql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
	}
	return MS_SQL_G(default_link);
}

/* {{{ proto int mssql_connect([string servername [, string username [, string password]]])
   Establishes a connection to a MS-SQL server */
PHP_FUNCTION(mssql_connect)
{
	php_mssql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}

/* }}} */

/* {{{ proto int mssql_pconnect([string servername [, string username [, string password]]])
   Establishes a persistent connection to a MS-SQL server */
PHP_FUNCTION(mssql_pconnect)
{
	php_mssql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}

/* }}} */

/* {{{ proto int mssql_close([int connectionid])
   Closes a connection to a MS-SQL server */
PHP_FUNCTION(mssql_close)
{
	zval **mssql_link_index=NULL;
	int id;
	mssql_link *mssql_ptr;
	
	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_mssql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &mssql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(mssql_ptr, mssql_link *, mssql_link_index, id, "MS SQL-Link", le_link, le_plink);

	if (mssql_link_index) 
		zend_list_delete((*mssql_link_index)->value.lval);
	else 
		zend_list_delete(id);

	RETURN_TRUE;
}

/* }}} */

/* {{{ proto bool mssql_select_db(string database_name [, int conn_id])
   Select a MS-SQL database */
PHP_FUNCTION(mssql_select_db)
{
	zval **db, **mssql_link_index;
	int id;
	mssql_link  *mssql_ptr;
	
	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &db)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mssql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
			CHECK_LINK(id);
			break;
		case 2:
			if (zend_get_parameters_ex(2, &db, &mssql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	ZEND_FETCH_RESOURCE2(mssql_ptr, mssql_link *, mssql_link_index, id, "MS SQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	
	if (dbuse(mssql_ptr->link, (*db)->value.str.val)==FAIL) {
		php_error(E_WARNING,"MS SQL:  Unable to select database:  %s", (*db)->value.str.val);
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}

/* }}} */

static void php_mssql_get_column_content_with_type(mssql_link *mssql_ptr,int offset,zval *result, int column_type)
{
	if (dbdatlen(mssql_ptr->link,offset) == 0) {
		ZVAL_NULL(result);
		return;
	}

	switch (column_type)
	{
		case SQLINT1:
		case SQLINT2:
		case SQLINT4:
		case SQLINTN: {	
			result->value.lval = (long) anyintcol(offset);
			result->type = IS_LONG;
			break;
		} 
		case SQLCHAR:
		case SQLVARCHAR:
		case SQLTEXT: {
			int length;
			char *data = charcol(offset);

			length=dbdatlen(mssql_ptr->link,offset);
			while (length>0 && data[length-1] == ' ') { /* nuke trailing whitespace */
				length--;
			}
			result->value.str.val = estrndup(data,length);
			result->value.str.len = length;
			result->type = IS_STRING;
			break;
		}
		case SQLFLT8: {
			result->value.dval = (double) floatcol(offset);
			result->type = IS_DOUBLE;
			break;
		}
		case SQLVARBINARY:
		case SQLBINARY:
		case SQLIMAGE: {
			DBBINARY *bin;
			unsigned char *res_buf;
			int res_length = dbdatlen(mssql_ptr->link, offset);

			res_buf = (unsigned char *) emalloc(res_length + 1);
			bin = ((DBBINARY *)dbdata(mssql_ptr->link, offset));
			memcpy(res_buf,bin,res_length);
			res_buf[res_length] = '\0';
			result->value.str.len = res_length;
			result->value.str.val = res_buf;
			result->type = IS_STRING;
			}
			break;
		case SQLNUMERIC:
		default: {
			if (dbwillconvert(column_type,SQLCHAR)) {
				char *res_buf;
				int res_length = dbdatlen(mssql_ptr->link,offset);
				if (column_type == SQLDATETIM4) res_length += 14;
				if (column_type == SQLDATETIME) res_length += 10;
			
				res_buf = (char *) emalloc(res_length + 1);
				res_length = dbconvert(NULL,column_type,dbdata(mssql_ptr->link,offset), res_length,SQLCHAR,res_buf,-1);

				result->value.str.val = res_buf;
				result->value.str.len = res_length;
				result->type = IS_STRING;
			} else {
				php_error(E_WARNING,"MS SQL:  column %d has unknown data type (%d)", offset, coltype(offset));
				ZVAL_FALSE(result);
			}
		}
	}
}

static void php_mssql_get_column_content_without_type(mssql_link *mssql_ptr,int offset,zval *result, int column_type)
{
	if (dbdatlen(mssql_ptr->link,offset) == 0) {
		ZVAL_NULL(result);
		return;
	}

	if (column_type == SQLVARBINARY ||
		column_type == SQLBINARY ||
		column_type == SQLIMAGE) {
		DBBINARY *bin;
		unsigned char *res_buf;
		int res_length = dbdatlen(mssql_ptr->link, offset);

		res_buf = (unsigned char *) emalloc(res_length + 1);
		bin = ((DBBINARY *)dbdata(mssql_ptr->link, offset));
		memcpy(res_buf, bin, res_length);
		res_buf[res_length] = '\0';
		result->value.str.len = res_length;
		result->value.str.val = res_buf;
		result->type = IS_STRING;
	}
	else if  (dbwillconvert(coltype(offset),SQLCHAR)) {
		unsigned char *res_buf;
		int res_length = dbdatlen(mssql_ptr->link,offset);
		if (column_type == SQLDATETIM4) res_length += 14;
		if (column_type == SQLDATETIME) res_length += 10;
		
		res_buf = (unsigned char *) emalloc(res_length + 1);
		res_length = dbconvert(NULL,coltype(offset),dbdata(mssql_ptr->link,offset), res_length, SQLCHAR,res_buf,-1);

		result->value.str.val = res_buf;
		result->value.str.len = res_length;
		result->type = IS_STRING;
	} else {
		php_error(E_WARNING,"MS SQL:  column %d has unknown data type (%d)", offset, coltype(offset));
		ZVAL_FALSE(result);
	}
}

static int _mssql_fetch_batch(mssql_link *mssql_ptr, mssql_result *result, int retvalue TSRMLS_DC) 
{
	int i, j = 0;
	int *column_types;
	char computed_buf[16];

	column_types = (int *) emalloc(sizeof(int) * result->num_fields);
	for (i=0; i<result->num_fields; i++) {
		char *fname = (char *)dbcolname(mssql_ptr->link,i+1);

		if (*fname) {
			result->fields[i].name = estrdup(fname);
		} else {
			if (j>0) {
				snprintf(computed_buf,16,"computed%d",j);
			} else {
				strcpy(computed_buf,"computed");
			}
			result->fields[i].name = estrdup(computed_buf);
			j++;
		}
		result->fields[i].max_length = dbcollen(mssql_ptr->link,i+1);
		result->fields[i].column_source = estrdup(dbcolsource(mssql_ptr->link,i+1));
		if (!result->fields[i].column_source) {
			result->fields[i].column_source = empty_string;
		}

		column_types[i] = coltype(i+1);

		result->fields[i].type = column_types[i];
		/* set numeric flag */
		switch (column_types[i]) {
			case SQLINT1:
			case SQLINT2:
			case SQLINT4:
			case SQLINTN:
			case SQLFLT8:
			case SQLNUMERIC:
			case SQLDECIMAL:
				result->fields[i].numeric = 1;
				break;
			case SQLCHAR:
			case SQLVARCHAR:
			case SQLTEXT:
			default:
				result->fields[i].numeric = 0;
				break;
		}
	}

	i=0;
	if (!result->data) {
		result->data = (zval **) emalloc(sizeof(zval *)*MSSQL_ROWS_BLOCK*(++result->blocks_initialized));
	}
	while (retvalue!=FAIL && retvalue!=NO_MORE_ROWS) {
		result->num_rows++;
		if (result->num_rows > result->blocks_initialized*MSSQL_ROWS_BLOCK) {
			result->data = (zval **) erealloc(result->data,sizeof(zval *)*MSSQL_ROWS_BLOCK*(++result->blocks_initialized));
		}
		result->data[i] = (zval *) emalloc(sizeof(zval)*result->num_fields);
		for (j=0; j<result->num_fields; j++) {
			INIT_ZVAL(result->data[i][j]);
			MS_SQL_G(get_column_content(mssql_ptr, j+1, &result->data[i][j], column_types[j]));
		}
		if (i<result->batchsize || result->batchsize==0) {
			i++;
			dbclrbuf(mssql_ptr->link,DBLASTROW(mssql_ptr->link)); 
			retvalue=dbnextrow(mssql_ptr->link);
		}
		else
			break;
		result->lastresult = retvalue;
	}
	efree(column_types);
	return i;
}

/* {{{ proto int mssql_fetch_batch(string result_index)
   Returns the next batch of records */
PHP_FUNCTION(mssql_fetch_batch)
{
	zval **mssql_result_index;
	mssql_result *result;
	mssql_link *mssql_ptr;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if ((*mssql_result_index)->type==IS_RESOURCE && (*mssql_result_index)->value.lval==0) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);
	mssql_ptr = result->mssql_ptr;
	_free_result(result, 0);
	result->cur_row=result->num_rows=0;
	result->num_rows = _mssql_fetch_batch(mssql_ptr, result, result->lastresult TSRMLS_CC);
	RETURN_LONG(result->num_rows);
}
/* }}} */

/* {{{ proto int mssql_query(string query [, int conn_id [, int batch_size]])
   Perform an SQL query on a MS-SQL server database */
PHP_FUNCTION(mssql_query)
{
	zval **query, **mssql_link_index, **zbatchsize;
	int retvalue;
	mssql_link *mssql_ptr;
	mssql_result *result;
	int id, num_fields;
	int batchsize;

	batchsize = MS_SQL_G(batchsize);
	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &query)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mssql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
			CHECK_LINK(id);
			break;
		case 2:
			if (zend_get_parameters_ex(2, &query, &mssql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		case 3:
			if (zend_get_parameters_ex(3, &query, &mssql_link_index, &zbatchsize)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			convert_to_long_ex(zbatchsize);
			batchsize = (*zbatchsize)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	ZEND_FETCH_RESOURCE2(mssql_ptr, mssql_link *, mssql_link_index, id, "MS SQL-Link", le_link, le_plink);
	
	convert_to_string_ex(query);
	
	if (dbcmd(mssql_ptr->link, (*query)->value.str.val)==FAIL) {
		php_error(E_WARNING,"MS SQL:  Unable to set query");
		RETURN_FALSE;
	}
	if (dbsqlexec(mssql_ptr->link)==FAIL || dbresults(mssql_ptr->link)==FAIL) {
		php_error(E_WARNING,"MS SQL:  Query failed");
		RETURN_FALSE;
	}
	
	/* The following is more or less the equivalent of mysql_store_result().
	 * fetch all rows from the server into the row buffer, thus:
	 * 1)  Being able to fire up another query without explicitly reading all rows
	 * 2)  Having numrows accessible
	 */
	retvalue=dbnextrow(mssql_ptr->link);
	
	if (retvalue==FAIL) {
		RETURN_FALSE;
	}

	if ((num_fields = dbnumcols(mssql_ptr->link)) <= 0) {
		RETURN_TRUE;
	}

	result = (mssql_result *) emalloc(sizeof(mssql_result));
	result->num_fields = num_fields;
	result->blocks_initialized = 1;
	
	result->batchsize = batchsize;
	result->data = NULL;
	result->blocks_initialized = 0;
	result->mssql_ptr = mssql_ptr;
	result->cur_field=result->cur_row=result->num_rows=0;

	result->fields = (mssql_field *) emalloc(sizeof(mssql_field)*result->num_fields);
	result->num_rows = _mssql_fetch_batch(mssql_ptr, result, retvalue TSRMLS_CC);
	
	ZEND_REGISTER_RESOURCE(return_value, result, le_result);
}
/* }}} */

/* {{{ proto int mssql_rows_affected(int conn_id)
   Returns the number of records affected by the query */
PHP_FUNCTION(mssql_rows_affected)
{
	zval **mssql_link_index;
	mssql_link *mssql_ptr;

	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &mssql_link_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE2(mssql_ptr, mssql_link *, mssql_link_index, -1, "MS SQL-Link", le_link, le_plink);
	RETURN_LONG(DBCOUNT(mssql_ptr->link));
}
/* }}} */


/* {{{ proto int mssql_free_result(string result_index)
   Free a MS-SQL result index */
PHP_FUNCTION(mssql_free_result)
{
	zval **mssql_result_index;
	mssql_result *result;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if ((*mssql_result_index)->type==IS_RESOURCE && (*mssql_result_index)->value.lval==0) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	zend_list_delete((*mssql_result_index)->value.lval);
	RETURN_TRUE;
}

/* }}} */

/* {{{ proto string mssql_get_last_message(void)
   Gets the last message from the MS-SQL server */
PHP_FUNCTION(mssql_get_last_message)
{
	RETURN_STRING(MS_SQL_G(server_message),1);
}

/* }}} */

/* {{{ proto int mssql_num_rows(int mssql_result_index)
   Returns the number of rows fetched in from the result id specified */
PHP_FUNCTION(mssql_num_rows)
{
	zval **mssql_result_index;
	mssql_result *result;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	return_value->value.lval = result->num_rows;
	return_value->type = IS_LONG;
}

/* }}} */

/* {{{ proto int mssql_num_fields(int mssql_result_index)
   Returns the number of fields fetched in from the result id specified */
PHP_FUNCTION(mssql_num_fields)
{
	zval **mssql_result_index;
	mssql_result *result;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	

	return_value->value.lval = result->num_fields;
	return_value->type = IS_LONG;
}

/* }}} */

static void php_mssql_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int result_type)
{
	zval **mssql_result_index, **resulttype = NULL;
	mssql_result *result;
	int i;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			if (!result_type) {
				result_type = MSSQL_BOTH;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &mssql_result_index, &resulttype)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(resulttype);
			result_type = (*resulttype)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}

	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	

	if (result->cur_row >= result->num_rows) {
		RETURN_FALSE;
	}
	
	if (array_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}
	
	for (i=0; i<result->num_fields; i++) {
		if (Z_TYPE(result->data[result->cur_row][i]) != IS_NULL) {
			char *data;
			int data_len;
			int should_copy;

			if (Z_TYPE(result->data[result->cur_row][i]) == IS_STRING) {
				if (PG(magic_quotes_runtime)) {
					data = php_addslashes(Z_STRVAL(result->data[result->cur_row][i]), Z_STRLEN(result->data[result->cur_row][i]), &result->data[result->cur_row][i].value.str.len, 1 TSRMLS_CC);
					should_copy = 0;
				}
				else
				{
					data = Z_STRVAL(result->data[result->cur_row][i]);
					data_len = Z_STRLEN(result->data[result->cur_row][i]);
					should_copy = 1;
				}

				if (result_type & MSSQL_NUM) {
					add_index_stringl(return_value, i, data, data_len, should_copy);
					should_copy = 1;
				}
				
				if (result_type & MSSQL_ASSOC) {
					add_assoc_stringl(return_value, result->fields[i].name, data, data_len, should_copy);
				}
			}
			else if (Z_TYPE(result->data[result->cur_row][i]) == IS_LONG) {
				if (result_type & MSSQL_NUM)
					add_index_long(return_value, i, result->data[result->cur_row][i].value.lval);
				
				if (result_type & MSSQL_ASSOC)
					add_assoc_long(return_value, result->fields[i].name, result->data[result->cur_row][i].value.lval);
			}
			else if (Z_TYPE(result->data[result->cur_row][i]) == IS_DOUBLE) {
				if (result_type & MSSQL_NUM)
					add_index_double(return_value, i, result->data[result->cur_row][i].value.dval);
				
				if (result_type & MSSQL_ASSOC)
					add_assoc_double(return_value, result->fields[i].name, result->data[result->cur_row][i].value.dval);
			}
		}
		else
		{
			if (result_type & MSSQL_NUM)
				add_index_null(return_value, i);
			if (result_type & MSSQL_ASSOC)
				add_assoc_null(return_value, result->fields[i].name);
		}
	}
	result->cur_row++;
}

/* {{{ proto array mssql_fetch_row(int result_id [, int result_type])
   Returns an array of the current row in the result set specified by result_id */
PHP_FUNCTION(mssql_fetch_row)
{
	php_mssql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, MSSQL_NUM);
}

/* }}} */

/* {{{ proto object mssql_fetch_object(int result_id [, int result_type])
   Returns a psuedo-object of the current row in the result set specified by result_id */
PHP_FUNCTION(mssql_fetch_object)
{
	php_mssql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, MSSQL_ASSOC);
	if (return_value->type==IS_ARRAY) {
		object_and_properties_init(return_value, &zend_standard_class_def, return_value->value.ht);
	}
}

/* }}} */

/* {{{ proto array mssql_fetch_array(int result_id [, int result_type])
   Returns an associative array of the current row in the result set specified by result_id */
PHP_FUNCTION(mssql_fetch_array)
{
	php_mssql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, MSSQL_ASSOC);
}

/* }}} */


/* {{{ proto int mssql_data_seek(int result_id, int offset)
   Moves the internal row pointer of the MS-SQL result associated with the specified result identifier to pointer to the specified row number */
PHP_FUNCTION(mssql_data_seek)
{
	zval **mssql_result_index, **offset;
	mssql_result *result;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &mssql_result_index, &offset)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	

	convert_to_long_ex(offset);
	if ((*offset)->value.lval<0 || (*offset)->value.lval>=result->num_rows) {
		php_error(E_WARNING,"MS SQL:  Bad row offset");
		RETURN_FALSE;
	}
	
	result->cur_row = (*offset)->value.lval;
	RETURN_TRUE;
}

/* }}} */

static char *php_mssql_get_field_name(int type)
{
	switch (type) {
		case SQLBINARY:
		case SQLVARBINARY:
			return "blob";
			break;
		case SQLCHAR:
		case SQLVARCHAR:
			return "char";
			break;
		case SQLTEXT:
			return "text";
			break;
		case SQLDATETIME:
		case SQLDATETIM4:
		case SQLDATETIMN:
			return "datetime";
			break;
		case SQLDECIMAL:
		case SQLFLT8:
		case SQLFLTN:
			return "real";
			break;
		case SQLINT1:
		case SQLINT2:
		case SQLINT4:
		case SQLINTN:
			return "int";
			break;
		case SQLNUMERIC:
			return "numeric";
			break;
		case SQLMONEY:
		case SQLMONEY4:
		case SQLMONEYN:
			return "money";
			break;
		case SQLBIT:
			return "bit";
			break;
		case SQLIMAGE:
			return "image";
			break;
		default:
			return "unknown";
			break;
	}
}

/* {{{ proto object mssql_fetch_field(int result_id [, int offset])
   Gets information about certain fields in a query result */
PHP_FUNCTION(mssql_fetch_field)
{
	zval **mssql_result_index, **offset;
	int field_offset;
	mssql_result *result;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			field_offset=-1;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &mssql_result_index, &offset)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(offset);
			field_offset = (*offset)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	if (field_offset==-1) {
		field_offset = result->cur_field;
		result->cur_field++;
	}
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		if (ZEND_NUM_ARGS()==2) { /* field specified explicitly */
			php_error(E_WARNING,"MS SQL:  Bad column offset");
		}
		RETURN_FALSE;
	}

	if (object_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}
	add_property_string(return_value, "name",result->fields[field_offset].name, 1);
	add_property_long(return_value, "max_length",result->fields[field_offset].max_length);
	add_property_string(return_value, "column_source",result->fields[field_offset].column_source, 1);
	add_property_long(return_value, "numeric", result->fields[field_offset].numeric);
	add_property_string(return_value, "type", php_mssql_get_field_name(result->fields[field_offset].type), 1);
}

/* }}} */

/* {{{ proto int mssql_field_length(int result_id [, int offset])
   Get the length of a MS-SQL field */
PHP_FUNCTION(mssql_field_length)
{
	zval **mssql_result_index, **offset;
	int field_offset;
	mssql_result *result;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			field_offset=-1;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &mssql_result_index, &offset)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(offset);
			field_offset = (*offset)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	if (field_offset==-1) {
		field_offset = result->cur_field;
		result->cur_field++;
	}
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		if (ZEND_NUM_ARGS()==2) { /* field specified explicitly */
			php_error(E_WARNING,"MS SQL:  Bad column offset");
		}
		RETURN_FALSE;
	}

	return_value->value.lval = result->fields[field_offset].max_length;
	return_value->type = IS_LONG;
}

/* }}} */

/* {{{ proto string mssql_field_name(int result_id [, int offset])
   Returns the name of the field given by offset in the result set given by result_id */
PHP_FUNCTION(mssql_field_name)
{
	zval **mssql_result_index, **offset;
	int field_offset;
	mssql_result *result;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			field_offset=-1;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &mssql_result_index, &offset)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(offset);
			field_offset = (*offset)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	if (field_offset==-1) {
		field_offset = result->cur_field;
		result->cur_field++;
	}
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		if (ZEND_NUM_ARGS()==2) { /* field specified explicitly */
			php_error(E_WARNING,"MS SQL:  Bad column offset");
		}
		RETURN_FALSE;
	}

	return_value->value.str.val = estrdup(result->fields[field_offset].name);
	return_value->value.str.len = strlen(result->fields[field_offset].name);
	return_value->type = IS_STRING;
}

/* }}} */

/* {{{ proto string mssql_field_type(int result_id [, int offset])
   Returns the type of a field */
PHP_FUNCTION(mssql_field_type)
{
	zval **mssql_result_index, **offset;
	int field_offset;
	mssql_result *result;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			field_offset=-1;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &mssql_result_index, &offset)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(offset);
			field_offset = (*offset)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	if (field_offset==-1) {
		field_offset = result->cur_field;
		result->cur_field++;
	}
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		if (ZEND_NUM_ARGS()==2) { /* field specified explicitly */
			php_error(E_WARNING,"MS SQL:  Bad column offset");
		}
		RETURN_FALSE;
	}

	return_value->value.str.val = estrdup(php_mssql_get_field_name(result->fields[field_offset].type));
	return_value->value.str.len = strlen(php_mssql_get_field_name(result->fields[field_offset].type));
	return_value->type = IS_STRING;
}

/* }}} */

/* {{{ proto bool mssql_field_seek(int result_id, int offset)
   Seeks to the specified field offset */
PHP_FUNCTION(mssql_field_seek)
{
	zval **mssql_result_index, **offset;
	int field_offset;
	mssql_result *result;

	if (ZEND_NUM_ARGS()!=2 || zend_get_parameters_ex(2, &mssql_result_index, &offset)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	convert_to_long_ex(offset);
	field_offset = (*offset)->value.lval;
	
	if (field_offset<0 || field_offset >= result->num_fields) {
		php_error(E_WARNING,"MS SQL:  Bad column offset");
		RETURN_FALSE;
	}

	result->cur_field = field_offset;
	RETURN_TRUE;
}

/* }}} */

/* {{{ proto string mssql_result(int result_id, int row, mixed field)
   Returns the contents of one cell from a MS-SQL result set */
PHP_FUNCTION(mssql_result)
{
	zval **row, **field, **mssql_result_index;
	int field_offset=0;
	mssql_result *result;

	if (ZEND_NUM_ARGS()!=3 || zend_get_parameters_ex(3, &mssql_result_index, &row, &field)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	
	
	convert_to_long_ex(row);
	if ((*row)->value.lval < 0 || (*row)->value.lval >= result->num_rows) {
		php_error(E_WARNING,"MS SQL:  Bad row offset (%d)", (*row)->value.lval);
		RETURN_FALSE;
	}

	switch((*field)->type) {
		case IS_STRING: {
			int i;

			for (i=0; i<result->num_fields; i++) {
				if (!strcasecmp(result->fields[i].name, (*field)->value.str.val)) {
					field_offset = i;
					break;
				}
			}
			if (i>=result->num_fields) { /* no match found */
				php_error(E_WARNING,"MS SQL:  %s field not found in result", (*field)->value.str.val);
				RETURN_FALSE;
			}
			break;
		}
		default:
			convert_to_long_ex(field);
			field_offset = (*field)->value.lval;
			if (field_offset<0 || field_offset>=result->num_fields) {
				php_error(E_WARNING,"MS SQL:  Bad column offset specified");
				RETURN_FALSE;
			}
			break;
	}

	*return_value = result->data[(*row)->value.lval][field_offset];
	ZVAL_COPY_CTOR(return_value);
}
/* }}} */

/* {{{ proto string mssql_next_result(int result_id)
   Move the internal result pointer to the next result */
PHP_FUNCTION(mssql_next_result)
{
	zval **mssql_result_index;
	int retvalue;
	mssql_result *result;
	mssql_link *mssql_ptr;

	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &mssql_result_index)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(result, mssql_result *, mssql_result_index, -1, "MS SQL-result", le_result);	

	mssql_ptr = result->mssql_ptr;
	retvalue = dbresults(mssql_ptr->link);
	if (retvalue == FAIL || retvalue == NO_MORE_RESULTS || retvalue == NO_MORE_RPC_RESULTS) {
		RETURN_FALSE;
	}
	else {
		_free_result(result, 1);
		result->cur_row=result->num_fields=result->num_rows=0;
		dbclrbuf(mssql_ptr->link,DBLASTROW(mssql_ptr->link));
		retvalue = dbnextrow(mssql_ptr->link);

		result->num_fields = dbnumcols(mssql_ptr->link);
		result->fields = (mssql_field *) emalloc(sizeof(mssql_field)*result->num_fields);
		result->num_rows = _mssql_fetch_batch(mssql_ptr, result, retvalue TSRMLS_CC);
		RETURN_TRUE;
	}

}
/* }}} */


/* {{{ proto void mssql_min_error_severity(int severity)
   Sets the lower error severity */
PHP_FUNCTION(mssql_min_error_severity)
{
	zval **severity;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &severity)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(severity);
	MS_SQL_G(min_error_severity) = (*severity)->value.lval;
}

/* }}} */

/* {{{ proto void mssql_min_message_severity(int severity)
   Sets the lower message severity */
PHP_FUNCTION(mssql_min_message_severity)
{
	zval **severity;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &severity)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(severity);
	MS_SQL_G(min_message_severity) = (*severity)->value.lval;
}
/* }}} */

/* {{{ proto int mssql_init(string sp_name [, int conn_id])
   Initializes a stored procedure or a remote stored procedure  */
PHP_FUNCTION(mssql_init)
{
	zval **sp_name, **mssql_link_index;
	mssql_link *mssql_ptr;
	mssql_statement *statement;
	int id;
	
	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &sp_name)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mssql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU);
			CHECK_LINK(id);
			break;

		case 2:
			if (zend_get_parameters_ex(2, &sp_name, &mssql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;

		default:
			WRONG_PARAM_COUNT;
			break;
	}

	ZEND_FETCH_RESOURCE2(mssql_ptr, mssql_link *, mssql_link_index, id, "MS SQL-Link", le_link, le_plink);
	
	convert_to_string_ex(sp_name);
	
	if (dbrpcinit(mssql_ptr->link, (*sp_name)->value.str.val,0)==FAIL) {
		php_error(E_WARNING,"MS SQL:  unable to init stored procedure");
		RETURN_FALSE;
	}

	statement=NULL;
	statement = ecalloc(1,sizeof(mssql_statement));
	
	if (statement!=NULL) {
		statement->link = mssql_ptr;
		statement->executed=FALSE;
	}
	else {
		php_error(E_WARNING,"mssql_init: unable to allocate statement");
		RETURN_FALSE;
	}

	statement->id = zend_list_insert(statement,le_statement);
	
	RETURN_RESOURCE(statement->id);
}
/* }}} */

/* {{{ proto int mssql_bind(int stmt, string param_name, mixed var, int type 
		[, int is_output[, int is_null[, int maxlen]])
   Adds a parameter to a stored procedure or a remote stored procedure  */
PHP_FUNCTION(mssql_bind)
{
	int	type, is_output, is_null, datalen, maxlen;
	zval **stmt, **param_name, **var, **yytype;
	mssql_link *mssql_ptr;
	mssql_statement *statement;
	mssql_bind bind,*bindp;
	int id, status;
	LPBYTE value;

	id=0;
	status=0;

	/* BEGIN input validation */
	switch(ZEND_NUM_ARGS()) {
		case 4: 
			if (zend_get_parameters_ex(4, &stmt, &param_name, &var, &yytype)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(yytype);
			type=(*yytype)->value.lval;
			is_null=FALSE;
			is_output=FALSE;
			maxlen=-1;
	
			break;
						
		case 5: {
			zval **yyis_output;

				if (zend_get_parameters_ex(5, &stmt, &param_name, &var, &yytype, &yyis_output)==FAILURE) {
					RETURN_FALSE;
				}
				convert_to_long_ex(yytype);
				convert_to_long_ex(yyis_output);
				type=(*yytype)->value.lval;
				is_null=FALSE;
				is_output=(*yyis_output)->value.lval;
				maxlen=-1;	
			}
			break;	

		case 6: {
				zval **yyis_output, **yyis_null;

				if (zend_get_parameters_ex(6, &stmt, &param_name, &var, &yytype, &yyis_output, &yyis_null)==FAILURE) {
					RETURN_FALSE;
				}
				convert_to_long_ex(yytype);
				convert_to_long_ex(yyis_output);
				convert_to_long_ex(yyis_null);
				type=(*yytype)->value.lval;
				is_output=(*yyis_output)->value.lval;
				is_null=(*yyis_null)->value.lval;
				maxlen=-1;
			}
			break;
		
		case 7: {
				zval **yyis_output, **yyis_null, **yymaxlen;

				if (zend_get_parameters_ex(7, &stmt, &param_name, &var, &yytype, &yyis_output, &yyis_null, &yymaxlen)==FAILURE) {
					RETURN_FALSE;
				}
				convert_to_long_ex(yytype);
				convert_to_long_ex(yyis_output);
				convert_to_long_ex(yyis_null);
				convert_to_long_ex(yymaxlen);
				type=(*yytype)->value.lval;
				is_output=(*yyis_output)->value.lval;
				is_null=(*yyis_null)->value.lval;
				maxlen=(*yymaxlen)->value.lval;				
			}
			break;	
		
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	/* END input validation */
	
	ZEND_FETCH_RESOURCE(statement, mssql_statement *, stmt, -1, "MS SQL-Statement", le_statement);
	if (statement==NULL) {
		RETURN_FALSE;
	}
	mssql_ptr=statement->link;

	/* modify datalen and maxlen according to dbrpcparam documentation */
	if ( (type==SQLVARCHAR) || (type==SQLCHAR) || (type==SQLTEXT) )	{	/* variable-length type */
		if (is_null) {
			maxlen=0;
			datalen=0;
		}
		else {
			convert_to_string_ex(var);
			datalen=(*var)->value.str.len;
			value=(LPBYTE)(*var)->value.str.val;
		}
	}
	else	{	/* fixed-length type */
		if (is_null)	{
			datalen=0;
		}
		else {
			datalen=-1;
		}
		maxlen=-1;

		switch (type)	{

			case SQLFLT8:
				convert_to_double_ex(var);
				value=(LPBYTE)(&(*var)->value.dval);
				break;

			case SQLINT1:
			case SQLINT2:
			case SQLINT4:
				convert_to_long_ex(var);
				value=(LPBYTE)(&(*var)->value.lval);
				break;

			default:
				php_error(E_WARNING,"mssql_bind: unsupported type");
				RETURN_FALSE;
				break;
		}
	}

	convert_to_string_ex(param_name);
	
	if (is_output) {
		status=DBRPCRETURN;
	}
	
	/* hashtable of binds */
	if (! statement->binds) {
		ALLOC_HASHTABLE(statement->binds);
		zend_hash_init(statement->binds, 13, NULL, _mssql_bind_hash_dtor, 0);
	}

	memset((void*)&bind,0,sizeof(mssql_bind));
	zend_hash_add(statement->binds,(*param_name)->value.str.val,(*param_name)->value.str.len,&bind,sizeof(mssql_bind),(void **)&bindp);
	bindp->zval=*var;
	zval_add_ref(var);

	/* no call to dbrpcparam if RETVAL */
	if ( strcmp("RETVAL",(*param_name)->value.str.val)!=0 ) {						
		if (dbrpcparam(mssql_ptr->link, (*param_name)->value.str.val, (BYTE)status, type, maxlen, datalen, (LPCBYTE)value)==FAIL) {
			php_error(E_WARNING,"MS SQL:  Unable to set parameter");
			RETURN_FALSE;
		}
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int mssql_execute(int stmt)
   Executes a stored procedure on a MS-SQL server database */
PHP_FUNCTION(mssql_execute)
{
	zval **stmt;
	int retvalue,retval_results;
	mssql_link *mssql_ptr;
	mssql_statement *statement;
	mssql_bind *bind;
	mssql_result *result;
	int num_fields,num_rets,type;	
	int blocks_initialized=1;
	int i;
	int batchsize;
	int ac = ZEND_NUM_ARGS();
	char *parameter;

	batchsize = MS_SQL_G(batchsize);
	if (ac !=1 || zend_get_parameters_ex(1, &stmt)==FAILURE) {
        WRONG_PARAM_COUNT;
    }

	ZEND_FETCH_RESOURCE(statement, mssql_statement *, stmt, -1, "MS SQL-Statement", le_statement);

	mssql_ptr=statement->link;

	if (dbrpcexec(mssql_ptr->link)==FAIL || dbsqlok(mssql_ptr->link)==FAIL) {
		php_error(E_WARNING,"MS SQL:  stored procedure execution failed.");
		RETURN_FALSE;
	}

	retval_results=dbresults(mssql_ptr->link);

	if (retval_results==FAIL) {
		php_error(E_WARNING,"MS SQL:  could not retrieve results");
		RETURN_FALSE;
	}

	/* The following is just like mssql_query, fetch all rows from the server into 
	 *	the row buffer. We add here the RETVAL and OUTPUT parameters stuff
	 */
	result=NULL;
	/* if multiple recordsets in a stored procedure were supported, we would 
	   use a "while (retval_results!=NO_MORE_RESULTS)" instead an "if" */
	if (retval_results==SUCCEED) {
		if ( (retvalue=(dbnextrow(mssql_ptr->link)))!=NO_MORE_ROWS ) {
			num_fields = dbnumcols(mssql_ptr->link);
			if (num_fields <= 0) {
				RETURN_TRUE;
			}
			
			result = (mssql_result *) emalloc(sizeof(mssql_result));
		
			result->batchsize = batchsize;
			result->blocks_initialized = 1;
			result->data = (zval **) emalloc(sizeof(zval *)*MSSQL_ROWS_BLOCK);
			result->mssql_ptr = mssql_ptr;
			result->cur_field=result->cur_row=result->num_rows=0;
			result->num_fields = num_fields;

			result->fields = (mssql_field *) emalloc(sizeof(mssql_field)*num_fields);
			result->num_rows = _mssql_fetch_batch(mssql_ptr, result, retvalue TSRMLS_CC);
		}
		retval_results=dbresults(mssql_ptr->link);
	}
	
	if (retval_results==SUCCEED) {
		php_error(E_WARNING,"mssql_execute:  multiple recordsets from a stored procedure not supported yet! (Skipping...)");
		retval_results=dbresults(mssql_ptr->link);
		
		while (retval_results==SUCCEED) {
			retval_results=dbresults(mssql_ptr->link);
		}
	}

	if (retval_results==NO_MORE_RESULTS) {
		/* Now to fetch RETVAL and OUTPUT values*/
		num_rets = dbnumrets(mssql_ptr->link);
		
		if (num_rets!=0) {
			for (i = 1; i <= num_rets; i++) {
				parameter=(char*)dbretname(mssql_ptr->link, i);
				type=dbrettype(mssql_ptr->link, i);
							
				if (statement->binds!=NULL ) {	/*	Maybe a non-parameter sp	*/
					if (zend_hash_find(statement->binds, parameter, strlen(parameter), (void**)&bind)==SUCCESS) {
						switch (type) {
							case SQLBIT:
							case SQLINT1:
							case SQLINT2:
							case SQLINT4:
								convert_to_long_ex(&bind->zval);
								bind->zval->value.lval=*((int *)(dbretdata(mssql_ptr->link,i)));
								break;
				
							case SQLFLT8:
								convert_to_double_ex(&bind->zval);
								bind->zval->value.dval=*((double *)(dbretdata(mssql_ptr->link,i)));
								break;

							case SQLCHAR:
							case SQLVARCHAR:
							case SQLTEXT:
								convert_to_string_ex(&bind->zval);
								bind->zval->value.str.len=dbretlen(mssql_ptr->link,i);
								bind->zval->value.str.val = estrndup(dbretdata(mssql_ptr->link,i),bind->zval->value.str.len);
								break;
						}
					}
					else {
						php_error(E_WARNING,"mssql_execute: an output parameter variable was not provided");
					}
				}
			}
		}
		
		if (statement->binds!=NULL ) {	/*	Maybe a non-parameter sp	*/
			if (zend_hash_find(statement->binds, "RETVAL", 6, (void**)&bind)==SUCCESS) {
				if (dbhasretstat(mssql_ptr->link)) {
					convert_to_long_ex(&bind->zval);
					bind->zval->value.lval=dbretstatus(mssql_ptr->link);
				}
				else {
					php_error(E_WARNING,"mssql_execute: stored procedure has no return value. Nothing was returned into RETVAL");
				}
			}
		}
	}

	if (result==NULL) {
		RETURN_TRUE;	/* no recordset returned ...*/
	}
	else {
		ZEND_REGISTER_RESOURCE(return_value, result, le_result);
	}
}
/* }}} */

/* {{{ proto string mssql_guid_string(string binary [,int short_format])
   Converts a 16 byte binary GUID to a string  */
PHP_FUNCTION(mssql_guid_string)
{
	zval **binary, **short_format;
	int sf = 0;
	char buffer[32+1];
	char buffer2[36+1];
	
	switch(ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &binary)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(binary);
			break;
		case 2:
			if (zend_get_parameters_ex(2, &binary, &short_format)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(binary);
			convert_to_long_ex(short_format);
			sf = (*short_format)->value.lval;
			break;

		default:
			WRONG_PARAM_COUNT;
			break;
	}

	dbconvert(NULL, SQLBINARY, (BYTE*)(*binary)->value.str.val, 16, SQLCHAR, buffer, -1);

	if (sf) {
		php_strtoupper(buffer, 32);
		RETURN_STRING(buffer, 1);
	}
	else {
		int i;
		for (i=0; i<4; i++) {
			buffer2[2*i] = buffer[6-2*i];
			buffer2[2*i+1] = buffer[7-2*i];
		}
		buffer2[8] = '-';
		for (i=0; i<2; i++) {
			buffer2[9+2*i] = buffer[10-2*i];
			buffer2[10+2*i] = buffer[11-2*i];
		}
		buffer2[13] = '-';
		for (i=0; i<2; i++) {
			buffer2[14+2*i] = buffer[14-2*i];
			buffer2[15+2*i] = buffer[15-2*i];
		}
		buffer2[18] = '-';
		for (i=0; i<4; i++) {
			buffer2[19+i] = buffer[16+i];
		}
		buffer2[23] = '-';
		for (i=0; i<12; i++) {
			buffer2[24+i] = buffer[20+i];
		}
		buffer2[36] = 0;

		php_strtoupper(buffer2, 36);
		RETURN_STRING(buffer2, 1);
	}
}
/* }}} */

#endif
