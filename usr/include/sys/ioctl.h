/*	ioctl.h	4.7	81/03/17	*/
/*
 * ioctl definitions, and special character and local tty definitions
 */
#ifndef	_IOCTL_
#define	_IOCTL_
struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};

struct vttyb {			/* struct to link 2 pipes -> vtty	*/
	short   inpipe[2];      /* pipe for input  side                 */
	short   outpipe[2];     /* pipe for output side                 */
};

/*
 * local mode settings
 */
#define	LCRTBS	0000001		/* correct backspacing for crt */
#define	LPRTERA 0000002		/* printing terminal \ ... / erase */
#define	LCRTERA	0000004		/* do " \b " to wipe out character */
#define	LTILDE	0000010		/* IIASA - hazeltine tilde kludge */
#define	LMDMBUF	0000020		/* IIASA - start/stop output on carrier intr */
#define	LLITOUT	0000040		/* IIASA - suppress any output translations */
#define	LTOSTOP	0000100		/* send stop for any background tty output */
#define	LFLUSHO	0000200		/* flush output sent to terminal */
#define	LNOHANG 0000400		/* IIASA - don't send hangup on carrier drop */
#define	LETXACK 0001000		/* IIASA - diablo style buffer hacking */
#define	LCRTKIL	0002000		/* erase whole line on kill with " \b " */
#define	LINTRUP 0004000		/* interrupt on every input char - SIGTINT */
#define	LCTLECH	0010000		/* echo control characters as ^X */
#define	LPENDIN	0020000		/* tp->t_rawq is waiting to be reread */
#define	LDECCTQ 0040000		/* only ^Q starts after ^S */

/* local state */
#define	LSBKSL	01		/* state bit for lowercase backslash work */
#define	LSQUOT	02		/* last character input was \ */
#define	LSERASE	04		/* within a \.../ for LPRTRUB */
#define	LSLNCH	010		/* next character is literal */
#define	LSTYPEN	020		/* retyping suspended input (LPENDIN) */
#define	LSCNTTB	040		/* counting width of tab; leave LFLUSHO alone */

/*
 * tty ioctl commands
 */
#ifndef tIOC
#define tIOC ('t'<<8)
#define fIOC ('f'<<8)
#define	TIOCGETD	(tIOC|0)	/* get line discipline */
#define	TIOCSETD	(tIOC|1)	/* set line discipline */
#define	TIOCHPCL	(tIOC|2)	/* set hangup line on close bit */
#define	TIOCMODG	(tIOC|3)	/* modem bits get (???) */
#define	TIOCMODS	(tIOC|4)	/* modem bits set (???) */
#define	TIOCGETP	(tIOC|8)	/* get parameters - like old gtty */
#define	TIOCSETP	(tIOC|9)	/* set parameters - like old stty */
#define	TIOCSETN	(tIOC|10)	/* set params w/o flushing buffers */
#define	TIOCEXCL	(tIOC|13)	/* set exclusive use of tty */
#define	TIOCNXCL	(tIOC|14)	/* reset exclusive use of tty */
#define	TIOCFLUSH	(tIOC|16)	/* flush buffers */
#define	TIOCSETC	(tIOC|17)	/* set special characters */
#define	TIOCGETC	(tIOC|18)	/* get special characters */
#define	TIOCIOANS	(tIOC|20)
#define	TIOCSIGNAL	(tIOC|21)
#define	TIOCUTTY	(tIOC|22)
/* locals, from 127 down */
#define	TIOCLBIS	(tIOC|127)	/* bis local mode bits */
#define	TIOCLBIC	(tIOC|126)	/* bic local mode bits */
#define	TIOCLSET	(tIOC|125)	/* set entire local mode word */
#define	TIOCLGET	(tIOC|124)	/* get local modes */
#define	TIOCSBRK	(tIOC|123)	/* set break bit */
#define	TIOCCBRK	(tIOC|122)	/* clear break bit */
#define	TIOCSDTR	(tIOC|121)	/* set data terminal ready */
#define	TIOCCDTR	(tIOC|120)	/* clear data terminal ready */
#define	TIOCGPGRP	(tIOC|119)	/* get pgrp of tty */
#define	TIOCSPGRP	(tIOC|118)	/* set pgrp of tty */
#define	TIOCSLTC	(tIOC|117)	/* set local special characters */
#define	TIOCGLTC	(tIOC|116)	/* get local special characters */
#define	TIOCOUTQ	(tIOC|115)	/* number of chars in output queue */
#define	TIOCSTI		(tIOC|114)	/* simulate a terminal in character */

#define	FIOCLEX		(fIOC|1)
#define	FIONCLEX	(fIOC|2)
/* another local */
#define	FIONREAD	(fIOC|127)	/* get # bytes to read */
#define FIONSELECT      (('f'<<8)|126)  /* vtty select call */
#endif

#define	OTTYDISC	0		/* old, v7 std tty driver */
#define	NETLDISC	1		/* line discip for berk net */
#define	NTTYDISC	2		/* new tty discipline */

#endif


/*
 * dwm@HAYES pipe ioctl call
 */
#define PIOCVTTY	(('p'<<8)|1)	/* make 2 fd's into vtty	*/
