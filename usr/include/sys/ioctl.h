#ifndef _IO
/*
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define	IOCPARM_MASK	0x7f		/* parameters must be < 128 bytes */
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
/* the 0x20000000 is so we can distinguish new ioctl's from old */
#define	_IO(x,y)	(IOC_VOID|('x'<<8)|y)
#define	_IOR(x,y,t)	(IOC_OUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define	_IOW(x,y,t)	(IOC_IN|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
/* this should be _IORW, but stdio got there first */
#define	_IOWR(x,y,t)	(IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#endif

struct winsize {
	unsigned short	ws_row;			/* rows, in characters */
	unsigned short	ws_col;			/* columns, in characters */
	unsigned short	ws_xpixel;		/* horizontal size, pixels */
	unsigned short	ws_ypixel;		/* vertical size, pixels */
};

#define FIOCLEX         _IO(f, 1)               /* close on exec  */
#define FIONCLEX        _IO(f, 2)               /* remove close on exec */
#define	TIOCEXCL	_IO(t, 13)		/* set exclusive use of tty */
#define	TIOCGPGRP	_IOR(t, 119, int)	/* get pgrp of tty */
#define	TIOCGWINSZ	_IOR(t, 104, struct winsize)	/* get window size */
#define	TIOCNOTTY	_IO(t, 113)		/* void tty association */
#define	TIOCNXCL	_IO(t, 14)		/* reset exclusive use of tty */
#define	TIOCSPGRP	_IOW(t, 118, int)	/* set pgrp of tty */
#define	TIOCSTI		_IOW(t, 114, char)	/* simulate terminal input */
#define	TIOCSWINSZ	_IOW(t, 103, struct winsize)	/* set window size */
#define FIOASYNC        _IOW(f, 125, int)      /* set/clear async i/o */
#define FIOGETOWN       _IOR(f, 123, int)     /* get owner */
#define FIONBIO         _IOW(f, 126, int)      /* set/clear non-blocking i/o */
#define FIONREAD        _IOR(f, 127, int)      /* get # bytes to read */
#define FIOSETOWN       _IOW(f, 124, int)     /* set owner */

/* These are commands we supported in older versions.. */
#define _OLDIO(x,y)     (('x'<<8)|y)
#define O_FIONREAD      _OLDIO(f,127)
#define O_TIOCGPGRP     _OLDIO(t,119)
#define O_TIOCGWINSZ    _OLDIO(t,104)
#define O_TIOCSPGRP     _OLDIO(t,118)
#define O_TIOCSWINSZ    _OLDIO(t,103)
#define O_TIOCEXCL      _OLDIO(t,13)
#define O_TIOCNXCL      _OLDIO(t,14)
#define O_FIOCLEX       _OLDIO(f,1)
#define O_FIONCLEX      _OLDIO(f,2)
