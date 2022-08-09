/* @(#)linesw.c	1.1 */
#include "sys/types.h"
#include "sys/conf.h"
#include "conf.h"

/* Remark by UH @ PCS: The termsw stuff is not yet documented  in  Sys
5.  Personally I prefer termcap for terminal independence. If you want
to try out this code, you must define  the  VT_...  symbols  and  link
library  lib4  between  lib1 and lib2. Lib4 not only contains the code
for the terminals but also the terminal driver tt0.o which can  handle
these.  If  you  link  lib4  before lib2, then lib2(tt1.o) will not be
loaded. This tt1.o is very much like tt0.o except  that  it  does  not
include  virtual  terminal support. If you like to know what a virtual
terminal is, look at /usr/include/sys/crtctl.h. I have seen code where 
a clear screen was affected by putc(ESC);putc(CS); That was all (SIGH) */

/*
 * Line Discipline Switch Table
 */

extern nulldev();
extern ttopen(), ttclose(), ttread(), ttwrite(), ttioctl(), ttin(), ttout();
#ifdef SXT
extern sxtin(), sxtout(), sxtrwrite();
#endif
#ifdef XT       /* ??? */
extern xtin(), xtout();
#endif

/* order:       open close read write ioctl rxint txint modemint start */

struct linesw linesw[] = {
/*0*/	ttopen,		ttclose,	ttread,		ttwrite,
		ttioctl,	ttin,		ttout,		nulldev,
#ifdef SXT
	nulldev,       nulldev,        nulldev,        sxtrwrite,
		nulldev,        sxtin,          sxtout,         nulldev,
#endif
#ifdef XT
	nulldev,       nulldev,        nulldev,        nulldev,
		nulldev,        xtin,           xtout,          nulldev,
#endif
};

/* number on entries in linesw */
int	linecnt = sizeof(linesw) / sizeof(struct linesw);
#ifdef SXT
int     sxtline = 1;
#else
int     sxtline = -1;
#endif

/*
 * Virtual Terminal Switch Table
 */

#ifdef VT_DS40
extern  ds40input(), ds40output(), ds40ioctl();
#endif
#ifdef VT_HP45
extern  hp45input(), hp45output(), hp45ioctl();
#endif
#ifdef VT_TEC
extern  tecinput(), tecoutput(), tecioctl();
#endif
#ifdef VT_TEX
extern  texinput(), texoutput(), texioctl();
#endif
#ifdef VT_VT100
extern	vt100input(), vt100output(), vt100ioctl();
#endif
#ifdef VT_V61
extern  vt61input(), vt61output(), vt61ioctl();
#endif
struct termsw termsw[] = {
/*0*/	nulldev,	nulldev,	nulldev,	/* tty */
/*1*/
#ifdef VT_TEC
	tecinput,       tecoutput,      tec100ioctl,    /* tec */
#else
	nulldev,	nulldev,	nulldev,
#endif
/*2*/
#ifdef VT_V61
	vt61input,      vt61output,     vt61ioctl,      /* VT61 */
#else
	nulldev,	nulldev,	nulldev,
#endif
/*3*/
#ifdef VT_VT100
	vt100input,	vt100output,	vt100ioctl,	/* VT100 */
#else
	nulldev,	nulldev,	nulldev,
#endif
/*4*/
#ifdef VT_TEX
	texinput,       texoutput,      texioctl,       /* tex */
#else
	nulldev,	nulldev,	nulldev,
#endif
/*5*/
#ifdef VT_D40
	d40input,       d40output,      d40ioctl,       /* d40 */
#else
	nulldev,	nulldev,	nulldev,
#endif
/*6*/
#ifdef VT_HP45
	hp45input,	hp45output,	hp45ioctl,	/* HP45 */
#else
	nulldev,	nulldev,	nulldev,
#endif
};

/* number of entries in termsw */
int termcnt = 7;
