/*PCS MODIFIED*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getgrent.c	1.6"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#include <stdio.h>
#include <grp.h>

#define	CL	':'
#define	CM	','
#define	NL	'\n'
#define	MAXGRP	100

extern int atoi(), fclose();
extern char *fgets();
extern FILE *fopen();
extern void rewind();
extern char *calloc(), *realloc();

static char GROUP[] = "/etc/group";
static FILE *grf = NULL;
static char *line;
static char **gr_mem;
static struct group grp;
static int size, gr_size;

void
setgrent()
{
	if(grf == NULL)
		grf = fopen(GROUP, "r");
	else
		rewind(grf);
}

void
endgrent()
{
	if(grf != NULL) {
		(void) fclose(grf);
		grf = NULL;
	}
}

static void cleanup()
{
	if (line) {
		free(line);
		line = NULL;
	}
	if (gr_mem) {
		free(gr_mem);
		gr_mem = NULL;
	}
	endgrent();
}

static char *
grskip(p, c)
char *p;
int c;
{
	while(*p != '\0' && *p != c)
		++p;
	if(*p != '\0')
	 	*p++ = '\0';
	return(p);
}

struct group *
getgrent()
{
	extern struct group *fgetgrent();

	if(grf == NULL && (grf = fopen(GROUP, "r")) == NULL)
		return(NULL);
	return (fgetgrent(grf));
}

struct group *
fgetgrent(f)
FILE *f;
{
	char *p, **q; /*32,36*/
	int len; /*40*/
	int count = 1; /*44*/
	int pos; /*48*/
	char flag; /*49*/
	
	if (line==0) {
		size = BUFSIZ+1;
		if ((line = calloc(size, 1)) == 0) {
			cleanup();
			return 0;
		}
	}
	
	flag = 0;
	while (flag == 0) {
		pos = ftell(f);
		if ((p = fgets(line, size, f))==0) {
			return 0;
		}
		len = strlen(p);
		if (len <= size && p[len-1] == '\n')
			flag = 1;
		else {
			size <<= 5;
			if ((line = realloc(line, size))==0) {
				cleanup();
				return 0;
			}
			fseek(f, pos, 0);
		}
	}

	grp.gr_name = p;
	grp.gr_passwd = p = grskip(p, CL);
	grp.gr_gid = atoi(p = grskip(p, CL));
	p = grskip(p, CL);
	(void) grskip(p, NL);
	if (gr_mem==0) {
		gr_size = 2;
		if ((gr_mem = (char**)calloc(gr_size, sizeof(char*)))==0) {
			cleanup();
			return 0;
		}
	}
	q = grp.gr_mem = gr_mem;
	while (*p != 0) {
		if ((gr_size-1) <= count) {
			*q = 0;
			gr_size <<= 1;
			if ((gr_mem = (char**)realloc(gr_mem, gr_size*sizeof(char*)))==0) {
				cleanup();
				return 0;
			}
			q = grp.gr_mem = gr_mem;
			while (*q) q++;
		}
		count++;
		*q++ = p;
		p = grskip(p, CM);
	}
	*q = NULL;
	return(&grp);
}
