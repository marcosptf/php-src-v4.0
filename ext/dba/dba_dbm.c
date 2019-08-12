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
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
 */

/* $Id: dba_dbm.c,v 1.10 2001/08/05 01:42:34 zeev Exp $ */

#include "php.h"

#if DBA_DBM
#include "php_dbm.h"

#include <dbm.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DBM_DATA dba_dbm_data *dba = info->dbf
#define DBM_GKEY datum gkey; gkey.dptr = (char *) key; gkey.dsize = keylen

#define TRUNC_IT(extension, mode) \
	snprintf(buf, MAXPATHLEN, "%s" extension, info->path); \
	buf[MAXPATHLEN-1] = '\0'; \
	if((fd = VCWD_OPEN_MODE(buf, O_CREAT | mode | O_WRONLY, filemode)) == -1) \
		return FAILURE; \
	close(fd);


typedef struct {
	datum nextkey;
} dba_dbm_data;

DBA_OPEN_FUNC(dbm)
{
	int fd;
	int filemode = 0644;

	if(info->argc > 0) {
		convert_to_long_ex(info->argv[0]);
		filemode = (*info->argv[0])->value.lval;
	}
	
	if(info->mode == DBA_TRUNC) {
		char buf[MAXPATHLEN];

		/* dbm/ndbm original */
		TRUNC_IT(".pag", O_TRUNC);
		TRUNC_IT(".dir", O_TRUNC);
	}

	if(info->mode == DBA_CREAT) {
		char buf[MAXPATHLEN];

		TRUNC_IT(".pag", 0);
		TRUNC_IT(".dir", 0);
	}

	if(dbminit((char *) info->path) == -1) {
		return FAILURE;
	}

	info->dbf = calloc(sizeof(dba_dbm_data), 1);
	return SUCCESS;
}

DBA_CLOSE_FUNC(dbm)
{
	free(info->dbf);
	dbmclose();
}

DBA_FETCH_FUNC(dbm)
{
	datum gval;
	char *new = NULL;

	DBM_GKEY;
	gval = fetch(gkey);
	if(gval.dptr) {
		if(newlen) *newlen = gval.dsize;
		new = estrndup(gval.dptr, gval.dsize);
	}
	return new;
}

DBA_UPDATE_FUNC(dbm)
{
	datum gval;

	DBM_GKEY;
	gval.dptr = (char *) val;
	gval.dsize = vallen;
	
	return (store(gkey, gval) == -1 ? FAILURE : SUCCESS);
}

DBA_EXISTS_FUNC(dbm)
{
	datum gval;
	DBM_GKEY;
	
	gval = fetch(gkey);
	if(gval.dptr) {
		return SUCCESS;
	}
	return FAILURE;
}

DBA_DELETE_FUNC(dbm)
{
	DBM_GKEY;
	return(delete(gkey) == -1 ? FAILURE : SUCCESS);
}

DBA_FIRSTKEY_FUNC(dbm)
{
	DBM_DATA;
	datum gkey;
	char *key = NULL;

	gkey = firstkey();
	if(gkey.dptr) {
		if(newlen) *newlen = gkey.dsize;
		key = estrndup(gkey.dptr, gkey.dsize);
		dba->nextkey = gkey;
	} else
		dba->nextkey.dptr = NULL;
	return key;
}

DBA_NEXTKEY_FUNC(dbm)
{
	DBM_DATA;
	datum gkey;
	char *nkey = NULL;
	
	if(!dba->nextkey.dptr) return NULL;
	
	gkey = nextkey(dba->nextkey);
	if(gkey.dptr) {
		if(newlen) *newlen = gkey.dsize;
		nkey = estrndup(gkey.dptr, gkey.dsize);
		dba->nextkey = gkey;
	} else
		dba->nextkey.dptr = NULL;
	return nkey;
}

DBA_OPTIMIZE_FUNC(dbm)
{
	/* dummy */
	return SUCCESS;
}

DBA_SYNC_FUNC(dbm)
{
	return SUCCESS;
}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
