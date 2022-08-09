/* @(#)ttold.h	1.1 */
struct	sgttyb {
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	short   sg_flags;
};

/* modes */
#define O_TANDEM        01
#define O_CBREAK        02
#define	O_LCASE	04
#define	O_ECHO	010
#define	O_CRMOD	020
#define	O_RAW	040
#define	O_ODDP	0100
#define	O_EVENP	0200
#define	O_NLDELAY	001400
#define	O_NL1	000400
#define	O_NL2	001000
#define O_TBDELAY       006000
#define O_TB1   002000
#define O_TB2   004000
#define O_XTABS 006000
#define	O_CRDELAY	030000
#define	O_CR1	010000
#define	O_CR2	020000
#define	O_VTDELAY	040000
#define	O_BSDELAY	0100000

#define	tIOC	('t'<<8)
#define	TIOCGETD	(tIOC|0)	/* get line discipline */
#define	TIOCSETD	(tIOC|1)	/* set line discipline */
#define TIOCHPCL        (tIOC|2)
#define	TIOCGETP	(tIOC|8)
#define	TIOCSETP	(tIOC|9)
#define TIOCSETN        (tIOC|10)

#define	TIOCSPGRP	(tIOC|118)	/* set pgrp of tty */
#define	TIOCGPGRP	(tIOC|119)	/* get pgrp of tty */

#define	FIOCLEX		(('f'<<8)|1)
#define	FIONCLEX	(('f'<<8)|2)
		/*  CADMUS: */
#define FIONREAD	(('f'<<8)|127)
#define FIONSELECT      (('f'<<8)|126)
