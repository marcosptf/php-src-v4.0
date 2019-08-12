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
   | Author: Zeev Suraski <zeev@zend.com>                                 |
   |         Harald Radi  <h.radi@nme.at>                                 |
   |         Alan Brown   <abrown@pobox.com>                              |
   +----------------------------------------------------------------------+
 */

/*
 * This module implements support for COM components that support the IDispatch
 * interface.  Both local (COM) and remote (DCOM) components can be accessed.
 *
 * Type libraries can be loaded (in order for PHP to recognize automation constants)
 * by specifying a typelib_file in the PHP .ini file.  That file should contain
 * paths to type libraries, one in every line.  By default, constants are registered
 * as case-sensitive.  If you want them to be defined as case-insensitive, add
 * #case_insensitive or #cis at the end of the type library path.
 *
 * This is also the first module to demonstrate Zend's OO syntax overloading
 * capabilities.  CORBA coders are invited to write a CORBA module as well!
 *
 * Zeev
 */

/*
 * 28.12.2000
 * unicode conversion fixed by Harald Radi <h.radi@nme.at>
 *
 * now all these strange '?'s should be disapeared
 */

/*
 * 28.1.2001
 * VARIANT datatype and pass_by_reference support
 */

/*
 * 03.6.2001
 * Enhanced Typelib support to include a search by name
 */

#ifdef PHP_WIN32

#define _WIN32_DCOM

#include <iostream.h>
#include <math.h>

#include "php.h"
#include "php_ini.h"

#include "php_COM.h"
#include "php_VARIANT.h"

static ITypeLib *php_COM_find_typelib(char *search_string, int mode TSRMLS_DC);
static int php_COM_load_typelib(ITypeLib *TypeLib, int mode TSRMLS_DC);
static int do_COM_offget(VARIANT *result, comval *array, pval *property, int cleanup TSRMLS_DC);
static int do_COM_propget(VARIANT *var_result, comval *obj, pval *arg_property, int cleanup TSRMLS_DC);
static void php_register_COM_class(TSRMLS_D);

static int le_comval;
static int codepage;

#ifdef _DEBUG
int resourcecounter = 1;
#endif

function_entry COM_functions[] = {
	PHP_FE(com_load,                                NULL)
	PHP_FE(com_invoke,                              NULL)
	PHP_FE(com_addref,                              NULL)
	PHP_FE(com_release,                             NULL)
	PHP_FE(com_propget,                             NULL)
	PHP_FE(com_propput,                             NULL)
	PHP_FE(com_load_typelib,                        NULL)
	PHP_FE(com_isenum,                              NULL)

	PHP_FALIAS(com_get,         com_propget,        NULL)
	PHP_FALIAS(com_propset,     com_propput,        NULL)
	PHP_FALIAS(com_set,         com_propput,        NULL)

	{ NULL, NULL, NULL }
};


static PHP_MINFO_FUNCTION(COM)
{
	DISPLAY_INI_ENTRIES();
}


PHPAPI HRESULT php_COM_invoke(comval *obj, DISPID dispIdMember, WORD wFlags, DISPPARAMS FAR*  pDispParams, VARIANT FAR*  pVarResult, char **ErrString TSRMLS_DC)
{
	HRESULT hr;
	int failed = FALSE;
	unsigned int ArgErr;
	EXCEPINFO ExceptInfo;

	*ErrString = NULL;
	/* @todo use DispInvoke here ? */
	if (C_ISREFD(obj)) {
		if (C_HASTLIB(obj)) {
			hr = C_TYPEINFO_VT(obj)->Invoke(C_TYPEINFO(obj), C_DISPATCH(obj), dispIdMember, wFlags, pDispParams, pVarResult, &ExceptInfo, &ArgErr);
			if (FAILED(hr)) {
				hr = C_DISPATCH_VT(obj)->Invoke(C_DISPATCH(obj), dispIdMember, &IID_NULL, LOCALE_SYSTEM_DEFAULT, wFlags, pDispParams, pVarResult, &ExceptInfo, &ArgErr);
				if (SUCCEEDED(hr)) {
					/*
					 * ITypLib doesn't work
					 * Release ITypeLib and fall back to IDispatch
					 */

					C_TYPEINFO_VT(obj)->Release(C_TYPEINFO(obj));
					C_HASTLIB(obj) = FALSE;
				}
			}
		} else {
			hr = C_DISPATCH_VT(obj)->Invoke(C_DISPATCH(obj), dispIdMember, &IID_NULL, LOCALE_SYSTEM_DEFAULT, wFlags, pDispParams, pVarResult, &ExceptInfo, &ArgErr);
		}

		if (FAILED(hr)) {
			switch (hr)	{
				case DISP_E_EXCEPTION: {
						int srclen;
						char *src = php_OLECHAR_to_char(ExceptInfo.bstrSource, &srclen, codepage TSRMLS_CC);
						int desclen;
						char *desc = php_OLECHAR_to_char(ExceptInfo.bstrDescription, &desclen, codepage TSRMLS_CC);

						*ErrString = pemalloc(srclen+desclen+50, 1);
						sprintf(*ErrString, "<b>Source</b>: %s <b>Description</b>: %s", src, desc);
						efree(src);
						efree(desc);
						SysFreeString(ExceptInfo.bstrSource);
						SysFreeString(ExceptInfo.bstrDescription);
						SysFreeString(ExceptInfo.bstrHelpFile);
					}
					break;
				case DISP_E_PARAMNOTFOUND:
				case DISP_E_TYPEMISMATCH:
					*ErrString = pemalloc(25, 1);
					sprintf(*ErrString, "<b>Argument</b>: %d", pDispParams->cArgs-ArgErr+1);
					break;
			}
		}

		if (pVarResult && (V_VT(pVarResult) == VT_EMPTY)) {
			V_VT(pVarResult) = VT_I4;
			V_I4(pVarResult) = hr;
		}

		return hr;
	} else {
		return DISP_E_UNKNOWNINTERFACE;
	}
}


PHPAPI HRESULT php_COM_get_ids_of_names(comval *obj, OLECHAR FAR* FAR* rgszNames, DISPID FAR* rgDispId TSRMLS_DC)
{
	HRESULT hr;

	if (C_ISREFD(obj)) {
		if (C_HASTLIB(obj)) {
			hr = C_TYPEINFO_VT(obj)->GetIDsOfNames(C_TYPEINFO(obj), rgszNames, 1, rgDispId);

			if (FAILED(hr)) {
				hr = C_DISPATCH_VT(obj)->GetIDsOfNames(C_DISPATCH(obj), &IID_NULL, rgszNames, 1, LOCALE_SYSTEM_DEFAULT, rgDispId);

				if (SUCCEEDED(hr)) {
					/*
					 * ITypLib doesn't work
					 * Release ITypeLib and fall back to IDispatch
					 */

					C_TYPEINFO_VT(obj)->Release(C_TYPEINFO(obj));
					C_HASTLIB(obj) = FALSE;
				}
			}
		} else {
			hr = C_DISPATCH_VT(obj)->GetIDsOfNames(C_DISPATCH(obj), &IID_NULL, rgszNames, 1, LOCALE_SYSTEM_DEFAULT, rgDispId);
		}

		return hr;
	} else {
		return DISP_E_UNKNOWNINTERFACE;
	}
}


PHPAPI HRESULT php_COM_release(comval *obj TSRMLS_DC)
{
	HRESULT hr;
	
	if (obj->refcount > 1) {
		C_RELEASE(obj);
	} else if (obj->refcount == 1) {
		if (C_HASTLIB(obj)) {
			C_TYPEINFO_VT(obj)->Release(C_TYPEINFO(obj));
		}
		if (C_HASENUM(obj)) {
			hr = C_ENUMVARIANT_VT(obj)->Release(C_ENUMVARIANT(obj));
		}
		hr = C_DISPATCH_VT(obj)->Release(C_DISPATCH(obj));
		C_RELEASE(obj);
	}

	return obj->refcount;
}


PHPAPI HRESULT php_COM_addref(comval *obj TSRMLS_DC)
{
	if (C_ISREFD(obj)) {
		C_ADDREF(obj);
	}

	return obj->refcount;
}


PHPAPI HRESULT php_COM_set(comval *obj, IDispatch FAR* FAR* ppDisp, int cleanup TSRMLS_DC)
{
	HRESULT hr = 1;
	DISPPARAMS dispparams;
	VARIANT *var_result;
	IDispatch FAR* pDisp;

	pDisp = *ppDisp;
	if (cleanup) {
		*ppDisp = NULL;
	}
	
	C_REFCOUNT(obj) = 1;
	C_DISPATCH(obj) = pDisp;
	C_HASTLIB(obj) = SUCCEEDED(C_DISPATCH_VT(obj)->GetTypeInfo(C_DISPATCH(obj), 0, LANG_NEUTRAL, &C_TYPEINFO(obj)));

	dispparams.rgvarg = NULL;
	dispparams.rgdispidNamedArgs = NULL;
	dispparams.cArgs = 0;
	dispparams.cNamedArgs = 0;

	ALLOC_VARIANT(var_result);

	if (C_HASENUM(obj) = SUCCEEDED(C_DISPATCH_VT(obj)->Invoke(C_DISPATCH(obj), DISPID_NEWENUM, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
									DISPATCH_METHOD|DISPATCH_PROPERTYGET, &dispparams, var_result, NULL, NULL))) {
		if (V_VT(var_result) == VT_UNKNOWN) {
		    C_HASENUM(obj) = SUCCEEDED(V_UNKNOWN(var_result)->lpVtbl->QueryInterface(V_UNKNOWN(var_result), &IID_IEnumVARIANT,
									(void**)&C_ENUMVARIANT(obj)));
		} else if (V_VT(var_result) == VT_DISPATCH) {
		    C_HASENUM(obj) = SUCCEEDED(V_DISPATCH(var_result)->lpVtbl->QueryInterface(V_DISPATCH(var_result), &IID_IEnumVARIANT,
									(void**)&C_ENUMVARIANT(obj)));
		}
	}

	FREE_VARIANT(var_result);

	if (!cleanup) {
		hr = C_DISPATCH_VT(obj)->AddRef(C_DISPATCH(obj));
	}

#ifdef _DEBUG
	obj->resourceindex = resourcecounter++;
#endif

	return hr;
}


PHPAPI HRESULT php_COM_clone(comval *obj, comval *clone, int cleanup TSRMLS_DC)
{
	HRESULT hr;

	C_HASTLIB(obj) = C_HASTLIB(clone);
	C_HASENUM(obj) = C_HASENUM(obj);
	C_DISPATCH(obj) = C_DISPATCH(clone);
	C_TYPEINFO(obj) = C_TYPEINFO(clone);

	if (cleanup || !C_ISREFD(obj)) {
		obj->refcount = clone->refcount;
		clone->refcount = 0;
	} else {
		if (C_HASTLIB(obj)) {
			C_TYPEINFO_VT(obj)->AddRef(C_TYPEINFO(obj));
		}
		if (C_HASENUM(obj)) {
			C_ENUMVARIANT_VT(obj)->AddRef(C_ENUMVARIANT(obj));
		}
		hr = C_DISPATCH_VT(obj)->AddRef(C_DISPATCH(obj));
		obj->refcount = 1;
	}

#ifdef _DEBUG
	obj->resourceindex = resourcecounter++;
#endif

	return hr;
}


PHPAPI char *php_COM_error_message(HRESULT hr TSRMLS_DC)
{
	void *pMsgBuf;

	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
					  hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &pMsgBuf, 0, NULL)) {
		char error_string[] = "No description available";

		pMsgBuf = LocalAlloc(LMEM_FIXED, sizeof(error_string));
		memcpy(pMsgBuf, error_string, sizeof(error_string));
	}

	return pMsgBuf;
}


static char *php_string_from_clsid(const CLSID *clsid TSRMLS_DC)
{
	LPOLESTR ole_clsid;
	char *clsid_str;

	StringFromCLSID(clsid, &ole_clsid);
	clsid_str = php_OLECHAR_to_char(ole_clsid, NULL, codepage TSRMLS_CC);
	LocalFree(ole_clsid);

	return clsid_str;
}


PHPAPI HRESULT php_COM_destruct(comval *obj TSRMLS_DC)
{
	HRESULT hr = S_OK;

	if (C_ISREFD(obj)) {
		C_REFCOUNT(obj) = 1;
		hr = php_COM_release(obj TSRMLS_CC);
	}
	efree(obj);

	return hr;
}


static void php_comval_destructor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	php_COM_destruct(rsrc->ptr TSRMLS_CC);
}


static PHP_INI_MH(OnTypelibFileChange)
{
	FILE *typelib_file;
	char *typelib_name_buffer;
	char *strtok_buf = NULL;
	int interactive;
	interactive = CG(interactive);

	if (!new_value || (typelib_file = VCWD_FOPEN(new_value, "r"))==NULL) {
		return FAILURE;
	}

	if (interactive) {
		printf("Loading type libraries...");
		fflush(stdout);
	}

	typelib_name_buffer = (char *) emalloc(sizeof(char)*1024);

	while(fgets(typelib_name_buffer, 1024, typelib_file)) {
		ITypeLib *pTL;
		char *typelib_name;
		char *modifier, *ptr;
		int mode = CONST_CS | CONST_PERSISTENT;	/* CONST_PERSISTENT is ok here */

		if (typelib_name_buffer[0]==';') {
			continue;
		}
		typelib_name = php_strtok_r(typelib_name_buffer, "\r\n", &strtok_buf); /* get rid of newlines */
		if (typelib_name == NULL) {
			continue;
		}
		typelib_name = php_strtok_r(typelib_name, "#", &strtok_buf);
		modifier = php_strtok_r(NULL, "#", &strtok_buf);
		if (modifier != NULL) {
			if (!strcmp(modifier, "cis") || !strcmp(modifier, "case_insensitive")) {
				mode &= ~CONST_CS;
			}
		}

		/* Remove leading/training white spaces on search_string */
		while(isspace(*typelib_name)) {/* Ends on '\0' in worst case */
			typelib_name ++;
		}
		ptr = typelib_name + strlen(typelib_name) - 1;
		while((ptr != typelib_name) && isspace(*ptr)) {
			*ptr = '\0';
			ptr--;
		}

		if (interactive) {
			printf("\rLoading %-60s\r", typelib_name);
		}
		
		if ((pTL = php_COM_find_typelib(typelib_name, mode TSRMLS_CC)) != NULL) {
			php_COM_load_typelib(pTL, mode TSRMLS_CC);
			pTL->lpVtbl->Release(pTL);
		}
	}

	efree(typelib_name_buffer);
	fclose(typelib_file);

	if (interactive) {
		printf("\r%70s\r", "");
	}

	return SUCCESS;
}


PHP_INI_BEGIN()
PHP_INI_ENTRY_EX("com.allow_dcom", "0", PHP_INI_SYSTEM, NULL, php_ini_boolean_displayer_cb)
PHP_INI_ENTRY_EX("com.autoregister_typelib", "0", PHP_INI_SYSTEM, NULL, php_ini_boolean_displayer_cb)
PHP_INI_ENTRY_EX("com.autoregister_verbose", "0", PHP_INI_SYSTEM, NULL, php_ini_boolean_displayer_cb)
PHP_INI_ENTRY_EX("com.autoregister_casesensitive", "1", PHP_INI_SYSTEM, NULL, php_ini_boolean_displayer_cb)
PHP_INI_ENTRY("com.typelib_file", "", PHP_INI_SYSTEM, OnTypelibFileChange)
PHP_INI_END()


/* {{{ proto int com_load(string module_name [, string remote_host [, int codepage [, string typelib]]])
   Loads a COM module */
PHP_FUNCTION(com_load)
{
	pval *module_name, *code_page, *typelib = NULL, *server_name = NULL;
	CLSID clsid;
	HRESULT hr;
	OLECHAR *ProgID;
	comval *obj;
	char *error_message;
	char *clsid_str;
	int mode = 0;
	ITypeLib *pTL;


	codepage = CP_ACP;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			zend_get_parameters(ht, 1, &module_name);
			break;

		case 2:
			zend_get_parameters(ht, 2, &module_name, &server_name);
			break;

		case 3:
			zend_get_parameters(ht, 3, &module_name, &server_name, &code_page);

			convert_to_long_ex(&code_page);
			codepage = code_page->value.lval;
			break;

		case 4:
			zend_get_parameters(ht, 4, &module_name, &server_name, &code_page, &typelib);

			convert_to_string_ex(&typelib);
			convert_to_long_ex(&code_page);
			codepage = Z_LVAL_P(code_page);

			break;

		default:
			ZEND_WRONG_PARAM_COUNT();
	}

	if (server_name != NULL)
	{
		if (Z_TYPE_P(server_name) == IS_NULL) {
			server_name = NULL;
		} else {
			if (!INI_INT("com.allow_dcom")) {
				php_error(E_WARNING, "DCOM is disabled");
				RETURN_FALSE;
			} else {
				convert_to_string_ex(&server_name);
			}
		}
	}

	ALLOC_COM(obj);
	convert_to_string_ex(&module_name);
	ProgID = php_char_to_OLECHAR(Z_STRVAL_P(module_name), Z_STRLEN_P(module_name), codepage TSRMLS_CC);

	/* obtain CLSID */
	if (FAILED(CLSIDFromString(ProgID, &clsid))) {
		/* Perhaps this is a Moniker? */
		IBindCtx *pBindCtx;
		IMoniker *pMoniker;
		ULONG ulEaten;

		/* @todo if (server_name) */

		if (!server_name) {
			/* @todo shouldn't the bind context be fetched on module startup and kept as a global shared instance ?
			 * all calls to BindToObject would deliver the same instance then (as desired)
			 * IBindCtx::RegisterObjectBound() should be called then after  mkparsedisplayname()
			 *
			 * @todo use mkparsedisplaynameex() ?
			 */ 
			if (SUCCEEDED(hr = CreateBindCtx(0, &pBindCtx))) {
				if (SUCCEEDED(hr = MkParseDisplayName(pBindCtx, ProgID, &ulEaten, &pMoniker))) {
					hr = pMoniker->lpVtbl->BindToObject(pMoniker, pBindCtx, NULL, &IID_IDispatch, (LPVOID *) &C_DISPATCH(obj));
					pMoniker->lpVtbl->Release(pMoniker);
				}
				pBindCtx->lpVtbl->Release(pBindCtx);
			}
		} else {
			hr = MK_E_SYNTAX;
		}
		
		efree(ProgID);
		
		if (FAILED(hr)) {
			php_COM_destruct(obj TSRMLS_CC);
			error_message = php_COM_error_message(hr TSRMLS_CC);  
			php_error(E_WARNING,"Invalid ProgID, GUID string, or Moniker: %s", error_message);
			LocalFree(error_message);
			RETURN_FALSE;
		}
	} else {
		efree(ProgID);
		/* obtain IDispatch */
		if (!server_name) {
			hr = CoCreateInstance(&clsid, NULL, CLSCTX_SERVER, &IID_IDispatch, (LPVOID *) &C_DISPATCH(obj));
		} else {
			COSERVERINFO server_info;
			MULTI_QI pResults;

			server_info.dwReserved1=0;
			server_info.dwReserved2=0;
			server_info.pwszName = php_char_to_OLECHAR(Z_STRVAL_P(server_name), Z_STRLEN_P(server_name), codepage TSRMLS_CC);
			server_info.pAuthInfo=NULL;

			pResults.pIID = &IID_IDispatch;
			pResults.pItf = NULL;
			pResults.hr = S_OK;
			hr=CoCreateInstanceEx(&clsid, NULL, CLSCTX_REMOTE_SERVER, &server_info, 1, &pResults);
			if (SUCCEEDED(hr)) {
				hr = pResults.hr;
				C_DISPATCH(obj) = (IDispatch *) pResults.pItf;
			}
			efree(server_info.pwszName);
		}

		if (FAILED(hr)) {
			error_message = php_COM_error_message(hr TSRMLS_CC);
			clsid_str = php_string_from_clsid(&clsid TSRMLS_CC);
			php_error(E_WARNING,"Unable to obtain IDispatch interface for CLSID %s: %s",clsid_str,error_message);
			LocalFree(error_message);
			efree(clsid_str);
			php_COM_destruct(obj TSRMLS_CC);
			RETURN_FALSE;
		}
	}

	php_COM_set(obj, &C_DISPATCH(obj), TRUE TSRMLS_CC);

	if (INI_INT("com.autoregister_casesensitive")) {
		mode |= CONST_CS;
	}

	if (C_HASTLIB(obj)) {
		if (INI_INT("com.autoregister_typelib")) {
			unsigned int idx;

			/* @todo check if typlib isn't already loaded */
			if (C_TYPEINFO_VT(obj)->GetContainingTypeLib(C_TYPEINFO(obj), &pTL, &idx) == S_OK) {
				php_COM_load_typelib(pTL, mode TSRMLS_CC);
				pTL->lpVtbl->Release(pTL);
			}
		}
	} else {
		if (typelib != NULL) {
			ITypeLib *pTL;

			if ((pTL = php_COM_find_typelib(Z_STRVAL_P(typelib), mode TSRMLS_CC)) != NULL) {
				C_HASTLIB(obj) = SUCCEEDED(pTL->lpVtbl->GetTypeInfo(pTL, 0, &C_TYPEINFO(obj)));
				/* idx 0 should deliver the ITypeInfo for the IDispatch Interface */
				if (INI_INT("com.autoregister_typelib")) {
					php_COM_load_typelib(pTL, mode TSRMLS_CC);
				}
				pTL->lpVtbl->Release(pTL);
			}
		}
	}

	RETURN_LONG(zend_list_insert(obj, IS_COM));
}
/* }}} */


int do_COM_invoke(comval *obj, pval *function_name, VARIANT *var_result, pval **arguments, int arg_count TSRMLS_DC)
{
	DISPID dispid;
	HRESULT hr;
	OLECHAR *funcname;
	char *error_message;
	VARIANT *variant_args;
	int current_arg, current_variant;
	DISPPARAMS dispparams;
	SAFEARRAY *pSA;

	if (C_HASENUM(obj) && strstr(Z_STRVAL_P(function_name), "next")) {
		/* Grab one argument off the stack, allocate enough
		 * VARIANTs
		 * Get the IEnumVariant interface and call ->Next();
		 */
		SAFEARRAYBOUND rgsabound[1];
		unsigned long count;

		switch (arg_count) {
			case 0:
				count = 1;
				break;

			case 1:
				convert_to_long_ex(&arguments[0]);
				count = Z_LVAL_P(arguments[0]);
				break;

			default:
				php_error(E_WARNING,"Wrong argument count to IEnumVariant::Next()");

				return FAILURE;
		}

		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = count;

		if ((pSA = SafeArrayCreate(VT_VARIANT, 1, rgsabound)) == NULL) {
			VariantInit(var_result);

			return FAILURE;
		} else {
			V_ARRAY(var_result) = pSA;
			V_VT(var_result) = VT_VARIANT|VT_ARRAY;
		}

		if (FAILED(hr = C_ENUMVARIANT_VT(obj)->Next(C_ENUMVARIANT(obj), count, pSA->pvData, &count))) {
			char *error_message = php_COM_error_message(hr TSRMLS_CC);
			php_error(E_WARNING,"IEnumVariant::Next() failed: %s", error_message);
			efree(error_message);
			VariantClear(var_result);
			return FAILURE;
		}

		if (count != rgsabound[0].cElements) {
			rgsabound[0].cElements = count;
			if (FAILED(SafeArrayRedim(pSA, rgsabound))) {
				char *error_message = php_COM_error_message(hr TSRMLS_CC);
				php_error(E_WARNING,"IEnumVariant::Next() failed: %s", error_message);
				efree(error_message);
				VariantClear(var_result);
				return FAILURE;
			}
		}

		/* return a single element if next() was called without count */
		if ((arg_count == 0) && (count == 1)) {
			long index[] = {0};

			SafeArrayGetElement(pSA, index, var_result);
			SafeArrayDestroy(pSA);
		}

		return SUCCESS;
	} else if (C_HASENUM(obj) && strstr(Z_STRVAL_P(function_name), "reset")) {
		if (FAILED(hr = C_ENUMVARIANT_VT(obj)->Reset(C_ENUMVARIANT(obj)))) {
			char *error_message = php_COM_error_message(hr TSRMLS_CC);
			php_error(E_WARNING,"IEnumVariant::Next() failed: %s", error_message);
			efree(error_message);
			return FAILURE;
		}
		return SUCCESS;
	} else if (C_HASENUM(obj) && strstr(Z_STRVAL_P(function_name), "skip")) {
		unsigned long count;

		switch (arg_count) {
			case 0:
				count = 1;
				break;

			case 1:
				convert_to_long_ex(&arguments[0]);
				count = Z_LVAL_P(arguments[0]);
				break;

			default:
				php_error(E_WARNING,"Wrong argument count to IEnumVariant::Skip()");
				return FAILURE;
		}
		if (FAILED(hr = C_ENUMVARIANT_VT(obj)->Skip(C_ENUMVARIANT(obj), count))) {
			char *error_message = php_COM_error_message(hr TSRMLS_CC);
			php_error(E_WARNING,"IEnumVariant::Next() failed: %s", error_message);
			efree(error_message);
			return FAILURE;
		}
		return SUCCESS;

	} else {
		char *ErrString;

		funcname = php_char_to_OLECHAR(Z_STRVAL_P(function_name), Z_STRLEN_P(function_name), codepage TSRMLS_CC);

		hr = php_COM_get_ids_of_names(obj, &funcname, &dispid TSRMLS_CC);

		if (FAILED(hr)) {
			error_message = php_COM_error_message(hr TSRMLS_CC);
			php_error(E_WARNING,"Unable to lookup %s: %s", Z_STRVAL_P(function_name), error_message);
			LocalFree(error_message);
			efree(funcname);
			return FAILURE;
		}

		variant_args = (VARIANT *) emalloc(sizeof(VARIANT) * arg_count);

		for (current_arg=0; current_arg<arg_count; current_arg++) {
			current_variant = arg_count - current_arg - 1;
			php_pval_to_variant(arguments[current_arg], &variant_args[current_variant], codepage TSRMLS_CC);
		}

		dispparams.rgvarg = variant_args;
		dispparams.rgdispidNamedArgs = NULL;
		dispparams.cArgs = arg_count;
		dispparams.cNamedArgs = 0;

		hr = php_COM_invoke(obj, dispid, DISPATCH_METHOD|DISPATCH_PROPERTYGET, &dispparams, var_result, &ErrString TSRMLS_CC);

		efree(funcname);
		for (current_arg=0;current_arg<arg_count;current_arg++) {
			/* don't release IDispatch pointers as they are used afterwards */
			if (V_VT(&variant_args[current_arg]) != VT_DISPATCH) {
				/* @todo review this: what happens to an array of IDispatchs or a VARIANT->IDispatch */
				VariantClear(&variant_args[current_arg]);
			}
		}
		efree(variant_args);

		if (FAILED(hr)) {
			error_message = php_COM_error_message(hr TSRMLS_CC);
			if (ErrString) {
				php_error(E_WARNING,"Invoke() failed: %s %s", error_message, ErrString);
				pefree(ErrString, 1);
			} else {
				php_error(E_WARNING,"Invoke() failed: %s", error_message);
			}
			LocalFree(error_message);
			return FAILURE;
		}
	}
	return SUCCESS;
}


/* {{{ proto mixed com_invoke(int module, string handler_name [, mixed arg [, ...]])
   Invokes a COM module */
PHP_FUNCTION(com_invoke)
{
	pval **arguments;
	pval *object, *function_name;
	comval *obj;
	int type;
	int arg_count = ZEND_NUM_ARGS();
	VARIANT *var_result;

	if (arg_count<2) {
		ZEND_WRONG_PARAM_COUNT();
	}
	arguments = (pval **) emalloc(sizeof(pval *)*arg_count);
	if (zend_get_parameters_array(ht, arg_count, arguments) == FAILURE) {
		RETURN_FALSE;
	}

	object = arguments[0];
	function_name = arguments[1];

	/* obtain IDispatch interface */
	convert_to_long(object);
	obj = (comval *)zend_list_find(Z_LVAL_P(object), &type);
	if (!obj || (type != IS_COM)) {
		php_error(E_WARNING,"%d is not a COM object handler", Z_STRVAL_P(function_name));
		RETURN_FALSE;
	}

	/* obtain property/method handler */
	convert_to_string_ex(&function_name);

	ALLOC_VARIANT(var_result);

	if (do_COM_invoke(obj, function_name, var_result, arguments+2, arg_count-2 TSRMLS_CC)==FAILURE) {
		FREE_VARIANT(var_result);
		efree(arguments);
	
		RETURN_FALSE;
	}

	RETVAL_VARIANT(var_result);

	efree(arguments);
}
/* }}} */


/* {{{ proto mixed com_invoke(int module)
   Releases a COM object */
PHP_FUNCTION(com_release)
{
	pval *object;
	comval *obj;
	int type;
	int arg_count = ZEND_NUM_ARGS();

	if (arg_count != 1) {
		ZEND_WRONG_PARAM_COUNT();
	}

	if (zend_get_parameters(ht, 1, &object)==FAILURE) {
		RETURN_FALSE;
	}

	/* obtain IDispatch interface */
	convert_to_long_ex(&object);
	obj = (comval *)zend_list_find(Z_LVAL_P(object), &type);
	if (!obj || (type != IS_COM)) {
		php_error(E_WARNING,"%d is not a COM object handler");
		RETURN_FALSE;
	}

	RETURN_LONG(php_COM_release(obj TSRMLS_CC))
}
/* }}} */


/* {{{ proto mixed com_addref(int module)
   Increases the reference counter on a COM object */
PHP_FUNCTION(com_addref)
{
	pval *object;
	comval *obj;
	int type;
	int arg_count = ZEND_NUM_ARGS();

	if (arg_count != 1) {
		ZEND_WRONG_PARAM_COUNT();
	}

	if (zend_get_parameters(ht, 1, &object)==FAILURE) {
		RETURN_FALSE;
	}

	/* obtain IDispatch interface */
	convert_to_long_ex(&object);
	obj = (comval *)zend_list_find(Z_LVAL_P(object), &type);
	if (!obj || (type != IS_COM))
	{
		php_error(E_WARNING,"%d is not a COM object handler");
		RETURN_FALSE;
	}

	RETURN_LONG(php_COM_addref(obj TSRMLS_CC));
}
/* }}} */


static int do_COM_offget(VARIANT *result, comval *array, pval *property, int cleanup TSRMLS_DC)
{
	pval function_name;
	int retval;

	ZVAL_STRINGL(&function_name, "Item", 4, 0);
	retval = do_COM_invoke(array, &function_name, result, &property, 1 TSRMLS_CC);
	if (cleanup) {
		php_COM_destruct(array TSRMLS_CC);
	}

	return retval;
}


static int do_COM_propget(VARIANT *var_result, comval *obj, pval *arg_property, int cleanup TSRMLS_DC)
{
	DISPID dispid;
	HRESULT hr;
	OLECHAR *propname;
	char *error_message;
	DISPPARAMS dispparams;
	char *ErrString;

	/* obtain property handler */
	propname = php_char_to_OLECHAR(Z_STRVAL_P(arg_property), Z_STRLEN_P(arg_property), codepage TSRMLS_CC);

	hr = php_COM_get_ids_of_names(obj, &propname, &dispid TSRMLS_CC);

	if (FAILED(hr)) {
		error_message = php_COM_error_message(hr TSRMLS_CC);
		php_error(E_WARNING,"Unable to lookup %s: %s", Z_STRVAL_P(arg_property), error_message);
		LocalFree(error_message);
		efree(propname);
		if (cleanup) {
			php_COM_destruct(obj TSRMLS_CC);
		}
		return FAILURE;
	}

	dispparams.cArgs = 0;
	dispparams.cNamedArgs = 0;

	hr = php_COM_invoke(obj, dispid, DISPATCH_PROPERTYGET, &dispparams, var_result, &ErrString TSRMLS_CC);

	if (FAILED(hr)) {
		error_message = php_COM_error_message(hr TSRMLS_CC);
		if (ErrString) {
			php_error(E_WARNING,"PropGet() failed: %s %s", error_message, ErrString);
			pefree(ErrString, 1);
		} else {
			php_error(E_WARNING,"PropGet() failed: %s", error_message);
		}
		LocalFree(error_message);
		efree(propname);
		if (cleanup) {
			php_COM_destruct(obj TSRMLS_CC);
		}
		return FAILURE;
	}

	efree(propname);
	if (cleanup) {
		php_COM_destruct(obj TSRMLS_CC);
	}
	return SUCCESS;
}


static void do_COM_propput(pval *return_value, comval *obj, pval *arg_property, pval *value TSRMLS_DC)
{
	DISPID dispid;
	HRESULT hr;
	OLECHAR *propname;
	char *error_message;
	VARIANT *var_result, *new_value;
	DISPPARAMS dispparams;
	DISPID mydispid = DISPID_PROPERTYPUT;
	char *ErrString;

	ALLOC_VARIANT(var_result);
	ALLOC_VARIANT(new_value);

	/* obtain property handler */
	propname = php_char_to_OLECHAR(Z_STRVAL_P(arg_property), Z_STRLEN_P(arg_property), codepage TSRMLS_CC);

	hr = php_COM_get_ids_of_names(obj, &propname, &dispid TSRMLS_CC);

	if (FAILED(hr)) {
		error_message = php_COM_error_message(hr TSRMLS_CC);
		php_error(E_WARNING,"Unable to lookup %s: %s", Z_STRVAL_P(arg_property), error_message);
		LocalFree(error_message);
		efree(propname);

		FREE_VARIANT(var_result);
		FREE_VARIANT(new_value);

		RETURN_FALSE;
	}

	php_pval_to_variant(value, new_value, codepage TSRMLS_CC);
	dispparams.rgvarg = new_value;
	dispparams.rgdispidNamedArgs = &mydispid;
	dispparams.cArgs = 1;
	dispparams.cNamedArgs = 1;

	hr = php_COM_invoke(obj, dispid, DISPATCH_PROPERTYPUT, &dispparams, NULL, &ErrString TSRMLS_CC);

	if (FAILED(hr)) {
		error_message = php_COM_error_message(hr TSRMLS_CC);
		if (ErrString) {
			php_error(E_WARNING,"PropPut() failed: %s %s", error_message, ErrString);
			pefree(ErrString, 1);
		} else {
			php_error(E_WARNING,"PropPut() failed: %s", error_message);
		}
		LocalFree(error_message);
		efree(propname);

		FREE_VARIANT(var_result);
		FREE_VARIANT(new_value);

		RETURN_FALSE;
	}

	dispparams.cArgs = 0;
	dispparams.cNamedArgs = 0;

	hr = php_COM_invoke(obj, dispid, DISPATCH_PROPERTYGET, &dispparams, var_result, &ErrString TSRMLS_CC);

	if (SUCCEEDED(hr)) {
		php_variant_to_pval(var_result, return_value, codepage TSRMLS_CC);
	} else {
		*return_value = *value;
		zval_copy_ctor(return_value);
	}

	if (ErrString) {
		pefree(ErrString, 1);
	}

	FREE_VARIANT(var_result);
	FREE_VARIANT(new_value);

	efree(propname);
}


/* {{{ proto mixed com_propget(int module, string property_name)
   Gets properties from a COM module */
PHP_FUNCTION(com_propget)
{
	pval *arg_comval, *arg_property;
	int type;
	comval *obj;
	VARIANT *var_result;

	if ((ZEND_NUM_ARGS() != 2) || (zend_get_parameters(ht, 2, &arg_comval, &arg_property) == FAILURE)) {
		ZEND_WRONG_PARAM_COUNT();
	}

	/* obtain IDispatch interface */
	convert_to_long(arg_comval);
	obj = (comval *)zend_list_find(Z_LVAL_P(arg_comval), &type);
	if (!obj || (type != IS_COM)) {
		php_error(E_WARNING,"%d is not a COM object handler", Z_LVAL_P(arg_comval));
	}
	convert_to_string_ex(&arg_property);

	ALLOC_VARIANT(var_result);

	if (do_COM_propget(var_result, obj, arg_property, FALSE TSRMLS_CC) == FAILURE) {
		FREE_VARIANT(var_result);
		RETURN_FALSE;
	}

	RETVAL_VARIANT(var_result);
}
/* }}} */


/* {{{ proto bool com_propput(int module, string property_name, mixed value)
   Puts the properties for a module */
PHP_FUNCTION(com_propput)
{
	pval *arg_comval, *arg_property, *arg_value;
	int type;
	comval *obj;

	if (ZEND_NUM_ARGS()!=3 || zend_get_parameters(ht, 3, &arg_comval, &arg_property, &arg_value)==FAILURE) {
		ZEND_WRONG_PARAM_COUNT();
	}

	/* obtain comval interface */
	convert_to_long(arg_comval);
	/* obtain comval interface */
	obj = (comval *)zend_list_find(Z_LVAL_P(arg_comval), &type);
	if (!obj || (type != IS_COM)) {
		php_error(E_WARNING,"%d is not a COM object handler", Z_LVAL_P(arg_comval));
	}
	convert_to_string_ex(&arg_property);

	do_COM_propput(return_value, obj, arg_property, arg_value TSRMLS_CC);
}
/* }}} */


/* {{{ proto bool com_load_typelib(string typelib_name [, int case_insensitiv]) 
   Loads a Typelib */
PHP_FUNCTION(com_load_typelib)
{
	pval *arg_typelib, *arg_cis;
	ITypeLib *pTL;
	int mode = CONST_CS;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			zend_get_parameters(ht, 1, &arg_typelib);
			break;

		case 2:
			zend_get_parameters(ht, 2, &arg_typelib, &arg_cis);
			convert_to_boolean_ex(&arg_cis);
			if (arg_cis->value.lval) {
				mode &= ~CONST_CS;
			}
			break;

		default:
			ZEND_WRONG_PARAM_COUNT();
	}

	convert_to_string_ex(&arg_typelib);
	pTL = php_COM_find_typelib(Z_STRVAL_P(arg_typelib), mode TSRMLS_CC);
	if (php_COM_load_typelib(pTL, mode TSRMLS_CC) == SUCCESS) {
		pTL->lpVtbl->Release(pTL);
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


PHPAPI pval php_COM_get_property_handler(zend_property_reference *property_reference)
{
	zend_overloaded_element *overloaded_property;
	zend_llist_element *element;
	pval return_value;
	pval **comval_handle;
	pval *object = property_reference->object;
	int type;
	comval *obj, *obj_prop;
	VARIANT *var_result;
	TSRMLS_FETCH();

	INIT_ZVAL(return_value);    
	ZVAL_NULL(&return_value);

	/* fetch the IDispatch interface */
	zend_hash_index_find(Z_OBJPROP_P(object), 0, (void **) &comval_handle);
	obj = (comval *) zend_list_find(Z_LVAL_P(*comval_handle), &type);
	if (!obj || (type != IS_COM)) {
		return return_value;
	}

	ALLOC_COM(obj_prop);
	ALLOC_VARIANT(var_result);

	for (element=property_reference->elements_list->head; element; element=element->next) {
		overloaded_property = (zend_overloaded_element *) element->data;
		switch (overloaded_property->type) {
			case OE_IS_ARRAY:
				if (do_COM_offget(var_result, obj, &overloaded_property->element, FALSE TSRMLS_CC) == FAILURE) {
					FREE_VARIANT(var_result);
					FREE_COM(obj_prop);

					return return_value;
				}
				break;

			case OE_IS_OBJECT:
				if (do_COM_propget(var_result, obj, &overloaded_property->element, FALSE TSRMLS_CC) == FAILURE) {
					FREE_VARIANT(var_result);
					FREE_COM(obj_prop);

					return return_value;
				}
				break;

			case OE_IS_METHOD: {
					FREE_VARIANT(var_result);
					if (obj != obj_prop) {
						FREE_COM(obj_prop);
	
						return_value = *object;
						ZVAL_ADDREF(&return_value);
					} else {
						RETVAL_COM(obj);
					}
					return return_value;
				}
				break;
		}

		if (V_VT(var_result) == VT_DISPATCH) {
			if (V_DISPATCH(var_result) == NULL) {
				FREE_VARIANT(var_result);
				FREE_COM(obj_prop);
				
				return return_value;
			}

			obj = obj_prop;
			php_COM_set(obj, &V_DISPATCH(var_result), TRUE TSRMLS_CC);
		} else {
			php_variant_to_pval(var_result, &return_value, codepage TSRMLS_CC);

			FREE_COM(obj_prop);
			obj_prop = NULL;
		}

		VariantInit(var_result);	// to protect C_DISPATCH(obj) from being freed when var_result is destructed
		pval_destructor(&overloaded_property->element);
	}

	if (obj_prop != NULL) {
		RETVAL_COM(obj);
	}

	FREE_VARIANT(var_result);              

	return return_value;
}


PHPAPI int php_COM_set_property_handler(zend_property_reference *property_reference, pval *value)
{
	pval result;
	zend_overloaded_element *overloaded_property;
	zend_llist_element *element;
	pval **comval_handle;
	pval *object = property_reference->object;
	comval *obj, *obj_prop;
	int type;
	VARIANT *var_result;
	TSRMLS_FETCH();

	/* fetch the IDispatch interface */
	zend_hash_index_find(Z_OBJPROP_P(object), 0, (void **) &comval_handle);
	obj = (comval *)zend_list_find(Z_LVAL_P(*comval_handle), &type);
	if (!obj || (type != IS_COM)) {
		return FAILURE;
	}
	
	ALLOC_COM(obj_prop);
	ALLOC_VARIANT(var_result);

	for (element=property_reference->elements_list->head; element != property_reference->elements_list->tail; element=element->next) {
		overloaded_property = (zend_overloaded_element *) element->data;
		switch (overloaded_property->type) {
			case OE_IS_ARRAY:
				if (do_COM_offget(var_result, obj, &overloaded_property->element, FALSE TSRMLS_CC) == FAILURE) {
					FREE_VARIANT(var_result);
					FREE_COM(obj_prop);

					return FAILURE;
				}
				break;

			case OE_IS_OBJECT:
				if (do_COM_propget(var_result, obj, &overloaded_property->element, FALSE TSRMLS_CC) == FAILURE) {
					FREE_VARIANT(var_result);
					FREE_COM(obj_prop);

					return FAILURE;
				}
				break;

			case OE_IS_METHOD:
				/* this shouldn't happen */
				return FAILURE;
				break;
		}

		if (V_VT(var_result) == VT_DISPATCH) {
			if (V_DISPATCH(var_result) == NULL) {
				FREE_VARIANT(var_result);
				FREE_COM(obj_prop);

				return FAILURE;
			}

			obj = obj_prop;
			php_COM_set(obj, &V_DISPATCH(var_result), TRUE TSRMLS_CC);
		} else {
			FREE_COM(obj_prop);
			FREE_VARIANT(var_result);

			return FAILURE;
		}

		VariantInit(var_result);	// to protect C_DISPATCH(obj) from being freed when var_result is destructed
		pval_destructor(&overloaded_property->element);
	}
	FREE_VARIANT(var_result);
	
	overloaded_property = (zend_overloaded_element *) element->data;
	do_COM_propput(&result, obj, &overloaded_property->element, value TSRMLS_CC);
	FREE_COM(obj_prop);

	pval_destructor(&overloaded_property->element);

	return SUCCESS;
}


PHPAPI void php_COM_call_function_handler(INTERNAL_FUNCTION_PARAMETERS, zend_property_reference *property_reference)
{
	pval property, **handle;
	pval *object = property_reference->object;
	zend_overloaded_element *function_name = (zend_overloaded_element *) property_reference->elements_list->tail->data;
	comval *obj;
	int type;

	if (zend_llist_count(property_reference->elements_list)==1
		&& !strcmp(Z_STRVAL(function_name->element), "com")) {
		/* constructor */
		pval *object_handle;

		PHP_FN(com_load)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
		if (!zend_is_true(return_value)) {
			ZVAL_FALSE(object);
			return;
		}
		ALLOC_ZVAL(object_handle);
		*object_handle = *return_value;
		pval_copy_constructor(object_handle);
		INIT_PZVAL(object_handle);
		zend_hash_index_update(Z_OBJPROP_P(object), 0, &object_handle, sizeof(pval *), NULL);
		pval_destructor(&function_name->element);

		return;
	}

	property = php_COM_get_property_handler(property_reference);
	if (property.type == IS_NULL) {
		if (property.refcount == 1) {
			pval_destructor(&property);
		}
		pval_destructor(&function_name->element);
		return;
	}
	zend_hash_index_find(Z_OBJPROP(property), 0, (void **) &handle);
	obj = (comval *)zend_list_find(Z_LVAL_PP(handle), &type);

	if (!obj || (type != IS_COM)) {
		pval_destructor(&property);
		pval_destructor(&function_name->element);
		return;
	}

	if (zend_llist_count(property_reference->elements_list)==1
		&& !strcmp(Z_STRVAL_P(&function_name->element), "release")) {
		RETVAL_LONG(php_COM_release(obj TSRMLS_CC));
	} else if (zend_llist_count(property_reference->elements_list)==1
		&& !strcmp(Z_STRVAL_P(&function_name->element), "addref")) {
		RETVAL_LONG(php_COM_addref(obj TSRMLS_CC));
	} else {
		pval **arguments;
		VARIANT *var_result;
		int arg_count = ZEND_NUM_ARGS();

		ALLOC_VARIANT(var_result);

		arguments = (pval **) emalloc(sizeof(pval *)*arg_count);
		zend_get_parameters_array(ht, arg_count, arguments);

		if (do_COM_invoke(obj , &function_name->element, var_result, arguments, arg_count TSRMLS_CC) == FAILURE) {
			RETVAL_FALSE;
		} else {
			php_variant_to_pval(var_result, return_value, codepage TSRMLS_CC);
		}

		FREE_VARIANT(var_result);
		efree(arguments);
	}

	if (property.refcount == 1) {
		pval_destructor(&property);
	}
	pval_destructor(&function_name->element);
}


static ITypeLib *php_COM_find_typelib(char *search_string, int mode TSRMLS_DC)
{
	ITypeLib *TypeLib = NULL;
	char *strtok_buf, *major, *minor;
	CLSID clsid;
	OLECHAR *p;

	/* Type Libraries:
	 * The string we have is either:
	 * a) a file name
	 * b) a CLSID, major, minor e.g. "{00000200-0000-0010-8000-00AA006D2EA4},2,0"
	 * c) a Type Library name e.g. "Microsoft OLE DB ActiveX Data Objects 1.0 Library"
	 * Searching for the name will be more expensive that the
	 * other two, so we will do that when both other attempts
	 * fail.
	 */

	search_string = php_strtok_r(search_string, ",", &strtok_buf);
	major = php_strtok_r(NULL, ",", &strtok_buf);
	minor = php_strtok_r(NULL, ",", &strtok_buf);

	p = php_char_to_OLECHAR(search_string, strlen(search_string), codepage TSRMLS_CC);
	/* Is the string a GUID ? */

	if (!FAILED(CLSIDFromString(p, &clsid))) {
		HRESULT hr;
		WORD major_i = 1;
		WORD minor_i = 0;

		/* We have a valid GUID, check to see if a major/minor */
		/* version was specified otherwise assume 1,0 */
		if ((major != NULL) && (minor != NULL)) {
			major_i = (WORD) atoi(major);
			minor_i = (WORD) atoi(minor);
		}

		/* The GUID will either be a typelibrary or a CLSID */
		hr = LoadRegTypeLib((REFGUID) &clsid, major_i, minor_i, LANG_NEUTRAL, &TypeLib);

		/* If the LoadRegTypeLib fails, let's try to instantiate */
		/* the class itself and then QI for the TypeInfo and */
		/* retrieve the type info from that interface */
		if (FAILED(hr) && (!major || !minor)) {
			IDispatch *Dispatch;
			ITypeInfo *TypeInfo;
			int idx;

			if (FAILED(CoCreateInstance(&clsid, NULL, CLSCTX_SERVER, &IID_IDispatch, (LPVOID *) &Dispatch))) {
				efree(p);
				return NULL;
			}
			if (FAILED(Dispatch->lpVtbl->GetTypeInfo(Dispatch, 0, LANG_NEUTRAL, &TypeInfo))) {
				Dispatch->lpVtbl->Release(Dispatch);
				efree(p);
				return NULL;
			}
			Dispatch->lpVtbl->Release(Dispatch);
			if (FAILED(TypeInfo->lpVtbl->GetContainingTypeLib(TypeInfo, &TypeLib, &idx))) {
				TypeInfo->lpVtbl->Release(TypeInfo);
				efree(p);
				return NULL;
			}
			TypeInfo->lpVtbl->Release(TypeInfo);
		}
	} else {
		if (FAILED(LoadTypeLib(p, &TypeLib))) {
			/* Walk HKCR/TypeLib looking for the string */
			/* If that succeeds, call ourself recursively */
			/* using the CLSID found, else give up and bail */
			HKEY hkey, hsubkey;
			DWORD SubKeys, MaxSubKeyLength;
			char *keyname;
			register unsigned int ii, jj;
			DWORD VersionCount;
			char version[20]; /* All the version keys are 1.0, 4.6, ... */
			char *libname;
			DWORD libnamelen;

			/* No Need for Unicode version any more */
			efree(p);

			/* Starting at HKEY_CLASSES_ROOT/TypeLib */
			/* Walk all subkeys (Typelib GUIDs) looking */
			/* at each version for a string match to the */
			/* supplied argument */

			if (ERROR_SUCCESS != RegOpenKey(HKEY_CLASSES_ROOT, "TypeLib",&hkey)) {
				/* This is pretty bad - better bail */
				return NULL;
			}
			if (ERROR_SUCCESS != RegQueryInfoKey(hkey, NULL, NULL, NULL, &SubKeys, &MaxSubKeyLength, NULL, NULL, NULL, NULL, NULL, NULL)) {
				RegCloseKey(hkey);
				return NULL;
			}
			MaxSubKeyLength++; /* \0 is not counted */
			keyname = emalloc(MaxSubKeyLength);
			libname = emalloc(strlen(search_string)+1);
			for (ii=0;ii<SubKeys;ii++) {
				if (ERROR_SUCCESS != RegEnumKey(hkey, ii, keyname, MaxSubKeyLength)) {
					/* Failed - who cares */
					continue;
				}
				if (ERROR_SUCCESS != RegOpenKey(hkey, keyname, &hsubkey)) {
					/* Failed - who cares */
					continue;
				}
				if (ERROR_SUCCESS != RegQueryInfoKey(hsubkey, NULL, NULL, NULL, &VersionCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
					/* Failed - who cares */
					RegCloseKey(hsubkey);
					continue;
				}
				for (jj=0;jj<VersionCount;jj++) {
					if (ERROR_SUCCESS != RegEnumKey(hsubkey, jj, version, sizeof(version))) {
						/* Failed - who cares */
						continue;
					}
					/* OK we just need to retrieve the default */
					/* value for this key and see if it matches */
					libnamelen = strlen(search_string)+1;
					if (ERROR_SUCCESS == RegQueryValue(hsubkey, version, libname, &libnamelen)) {
						if ((mode & CONST_CS) ? (strcmp(libname, search_string) == 0) : (stricmp(libname, search_string) == 0)) {
							char *str;
							int major, minor;

							/* Found it */
							RegCloseKey(hkey);
							RegCloseKey(hsubkey);

							efree(libname);
							/* We can either open up the "win32" key and find the DLL name */
							/* Or just parse the version string and pass that in */
							/* The version string seems like a more portable solution */
							/* Given that there is a COM on Unix */
							if (2 != sscanf(version, "%d.%d", &major, &minor)) {
								major = 1;
								minor = 0;
							}
							str = emalloc(strlen(keyname)+strlen(version)+20); /* 18 == safety, 2 == extra comma and \0 */
							sprintf(str, "%s,%d,%d", keyname, major, minor);
							efree(keyname);
							TypeLib = php_COM_find_typelib(str, mode TSRMLS_CC);
							efree(str);
							/* This is probbaly much harder to read and follow */
							/* But it is MUCH more effiecient than trying to */
							/* test for errors and leave through a single "return" */
							return TypeLib;
						}
					} else {
						/* Failed - perhaps too small abuffer */
						/* But if too small, then the name does not match */
					}
				}
				RegCloseKey(hsubkey);
			}
			efree(keyname);
			efree(libname);
			return NULL;
		}
	}
	efree(p);
	return TypeLib;
}


static int php_COM_load_typelib(ITypeLib *TypeLib, int mode TSRMLS_DC)
{
	ITypeComp *TypeComp;
	int i;
	int interfaces;

	if (NULL == TypeLib) {
		return FAILURE;
	}

	interfaces = TypeLib->lpVtbl->GetTypeInfoCount(TypeLib);

	TypeLib->lpVtbl->GetTypeComp(TypeLib, &TypeComp);
	for (i=0; i<interfaces; i++) {
		TYPEKIND pTKind;

		TypeLib->lpVtbl->GetTypeInfoType(TypeLib, i, &pTKind);
		if (pTKind==TKIND_ENUM) {
			ITypeInfo *TypeInfo;
			VARDESC *pVarDesc;
			UINT NameCount;
			int j;
#if 0
			BSTR bstr_EnumId;
			char *EnumId;

			TypeLib->lpVtbl->GetDocumentation(TypeLib, i, &bstr_EnumId, NULL, NULL, NULL);
			EnumId = php_OLECHAR_to_char(bstr_EnumId, NULL, codepage);
			printf("Enumeration %d - %s:\n", i, EnumId);
			efree(EnumId);
#endif

			TypeLib->lpVtbl->GetTypeInfo(TypeLib, i, &TypeInfo);

			j=0;
			while(SUCCEEDED(TypeInfo->lpVtbl->GetVarDesc(TypeInfo, j, &pVarDesc))) {
				BSTR bstr_ids;
				zend_constant c;
				zval exists, results;
				char *const_name;

				TypeInfo->lpVtbl->GetNames(TypeInfo, pVarDesc->memid, &bstr_ids, 1, &NameCount);
				if (NameCount!=1) {
					j++;
					continue;
				}
				const_name = php_OLECHAR_to_char(bstr_ids, &c.name_len, codepage TSRMLS_CC);
				c.name = zend_strndup(const_name, c.name_len);
				efree(const_name);
				c.name_len++; /* length should include the NULL */
				SysFreeString(bstr_ids);
				
				/* Before registering the contsnt, let's see if we can find it */
				if (zend_get_constant(c.name, c.name_len-1, &exists TSRMLS_CC)) {
					/* Oops, it already exists. No problem if it is defined as the same value */
					/* Check to see if they are the same */
					if (!compare_function(&results, &c.value, &exists TSRMLS_CC) && INI_INT("com.autoregister_verbose")) {
						php_error(E_WARNING,"Type library value %s is already defined and has a different value", c.name);
					}
					free(c.name);
					j++;
					continue;
				}

				php_variant_to_pval(pVarDesc->lpvarValue, &c.value, codepage TSRMLS_CC);
				if (mode & CONST_PERSISTENT) {
					zval_persist(&c.value TSRMLS_CC);
					mode |= CONST_EFREE_PERSISTENT;
				}
				c.flags = mode;

				zend_register_constant(&c TSRMLS_CC);

				j++;
			}
			TypeInfo->lpVtbl->Release(TypeInfo);
		}
	}

	return SUCCESS;
}


/* {{{ proto bool com_isenum(com_module obj)
   Grabs an IEnumVariant */
PHP_FUNCTION(com_isenum)
{
	pval *object;
	pval **comval_handle;
	comval *obj;
	int type;

	if (ZEND_NUM_ARGS() != 1) {
		ZEND_WRONG_PARAM_COUNT();
	}

	zend_get_parameters(ht, 1, &object);

	/* obtain IDispatch interface */
	zend_hash_index_find(Z_OBJPROP_P(object), 0, (void **) &comval_handle);
	obj = (comval *) zend_list_find(Z_LVAL_PP(comval_handle), &type);
	if (!obj || (type != IS_COM)) {
		php_error(E_WARNING,"%s is not a COM object handler", "");
		RETURN_FALSE;
	}

	RETURN_BOOL(C_HASENUM(obj));
}
/* }}} */


static void php_register_COM_class(TSRMLS_D)
{
	INIT_OVERLOADED_CLASS_ENTRY(COM_class_entry, "COM", NULL,
								php_COM_call_function_handler,
								php_COM_get_property_handler,
								php_COM_set_property_handler);

	zend_register_internal_class(&COM_class_entry TSRMLS_CC);
}


PHP_MINIT_FUNCTION(COM)
{
	le_comval = zend_register_list_destructors_ex(php_comval_destructor, NULL, "COM", module_number);
	php_register_COM_class(TSRMLS_C);
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(COM)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

/* exports for external object creation */

zend_module_entry COM_module_entry = {
	"com", COM_functions, PHP_MINIT(COM), PHP_MSHUTDOWN(COM), NULL, NULL, PHP_MINFO(COM), STANDARD_MODULE_PROPERTIES
};

PHPAPI int php_COM_get_le_comval()
{
	return le_comval;
}

#endif
