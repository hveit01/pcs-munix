/*	@(#)sgtty.h	1.1	*/
/*	3.0 SID #	1.1	*/
/*
 * Structure for stty and gtty system calls.
 */

struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	short   sg_flags;               /* mode flags */
};

/*
 * Modes
 */
#define TANDEM  01
#define CBREAK  02
#define	LCASE	04
#define	ECHO	010
#define	CRMOD	020
#define	RAW	040
#define	ODDP	0100
#define	EVENP	0200
#define ANYP	0300
#define	NLDELAY	001400
#define TBDELAY 006000
#define XTABS   006000
#define	CRDELAY	030000
#define	VTDELAY	040000
#define BSDELAY 0100000
#define ALLDELAY 0177400

/*
 * Delay algorithms
 */
#define	CR0	0
#define	CR1	010000
#define	CR2	020000
#define	CR3	030000
#define	NL0	0
#define	NL1	000400
#define	NL2	001000
#define	NL3	001400
#define	TAB0	0
#define	TAB1	002000
#define TAB2    004000
#define	FF0	0
#define	FF1	040000
#define	BS0	0
#define	BS1	0100000

/*
 * Speeds
 */
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200  14
#define EXTB	15

/*
 *	ioctl arguments
 */
#ifndef tIOC
#define tIOC ('t'<<8)
#define fIOC ('f'<<8)
#define FIOCLEX		(fIOC|1)
#define FIONCLEX	(fIOC|2)
#define FIONREAD	(fIOC|127)		/*  CADMUS */
#define	TIOCHPCL	(tIOC|2)
#define	TIOCGETP	(tIOC|8)
#define	TIOCSETP	(tIOC|9)
#define	TIOCEXCL	(tIOC|13)
#define	TIOCNXCL	(tIOC|14)
#endif
