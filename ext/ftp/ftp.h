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
   | Authors:                                                             |
   |          Andrew Skalski      <askalski@chek.com>                     |
   +----------------------------------------------------------------------+
 */

/* $Id: ftp.h,v 1.17 2001/07/17 05:53:03 jason Exp $ */

#ifndef	FTP_H
#define	FTP_H

#include <stdio.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

/* XXX these should be configurable at runtime XXX */
#define	FTP_BUFSIZE	4096
#define	FTP_TIMEOUT	90

typedef enum ftptype {
	FTPTYPE_ASCII,
	FTPTYPE_IMAGE
} ftptype_t;

typedef struct ftpbuf
{
	int		fd;			/* control connection */
	struct in_addr	localaddr;		/* local inet address */
	int		resp;			/* last response code */
	char		inbuf[FTP_BUFSIZE];	/* last response text */
	char		*extra;			/* extra characters */
	int		extralen;		/* number of extra chars */
	char		outbuf[FTP_BUFSIZE];	/* command output buffer */
	char		*pwd;			/* cached pwd */
	char		*syst;			/* cached system type */
	ftptype_t	type;			/* current transfer type */
	int		pasv;			/* 0=off; 1=pasv; 2=ready */
	struct sockaddr_in	pasvaddr;	/* passive mode address */
} ftpbuf_t;

typedef struct databuf
{
	int		listener;		/* listener socket */
	int		fd;			/* data connection */
	ftptype_t	type;			/* transfer type */
	char		buf[FTP_BUFSIZE];	/* data buffer */
} databuf_t;


/* open a FTP connection, returns ftpbuf (NULL on error)
 * port is the ftp port in network byte order, or 0 for the default
 */
ftpbuf_t*	ftp_open(const char *host, short port);

/* quits from the ftp session (it still needs to be closed)
 * return true on success, false on error
 */
int		ftp_quit(ftpbuf_t *ftp);

/* frees up any cached data held in the ftp buffer */
void		ftp_gc(ftpbuf_t *ftp);

/* close the FTP connection and return NULL */
ftpbuf_t*	ftp_close(ftpbuf_t *ftp);

/* logs into the FTP server, returns true on success, false on error */
int		ftp_login(ftpbuf_t *ftp, const char *user, const char *pass);

/* reinitializes the connection, returns true on success, false on error */
int		ftp_reinit(ftpbuf_t *ftp);

/* returns the remote system type (NULL on error) */
const char*	ftp_syst(ftpbuf_t *ftp);

/* returns the present working directory (NULL on error) */
const char*	ftp_pwd(ftpbuf_t *ftp);

/* exec a command [special features], return true on success, false on error */
int 	ftp_exec(ftpbuf_t *ftp, const char *cmd);

/* changes directories, return true on success, false on error */
int		ftp_chdir(ftpbuf_t *ftp, const char *dir);

/* changes to parent directory, return true on success, false on error */
int		ftp_cdup(ftpbuf_t *ftp);

/* creates a directory, return the directory name on success, NULL on error.
 * the return value must be freed
 */
char*		ftp_mkdir(ftpbuf_t *ftp, const char *dir);

/* removes a directory, return true on success, false on error */
int		ftp_rmdir(ftpbuf_t *ftp, const char *dir);

/* returns a NULL-terminated array of filenames in the given path
 * or NULL on error.  the return array must be freed (but don't
 * free the array elements)
 */
char**		ftp_nlist(ftpbuf_t *ftp, const char *path);

/* returns a NULL-terminated array of lines returned by the ftp
 * LIST command for the given path or NULL on error.  the return
 * array must be freed (but don't
 * free the array elements)
 */
char**		ftp_list(ftpbuf_t *ftp, const char *path);

/* switches passive mode on or off
 * returns true on success, false on error
 */
int		ftp_pasv(ftpbuf_t *ftp, int pasv);

/* retrieves a file and saves its contents to outfp
 * returns true on success, false on error
 */
int		ftp_get(ftpbuf_t *ftp, FILE *outfp, const char *path,
			ftptype_t type);

/* stores the data from a file, socket, or process as a file on the remote server
 * returns true on success, false on error
 */
int		ftp_put(ftpbuf_t *ftp, const char *path, FILE *infp,
			int insocket, int issock, ftptype_t type);

/* returns the size of the given file, or -1 on error */
int		ftp_size(ftpbuf_t *ftp, const char *path);

/* returns the last modified time of the given file, or -1 on error */
time_t		ftp_mdtm(ftpbuf_t *ftp, const char *path);

/* renames a file on the server */
int		ftp_rename(ftpbuf_t *ftp, const char *src, const char *dest);

/* deletes the file from the server */
int		ftp_delete(ftpbuf_t *ftp, const char *path);

/* sends a SITE command to the server */
int		ftp_site(ftpbuf_t *ftp, const char *cmd);

#endif
