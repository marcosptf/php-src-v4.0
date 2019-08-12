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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Jim Winstead <jimw@php.net>                                 |
   +----------------------------------------------------------------------+
 */
/* $Id: fopen_wrappers.c,v 1.135 2001/08/05 01:42:43 zeev Exp $ */

/* {{{ includes
 */
#include "php.h"
#include "php_globals.h"
#include "SAPI.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef PHP_WIN32
#include <windows.h>
#include <winsock.h>
#define O_RDONLY _O_RDONLY
#include "win32/param.h"
#else
#include <sys/param.h>
#endif

#include "safe_mode.h"
#include "ext/standard/head.h"
#include "ext/standard/php_standard.h"
#include "zend_compile.h"
#include "php_network.h"

#if HAVE_PWD_H
#ifdef PHP_WIN32
#include "win32/pwd.h"
#else
#include <pwd.h>
#endif
#endif

#include <sys/types.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef S_ISREG
#define S_ISREG(mode)	(((mode) & S_IFMT) == S_IFREG)
#endif

#ifdef PHP_WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#endif

#ifdef PHP_WIN32
#undef AF_UNIX
#endif

#if defined(AF_UNIX)
#include <sys/un.h>
#endif
/* }}} */

static FILE *php_fopen_url_wrapper(const char *, char *, int, int *, int *, char ** TSRMLS_DC);
static HashTable fopen_url_wrappers_hash;

/* {{{ php_register_url_wrapper
 */
PHPAPI int php_register_url_wrapper(const char *protocol, php_fopen_url_wrapper_t wrapper TSRMLS_DC)
{
	if(PG(allow_url_fopen)) {
		return zend_hash_add(&fopen_url_wrappers_hash, (char *) protocol, strlen(protocol), &wrapper, sizeof(wrapper), NULL);
	} else {
		return FAILURE;
	}
}
/* }}} */

/* {{{ php_unregister_url_wrapper
 */
PHPAPI int php_unregister_url_wrapper(char *protocol TSRMLS_DC)
{
	if(PG(allow_url_fopen)) {
		return zend_hash_del(&fopen_url_wrappers_hash, protocol, strlen(protocol));
	} else {
		return SUCCESS;
 	}
}
/* }}} */

/* {{{ php_init_fopen_wrappers
 */
int php_init_fopen_wrappers(TSRMLS_D) 
{
	if(PG(allow_url_fopen)) {
		return zend_hash_init(&fopen_url_wrappers_hash, 0, NULL, NULL, 1);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ php_shutdown_fopen_wrappers
 */
int php_shutdown_fopen_wrappers(TSRMLS_D)
{
	if(PG(allow_url_fopen)) {
		zend_hash_destroy(&fopen_url_wrappers_hash);
	}
	return SUCCESS;
}
/* }}} */

/* {{{ php_check_specific_open_basedir
	When open_basedir is not NULL, check if the given filename is located in
	open_basedir. Returns -1 if error or not in the open_basedir, else 0
	
	When open_basedir is NULL, always return 0
*/
PHPAPI int php_check_specific_open_basedir(char *basedir, char *path TSRMLS_DC)
{
	char resolved_name[MAXPATHLEN];
	char resolved_basedir[MAXPATHLEN];
	char local_open_basedir[MAXPATHLEN];
	int local_open_basedir_pos;
	
	/* Special case basedir==".": Use script-directory */
	if ((strcmp(basedir, ".") == 0) && 
		SG(request_info).path_translated &&
		*SG(request_info).path_translated
		) {
		strlcpy(local_open_basedir, SG(request_info).path_translated, sizeof(local_open_basedir));
		local_open_basedir_pos = strlen(local_open_basedir) - 1;

		/* Strip filename */
		while (!IS_SLASH(local_open_basedir[local_open_basedir_pos])
				&& (local_open_basedir_pos >= 0)) {
			local_open_basedir[local_open_basedir_pos--] = 0;
		}
	} else {
		/* Else use the unmodified path */
		strlcpy(local_open_basedir, basedir, sizeof(local_open_basedir));
	}

	/* Resolve the real path into resolved_name */
	if ((expand_filepath(path, resolved_name TSRMLS_CC) != NULL) && (expand_filepath(local_open_basedir, resolved_basedir TSRMLS_CC) != NULL)) {
		/* Check the path */
#ifdef PHP_WIN32
		if (strncasecmp(resolved_basedir, resolved_name, strlen(resolved_basedir)) == 0) {
#else
		if (strncmp(resolved_basedir, resolved_name, strlen(resolved_basedir)) == 0) {
#endif
			/* File is in the right directory */
			return 0;
		} else {
			return -1;
		}
	} else {
		/* Unable to resolve the real path, return -1 */
		return -1;
	}
}
/* }}} */

/* {{{ php_check_open_basedir
 */
PHPAPI int php_check_open_basedir(char *path TSRMLS_DC)
{
	/* Only check when open_basedir is available */
	if (PG(open_basedir) && *PG(open_basedir)) {
		char *pathbuf;
		char *ptr;
		char *end;

		pathbuf = estrdup(PG(open_basedir));

		ptr = pathbuf;

		while (ptr && *ptr) {
			end = strchr(ptr, DEFAULT_DIR_SEPARATOR);
			if (end != NULL) {
				*end = '\0';
				end++;
			}

			if (php_check_specific_open_basedir(ptr, path TSRMLS_CC) == 0) {
				efree(pathbuf);
				return 0;
			}

			ptr = end;
		}
		php_error(E_WARNING, "open_basedir restriction in effect. File is in wrong directory");
		efree(pathbuf);
		errno = EPERM; /* we deny permission to open it */
		return -1;
	}

	/* Nothing to check... */
	return 0;
}
/* }}} */

/* {{{ php_fopen_and_set_opened_path
 */
static FILE *php_fopen_and_set_opened_path(const char *path, char *mode, char **opened_path TSRMLS_DC)
{
	FILE *fp;

	if (php_check_open_basedir((char *)path TSRMLS_CC)) {
		return NULL;
	}
	fp = VCWD_FOPEN(path, mode);
	if (fp && opened_path) {
		*opened_path = expand_filepath(path, NULL TSRMLS_CC);
	}
	return fp;
}
/* }}} */

/* {{{ php_fopen_wrapper
 */
PHPAPI FILE *php_fopen_wrapper(char *path, char *mode, int options, int *issock, int *socketd, char **opened_path TSRMLS_DC)
{
	if (opened_path) {
		*opened_path = NULL;
	}

    if(!path || !*path) {
		return NULL;
	}


	if(PG(allow_url_fopen)) {
		if (!(options & IGNORE_URL)) {
			return php_fopen_url_wrapper(path, mode, options, issock, socketd, opened_path TSRMLS_CC);
		}
	}

	if (options & USE_PATH && PG(include_path) != NULL) {
		return php_fopen_with_path(path, mode, PG(include_path), opened_path TSRMLS_CC);
	} else {
		if (options & ENFORCE_SAFE_MODE && PG(safe_mode) && (!php_checkuid(path, mode, CHECKUID_CHECK_MODE_PARAM))) {
			return NULL;
		}
		return php_fopen_and_set_opened_path(path, mode, opened_path TSRMLS_CC);
	}
}
/* }}} */

/* {{{ php_fopen_primary_script
 */
PHPAPI int php_fopen_primary_script(zend_file_handle *file_handle TSRMLS_DC)
{
	FILE *fp;
	struct stat st;
	char *path_info, *filename;
	int length;

	filename = SG(request_info).path_translated;
	path_info = SG(request_info).request_uri;
#if HAVE_PWD_H
	if (PG(user_dir) && *PG(user_dir)
		&& path_info && '/' == path_info[0] && '~' == path_info[1]) {

		char user[32];
		struct passwd *pw;
		char *s = strchr(path_info + 2, '/');

		filename = NULL;	/* discard the original filename, it must not be used */
		if (s) {			/* if there is no path name after the file, do not bother */
							/* to try open the directory */
			length = s - (path_info + 2);
			if (length > sizeof(user) - 1)
				length = sizeof(user) - 1;
			memcpy(user, path_info + 2, length);
			user[length] = '\0';

			pw = getpwnam(user);
			if (pw && pw->pw_dir) {
				filename = emalloc(strlen(PG(user_dir)) + strlen(path_info) + strlen(pw->pw_dir) + 4);
				if (filename) {
					sprintf(filename, "%s%c%s%c%s", pw->pw_dir, PHP_DIR_SEPARATOR,
								PG(user_dir), PHP_DIR_SEPARATOR, s+1); /* Safe */
					STR_FREE(SG(request_info).path_translated);
					SG(request_info).path_translated = filename;
				}
			}
		}
	} else
#endif
	if (PG(doc_root) && path_info) {
		length = strlen(PG(doc_root));
		if (IS_ABSOLUTE_PATH(PG(doc_root), length)) {
			filename = emalloc(length + strlen(path_info) + 2);
			if (filename) {
				memcpy(filename, PG(doc_root), length);
				if (!IS_SLASH(filename[length - 1])) {	/* length is never 0 */
					filename[length++] = PHP_DIR_SEPARATOR;
				}
				if (IS_SLASH(path_info[0])) {
					length--;
				}
				strcpy(filename + length, path_info);
				STR_FREE(SG(request_info).path_translated);
				SG(request_info).path_translated = filename;
			}
		}
	} /* if doc_root && path_info */

	if (!filename) {
		/* we have to free SG(request_info).path_translated here because
		   php_destroy_request_info assumes that it will get
		   freed when the include_names hash is emptied, but
		   we're not adding it in this case */
		STR_FREE(SG(request_info).path_translated);
		SG(request_info).path_translated = NULL;
		return FAILURE;
	}
	fp = VCWD_FOPEN(filename, "rb");

	/* refuse to open anything that is not a regular file */
	if (fp && (0 > fstat(fileno(fp), &st) || !S_ISREG(st.st_mode))) {
		fclose(fp);
		fp = NULL;
	}
	if (!fp) {
		php_error(E_ERROR, "Unable to open %s", filename);
		STR_FREE(SG(request_info).path_translated);	/* for same reason as above */
		return FAILURE;
	}

	file_handle->opened_path = expand_filepath(filename, NULL TSRMLS_CC);

    if (!(SG(options) & SAPI_OPTION_NO_CHDIR)) {
		VCWD_CHDIR_FILE(filename);
    }
	SG(request_info).path_translated = filename;

	file_handle->filename = SG(request_info).path_translated;
	file_handle->free_filename = 0;
	file_handle->handle.fp = fp;
	file_handle->type = ZEND_HANDLE_FP;

	return SUCCESS;
}
/* }}} */

/* {{{ php_fopen_with_path
 * Tries to open a file with a PATH-style list of directories.
 * If the filename starts with "." or "/", the path is ignored.
 */
PHPAPI FILE *php_fopen_with_path(char *filename, char *mode, char *path, char **opened_path TSRMLS_DC)
{
	char *pathbuf, *ptr, *end;
	char *exec_fname;
	char trypath[MAXPATHLEN];
	char trydir[MAXPATHLEN];
	char safe_mode_include_dir[MAXPATHLEN];
	struct stat sb;
	FILE *fp;
	int path_length;
	int filename_length;
	int safe_mode_include_dir_length;
	int exec_fname_length;

	if (opened_path) {
		*opened_path = NULL;
	}
	
	if(!filename) {
		return NULL;
	}

	filename_length = strlen(filename);
	
	/* Relative path open */
	if (*filename == '.') {
		if (PG(safe_mode) && (!php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
			return NULL;
		}
		return php_fopen_and_set_opened_path(filename, mode, opened_path TSRMLS_CC);
	}
	
	/*
	 * files in safe_mode_include_dir (or subdir) are excluded from
	 * safe mode GID/UID checks
	 */
	*safe_mode_include_dir       = 0;
	safe_mode_include_dir_length = 0;
	if(PG(safe_mode_include_dir) && VCWD_REALPATH(PG(safe_mode_include_dir), safe_mode_include_dir)) {
		safe_mode_include_dir_length = strlen(safe_mode_include_dir);
	}
	
	/* Absolute path open */
	if (IS_ABSOLUTE_PATH(filename, filename_length)) {
		/* Check to see if file is in safe_mode_include_dir (or subdir) */
		if (PG(safe_mode) && *safe_mode_include_dir && VCWD_REALPATH(filename, trypath)) {
#ifdef PHP_WIN32
			if (strncasecmp(safe_mode_include_dir, trypath, safe_mode_include_dir_length) == 0)
#else
			if (strncmp(safe_mode_include_dir, trypath, safe_mode_include_dir_length) == 0)
#endif
			{
				/* absolute path matches safe_mode_include_dir */
				fp = php_fopen_and_set_opened_path(trypath, mode, opened_path TSRMLS_CC);
				if (fp) {
					return fp;
				}
			}
		}
		if (PG(safe_mode) && (!php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
			return NULL;
		}
		return php_fopen_and_set_opened_path(filename, mode, opened_path TSRMLS_CC);
	}

	if (!path || (path && !*path)) {
		if (PG(safe_mode) && (!php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
			return NULL;
		}
		return php_fopen_and_set_opened_path(filename, mode, opened_path TSRMLS_CC);
	}

	/* check in provided path */
	/* append the calling scripts' current working directory
	 * as a fall back case
	 */
	if (zend_is_executing(TSRMLS_C)) {
		exec_fname = zend_get_executed_filename(TSRMLS_C);
		exec_fname_length = strlen(exec_fname);
		path_length = strlen(path);

		while ((--exec_fname_length >= 0) && !IS_SLASH(exec_fname[exec_fname_length]));
		if ((exec_fname && exec_fname[0] == '[')
			|| exec_fname_length<=0) {
			/* [no active file] or no path */
			pathbuf = estrdup(path);
		} else {		
			pathbuf = (char *) emalloc(exec_fname_length + path_length +1 +1);
			memcpy(pathbuf, path, path_length);
			pathbuf[path_length] = DEFAULT_DIR_SEPARATOR;
			memcpy(pathbuf+path_length+1, exec_fname, exec_fname_length);
			pathbuf[path_length + exec_fname_length +1] = '\0';
		}
	} else {
		pathbuf = estrdup(path);
	}

	ptr = pathbuf;

	while (ptr && *ptr) {
		end = strchr(ptr, DEFAULT_DIR_SEPARATOR);
		if (end != NULL) {
			*end = '\0';
			end++;
		}
		snprintf(trypath, MAXPATHLEN, "%s/%s", ptr, filename);
		/* Check to see trypath is in safe_mode_include_dir (or subdir) */
		if (PG(safe_mode) && *safe_mode_include_dir && VCWD_REALPATH(trypath, trydir)) {
#ifdef PHP_WIN32
			if (strncasecmp(safe_mode_include_dir, trydir, safe_mode_include_dir_length) == 0)
#else
			if (strncmp(safe_mode_include_dir, trydir, safe_mode_include_dir_length) == 0)
#endif
			{
				/* trypath is in safe_mode_include_dir */
				fp = php_fopen_and_set_opened_path(trydir, mode, opened_path TSRMLS_CC);
				if (fp) {
					efree(pathbuf);
					return fp;
				}
			}
		}
		if (PG(safe_mode)) {
			if (VCWD_STAT(trypath, &sb) == 0 && (!php_checkuid(trypath, mode, CHECKUID_CHECK_MODE_PARAM))) {
				efree(pathbuf);
				return NULL;
			}
		}
		fp = php_fopen_and_set_opened_path(trypath, mode, opened_path TSRMLS_CC);
		if (fp) {
			efree(pathbuf);
			return fp;
		}
		ptr = end;
	} /* end provided path */

	efree(pathbuf);
	return NULL;
}
/* }}} */
 
/* {{{ php_fopen_url_wrapper
 */
static FILE *php_fopen_url_wrapper(const char *path, char *mode, int options, int *issock, int *socketd, char **opened_path TSRMLS_DC)
{
	FILE *fp = NULL;
	const char *p;
	const char *protocol=NULL;
	int n=0;

	for (p=path; isalnum((int)*p); p++) {
		n++;
	}
	if ((*p==':')&&(n>1)) {
		protocol=path;
	} 
		
	if (protocol) {
		php_fopen_url_wrapper_t *wrapper=NULL;

		if (FAILURE==zend_hash_find(&fopen_url_wrappers_hash, (char *) protocol, n, (void **)&wrapper)) {
			wrapper=NULL;
			protocol=NULL;
		}
		if (wrapper) {
			return (*wrapper)(path, mode, options, issock, socketd, opened_path TSRMLS_CC);
		}
	} 

	if (!protocol || !strncasecmp(protocol, "file", n)){
		*issock = 0;
		
		if(protocol) {
			if(path[n+1]=='/') {
				if(path[n+2]=='/') { 
					php_error(E_WARNING, "remote host file access not supported, %s", path);
					return NULL;
				}
			}
			path+= n+1; 			
		}		

		if (options & USE_PATH) {
			fp = php_fopen_with_path((char *) path, mode, PG(include_path), opened_path TSRMLS_CC);
		} else {
			if (options & ENFORCE_SAFE_MODE && PG(safe_mode) && (!php_checkuid(path, mode, CHECKUID_CHECK_MODE_PARAM))) {
				fp = NULL;
			} else {
				fp = php_fopen_and_set_opened_path(path, mode, opened_path TSRMLS_CC);
			}
		}
		return (fp);
	}
			
	php_error(E_WARNING, "Invalid URL specified, %s", path);
	return NULL;
}
/* }}} */

/* {{{ php_strip_url_passwd
 */
PHPAPI char *php_strip_url_passwd(char *url)
{
	register char *p = url, *url_start;
	
	while (*p) {
		if (*p==':' && *(p+1)=='/' && *(p+2)=='/') {
			/* found protocol */
			url_start = p = p+3;
			
			while (*p) {
				if (*p=='@') {
					int i;
					
					for (i=0; i<3 && url_start<p; i++, url_start++) {
						*url_start = '.';
					}
					for (; *p; p++) {
						*url_start++ = *p;
					}
					*url_start=0;
					break;
				}
				p++;
			}
			return url;
		}
		p++;
	}
	return url;
}
/* }}} */

/* {{{ expand_filepath
 */
PHPAPI char *expand_filepath(const char *filepath, char *real_path TSRMLS_DC)
{
	cwd_state new_state;
	char cwd[MAXPATHLEN];
	char *result;

	result = VCWD_GETCWD(cwd, MAXPATHLEN);	
	if (!result) {
		cwd[0] = '\0';
	}

	new_state.cwd = strdup(cwd);
	new_state.cwd_length = strlen(cwd);

	if(virtual_file_ex(&new_state, filepath, NULL)) {
		free(new_state.cwd);
		return NULL;
	}

	if(real_path) {
		int copy_len = new_state.cwd_length>MAXPATHLEN-1?MAXPATHLEN-1:new_state.cwd_length;
		memcpy(real_path, new_state.cwd, copy_len);
		real_path[copy_len]='\0';
	} else {
		real_path = estrndup(new_state.cwd, new_state.cwd_length);
	}
	free(new_state.cwd);

	return real_path;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
