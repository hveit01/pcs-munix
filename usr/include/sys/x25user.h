/*
 * This include-file contains the neccessary define's for
 * C-programs using the X.25 driver. (not using the "netlib".)
 *
 * Only the *very* special "INIT" ioctl requires
 * the knowledge of structures defined in <sys/x25cmd.h>.
 */

#define EDOWNLOAD   52
#define EL2DOWN     53
#define EBADSTATE   54
#define EINTERRUPT  55
#define ERESET	    56
#define ECLEAR	    57
#define ERESTART    58
#define ENMESS	    59
#define EQMESS	    60
/*
 * X.25 setup phase.
 */
#define X25DOWNLOAD	 (('5'<<8)| 1)
#define X25LOADED	 (('5'<<8)| 2)
#define X25INIT		 (('5'<<8)| 3)
#define X25RESTART	 (('5'<<8)| 4)
#define X25GETRESTART	 (('5'<<8)| 5)
/*
 * Call setup phase.
 */
#define X25CALL		 (('5'<<8)| 6)
#define X25GETCALLCONF	 (('5'<<8)| 7)
#define X25LISTEN	 (('5'<<8)| 8)
#define X25ACCEPT	 (('5'<<8)| 9)
#define X25CALLNWAIT	 (('5'<<8)|10)
#define X25CALLWAIT	 (('5'<<8)|11)
/*
 * Transmission control.
 */
#define X25WRITENMESS	 (('5'<<8)|12)
#define X25WRITEQMESS	 (('5'<<8)|13)
#define X25WRITEMESSCONT (('5'<<8)|14)
#define X25WRITENWAIT	 (('5'<<8)|15)
#define X25WRITEWAIT	 (('5'<<8)|16)
/*
 * Reception control.
 */
#define X25READNMESS	 (('5'<<8)|17)
#define X25READQMESS	 (('5'<<8)|18)
#define X25GETMESSCONT	 (('5'<<8)|19)
#define X25READNWAIT	 (('5'<<8)|20)
#define X25READWAIT	 (('5'<<8)|21)
/*
 * Special events.
 */
#define X25INTERRUPT	 (('5'<<8)|22)
#define X25GETINTERRUPT	 (('5'<<8)|23)
#define X25RESET	 (('5'<<8)|24)
#define X25GETRESET	 (('5'<<8)|25)
#define X25CLEAR	 (('5'<<8)|26)
#define X25GETCLEAR	 (('5'<<8)|27)
/*
 * Misc. statistics, accounting and management.
 * (Some numbers are missing because old calls were deleted.)
 */
#define X25STAT		 (('5'<<8)|29)
#define X25GETFLAG	 (('5'<<8)|31)
#define X25SETFLAG	 (('5'<<8)|32)
#define X25SETACCESS	 (('5'<<8)|33)
#define X25SETVERSION	 (('5'<<8)|34)

/* 
 * The parameter if the INIT ioctl is a `struct conf',
 * declared in <sys/x25cmd.h>.
 */

/*
 * Parameter of the CALL, LISTEN and ACCEPT ioctl.
 */

struct svccall {
	char	 *c_calling_address;
	char     *c_called_address;
	char     *c_facf;
	unsigned short c_facl;
	char     *c_cudf;
	unsigned short c_cudl;
};

/*
 * Parameter of the RESET, GETRESET, CLEAR and GETCLEAR ioctl.
 */

struct svcreason {
	char	  r_cause;
	char	  r_diag;
};

/*
 * The parameter of the GETFLAG/SETFLAG ioctl is a pointer to a long word
 * with the following bits defined.
 */

#define FL_PUSER	1		/* `printfs' to user instead of cons. */
#define FL_PMAP		2		/* Print mapping of svc's to chan's. */
#define FL_PCLEAR	4		/* Print incom. calls w/o listener. */
#define FL_PSYS		8		/* Print system calls. */

/*
 * Parameter of the STAT ioctl.
 */

#ifndef MAXCHAN
#include <sys/x25param.h>
#endif

struct svcstat {
	short s_hwinfo;		/* Misc. hardware info. */
	char *s_hwaddr;		/* Controller HW Base (Debugging only). */
	short s_l2state;	/* HDLC is up if l2state != 0. */
	short s_l3state;	/* Level 3 is up if l3state != 0. */
	short s_nchan;		/* Actual numbers of channels available. */
	struct s_chan {
		short  s_state;	/* Channel state. */
		char   s_channo;/* True chan. number, if state==S_CONNECTED. */
		char   s_protid;/* Protocol-id, if state==S_LISTEN. */
		short  s_pgrp;	/* Process group. */
	} s_chan[MAXCHAN+1];
};

/* s_hwinfo bits. */

#define S_HWNUM		0x03		/* Mask 2 bit: board number 0..3. */
#define S_IS_UP		0x08		/* Is downloaded and running. */
/* HWNUM is not valid if !IS_UP. */
/* Bit 0x04 is no longer needed. */

/* s_chan[].s_state codes. */

#define S_NOTOPEN	-1	/* Not open. */
#define S_OPEN		 0	/* Open, but no listen and not connected. */
#define S_LISTEN	 1	/* Waiting for incoming calls. */
#define S_CONNECTED	 2	/* Connection established. */

/*
 * Parameter of the SETACCESS ioctl is specified in <sys/x25access.h>.
 */

/*
 * Parameter of the SETVERSION ioctl is a null terminated string of
 * at most 10 bytes (including the null byte).
 */

/*
 * Reading channel 0 returns a code in the first byte and than the data.
 * The following codes are defined:
 */

#define RD0_TRACE	'T'			/* Trace info. */
#define RD0_PRINTF	'P'			/* Printf info. */
#define RD0_ACCOUNT	'A'			/* Account data. */
#define RD0_REQUEST	'R'			/* Access cntrl: user req. */
