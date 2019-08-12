/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2001 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.92 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_92.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#include <stdio.h>
#include "zend.h"
#include "zend_API.h"
#include "zend_globals.h"
#include "zend_constants.h"
#include "zend_list.h"

ZEND_API char *empty_string = "";	/* in order to save emalloc() and efree() time for
									 * empty strings (usually used to denote empty
									 * return values in failed functions).
									 * The macro STR_FREE() will not efree() it.
									 */


ZEND_API void _zval_dtor(zval *zvalue ZEND_FILE_LINE_DC)
{
	if (zvalue->type==IS_LONG) {
		return;
	}
	switch(zvalue->type) {
		case IS_STRING:
		case IS_CONSTANT:
			CHECK_ZVAL_STRING_REL(zvalue);
			STR_FREE_REL(zvalue->value.str.val);
			break;
		case IS_ARRAY:
		case IS_CONSTANT_ARRAY: {
				TSRMLS_FETCH();

				if (zvalue->value.ht && (zvalue->value.ht != &EG(symbol_table))) {
					zend_hash_destroy(zvalue->value.ht);
					FREE_HASHTABLE(zvalue->value.ht);
				}
			}
			break;
		case IS_OBJECT:
			zend_hash_destroy(zvalue->value.obj.properties);
			FREE_HASHTABLE(zvalue->value.obj.properties);
			break;
		case IS_RESOURCE:	{
				TSRMLS_FETCH();

				/* destroy resource */
				zend_list_delete(zvalue->value.lval);
			}
			break;
		case IS_LONG:
		case IS_DOUBLE:
		case IS_BOOL:
		case IS_NULL:
		default:
			return;
			break;
	}
}


ZEND_API void zval_add_ref(zval **p)
{
	(*p)->refcount++;
}


ZEND_API int _zval_copy_ctor(zval *zvalue ZEND_FILE_LINE_DC)
{
	switch (zvalue->type) {
		case IS_RESOURCE: {
				TSRMLS_FETCH();

				zend_list_addref(zvalue->value.lval);
			}
			break;
		case IS_BOOL:
		case IS_LONG:
		case IS_NULL:
			break;
		case IS_CONSTANT:
		case IS_STRING:
			if (zvalue->value.str.val) {
				if (zvalue->value.str.len==0) {
					zvalue->value.str.val = empty_string;
					return SUCCESS;
				}
			}
			CHECK_ZVAL_STRING_REL(zvalue);
			zvalue->value.str.val = (char *) estrndup_rel(zvalue->value.str.val, zvalue->value.str.len);
			break;
		case IS_ARRAY:
		case IS_CONSTANT_ARRAY: {
				zval *tmp;
				HashTable *original_ht = zvalue->value.ht;
				TSRMLS_FETCH();

				if (zvalue->value.ht == &EG(symbol_table)) {
					return SUCCESS; /* do nothing */
				}
				ALLOC_HASHTABLE_REL(zvalue->value.ht);
				zend_hash_init(zvalue->value.ht, 0, NULL, ZVAL_PTR_DTOR, 0);
				zend_hash_copy(zvalue->value.ht, original_ht, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
			}
			break;
		case IS_OBJECT: {
				zval *tmp;
				HashTable *original_ht = zvalue->value.obj.properties;

				ALLOC_HASHTABLE_REL(zvalue->value.obj.properties);
				zend_hash_init(zvalue->value.obj.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
				zend_hash_copy(zvalue->value.obj.properties, original_ht, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
			}
			break;
	}
	return SUCCESS;
}


ZEND_API int zval_persist(zval *zvalue TSRMLS_DC)
{
	switch (zvalue->type) {
		case IS_RESOURCE:
			return FAILURE; /* resources cannot be persisted */
			break;
		case IS_BOOL:
		case IS_LONG:
		case IS_NULL:
			break;
		case IS_CONSTANT:
		case IS_STRING:
			if (zvalue->value.str.val) {
				if (zvalue->value.str.len==0) {
					zvalue->value.str.val = empty_string;
					return SUCCESS;
				}
			}
			persist_alloc(zvalue->value.str.val);
			break;
		case IS_ARRAY:
		case IS_CONSTANT_ARRAY:
			persist_alloc(zvalue->value.ht);
			zend_hash_apply(zvalue->value.ht, (apply_func_t) zval_persist TSRMLS_CC);
			break;
		case IS_OBJECT:
			persist_alloc(zvalue->value.obj.properties);
			zend_hash_apply(zvalue->value.obj.properties, (apply_func_t) zval_persist TSRMLS_CC);
			break;
	}
	return SUCCESS;
}


ZEND_API int zend_print_variable(zval *var) 
{
	return zend_print_zval(var, 0);
}


#if ZEND_DEBUG
ZEND_API int _zval_copy_ctor_wrapper(zval *zvalue)
{
	return zval_copy_ctor(zvalue);
}


ZEND_API void _zval_dtor_wrapper(zval *zvalue)
{
	zval_dtor(zvalue);
}



ZEND_API void _zval_ptr_dtor_wrapper(zval **zval_ptr)
{
	zval_ptr_dtor(zval_ptr);
}
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
