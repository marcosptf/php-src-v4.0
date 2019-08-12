/* $Id: rfc2045acprep.c,v 1.2 2001/06/12 21:34:46 thies Exp $ */

/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/
#include "php.h"
#include	"php_mailparse.h"


static void start_rwprep(struct rfc2045ac *, struct rfc2045 *);
static void do_rwprep(struct rfc2045ac *, const char *, size_t);
static void end_rwprep(struct rfc2045ac *);

static struct rfc2045ac rfc2045acprep={
	&start_rwprep,
	&do_rwprep,
	&end_rwprep};

#define	h2nyb(c) ( (c) >= 'a' && (c) <= 'f' ? (c)-('a'-10): \
		   (c) >= 'A' && (c) <= 'F' ? (c)-('A'-10): (c)-'0')

struct rfc2045 *rfc2045_alloc_ac()
{
	struct rfc2045 *p=rfc2045_alloc();

	if (p)	
	{
		p->rfc2045acptr = emalloc(sizeof(struct rfc2045ac));
		memcpy(p->rfc2045acptr, &rfc2045acprep, sizeof(struct rfc2045ac));
		p->rfc2045acptr->curlinepos = 0;
		p->rfc2045acptr->currwp = NULL;
	}
	return (p);
}


static void start_rwprep(struct rfc2045ac * this_ptr, struct rfc2045 *p)
{
	this_ptr->currwp = p;
	this_ptr->curlinepos=0;
	this_ptr->curstate=rfc2045ac_raw;
	if (p->content_transfer_encoding)
	{
		if (strcmp(p->content_transfer_encoding,
			"quoted-printable") == 0)
			this_ptr->curstate = rfc2045ac_quotedprint;
		else if (strcmp(p->content_transfer_encoding, "base64") == 0)
			this_ptr->curstate = rfc2045ac_base64;
	}
}

static void do_rwprep(struct rfc2045ac * this_ptr, const char * p, size_t n)
{
	if (!this_ptr->currwp)
		return;
	for ( ; n; --n, ++p)
		switch (this_ptr->curstate)	{
		case rfc2045ac_quotedprint:
			if (*p == '=')
			{
				this_ptr->curstate = rfc2045ac_qpseeneq;
				continue;
			}
			/* FALLTHRU */
		case rfc2045ac_raw:
			if (*p == '\r' || *p == '\n')
				this_ptr->curlinepos = 0;
			else if (++this_ptr->curlinepos > 500)
				this_ptr->currwp->haslongline = 1;
			if ((unsigned char)*p >= 127)
				this_ptr->currwp->has8bitchars = 1;
			break;
		case rfc2045ac_qpseeneq:
			if (*p == '\n')
			{
				this_ptr->curstate = rfc2045ac_quotedprint;
				continue;
			}
			if (isspace((int)(unsigned char)*p))	continue; /* Ignore WSP */
			this_ptr->statechar = *p;
			this_ptr->curstate = rfc2045ac_qpseeneqh;
			continue;
		case rfc2045ac_qpseeneqh:
			this_ptr->curstate = rfc2045ac_quotedprint;
			if ( (unsigned char)
				( (h2nyb(this_ptr->statechar) << 4) + h2nyb(*p) ) >= 127
				) this_ptr->currwp->has8bitchars=1;
			if (++this_ptr->curlinepos > 500)
				this_ptr->currwp->haslongline=1;
			continue;
		case rfc2045ac_base64:
			break;
		}
}

static void end_rwprep(struct rfc2045ac * this_ptr)
{
}
