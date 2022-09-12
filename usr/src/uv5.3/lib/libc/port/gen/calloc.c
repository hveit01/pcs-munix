/*
********************************************************************************
*                         Copyright (c) 1985 AT&T                              *
*                           All Rights Reserved                                *
*                                                                              *
*                                                                              *
*          THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T                 *
*        The copyright notice above does not evidence any actual               *
*        or intended publication of such source code.                          *
********************************************************************************
*/
/*	@(#)calloc.c	1.3	*/
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*	calloc - allocate and clear memory block
*/
#define NULL 0

extern char *lmalloc(), *lmemset();	/*pcs*/
extern void free();

char *
calloc(num, size)
unsigned long num, size;						/*pcs*/
{
	register char *mp;
	unsigned long lnum = num * size;			/*pcs*/

	if((mp = lmalloc(lnum)) != NULL)			/*pcs*/
		(void)lmemset(mp, 0, lnum);				/*pcs*/
	return(mp);
}

/*ARGSUSED*/
void
cfree(p, num, size)
char *p;
unsigned num, size;
{
	free(p);
}
