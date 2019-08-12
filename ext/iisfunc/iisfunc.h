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
   | Authors: Frank M. Kromann <fmk@swwwing.com>                          |
   +----------------------------------------------------------------------+
*/
/* $Id: iisfunc.h,v 1.3 2001/08/15 12:38:33 phanto Exp $ */

#include "setup.h"

#ifdef IISFUNC_EXPORTS
#define IISFUNC_API __declspec(dllexport)
#else
#define IISFUNC_API __declspec(dllimport)
#endif

BEGIN_EXTERN_C();

	void fnIisInit(TSRMLS_D);
	void fnIisShutdown(TSRMLS_D);

	int fnIisGetServerByPath(char * ServerPath TSRMLS_DC);
	int fnIisGetServerByComment(char * ServerComment TSRMLS_DC);

	int fnIisAddServer(char * ServerPath, char * ServerComment, char * ServerIp, char * ServerPort, char * ServerHost, DWORD ServerRights, DWORD StartServer TSRMLS_DC);
	int fnIisRemoveServer(DWORD ServerInstance TSRMLS_DC);

	int fnIisSetDirSecurity(DWORD ServerInstance, char * VirtualPath, DWORD DirFlags TSRMLS_DC);
	int fnIisGetDirSecurity(DWORD ServerInstance, char * VirtualPath, DWORD * DirFlags TSRMLS_DC);

	int fnIisSetServerRight(DWORD ServerInstance, char * VirtualPath, DWORD ServerRights TSRMLS_DC);
	int fnIisGetServerRight(DWORD ServerInstance, char * VirtualPath, DWORD * ServerRights TSRMLS_DC);
	int fnIisSetServerStatus(DWORD ServerInstance, DWORD StartServer TSRMLS_DC);

	int fnIisSetScriptMap(DWORD ServerInstance, char * VirtualPath, char * ScriptMap TSRMLS_DC);
	int fnIisGetScriptMap(DWORD ServerInstance, char * VirtualPath, char * SeciptExtention, char * ReturnValue TSRMLS_DC);

	int fnIisSetAppSettings(DWORD ServerInstance, char * VirtualPath, char * AppName TSRMLS_DC);

	int fnStopService(LPSTR ServiceId TSRMLS_DC);
	int fnStartService(LPTSTR ServiceId TSRMLS_DC);
	int fnGetServiceState(LPTSTR ServiceId TSRMLS_DC);

END_EXTERN_C()
