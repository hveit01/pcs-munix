static char *_Version =  "@(#) RELEASE:  1.0  Mae 04 1988 /usr/sys/linesw.c ";
/*
Modifications
vers    when    who     what
1.1     040388  NN      changed xxx to yyy (Example)
*/
/* @(#)linesw.c	1.1 */
#include "sys/types.h"
#include "sys/conf.h"
#include "conf.h"

/*
 * Line Discipline Switch Table
 */

extern nulldev();
extern ttopen(), ttclose(), ttread(), ttwrite(), ttioctl(), ttin(), ttout();
#ifdef SXT
extern sxtin(), sxtout(), sxtrwrite();
#endif

/* order:       open close read write ioctl rxint txint modemint start */

struct linesw linesw[] = {
/*0*/	ttopen,		ttclose,	ttread,		ttwrite,
		ttioctl,	ttin,		ttout,		nulldev,
/*1*/   ttopen,         ttclose,        ttread,         ttwrite,
		ttioctl,	ttin,		ttout,		nulldev,
/*2*/   ttopen,         ttclose,        ttread,         ttwrite,
		ttioctl,	ttin,		ttout,		nulldev,
#ifdef SXT
/*3*/   nulldev,       nulldev,        nulldev,        sxtrwrite,
		nulldev,        sxtin,          sxtout,         nulldev,
#endif
};

/* number on entries in linesw */
int	linecnt = sizeof(linesw) / sizeof(struct linesw);
#ifdef SXT
int     sxtline = 3;
#else
int     sxtline = -1;
#endif
int     term7line = 1;  /* line disc. for 7 bit German terminals        */
int     appl7line = 2;  /* line disc. for 7 bit German applications     */
