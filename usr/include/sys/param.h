/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident        "@(#)kern-port:sys/param.h      10.10"*/

#undef  PRERELEASE
#define PCS 1
#define M32 1
#ifndef C20
#define MUNET 1                 /* inserts code for MUNIX/NET */
#define DISKLESS 1              /* diskless node code         */
#endif
#define SELECT 1                /* code for SELECT            */

/*
 * fundamental variables
 * don't change too often
 */
#include "sys/fs/s5param.h"

#define	MAXPID	30000		/* max process id */
#define	MAXUID	60000		/* max user id */
#define	MAXLINK	1000		/* max links */

#define MAXBLK  15              /* max pages possible for phys IO */
#define SSIZE   1               /* initial stack size (*4096 bytes) */
#define SINCR   1               /* increment of stack (*4096 bytes) */
#define USIZE   1               /* size of user block (*4096 bytes) */
#ifndef C20
#define USRSTACK 0x3f7ff000     /* Start of user stack */
#else
#define USRSTACK 0x80000000     /* Start of user stack */
#endif

#define	CANBSIZ	256		/* max size of typewriter line	*/
extern int hz;
#define HZ      hz              /* Ticks/second of the clock */
#define NCARGS  10240           /* # characters in exec arglist */
				/*   must be multiple of NBPW.  */

#define SCHMAX  127

/*	The following define is here for temporary compatibility
**	and should be removed in the next release.  It gives a
**	value for the maximum number of open files per process.
**	However, this value is no longer a constant.  It is a
**	configurable parameter, NOFILES, specified in the kernel
**	master file and available in v.v_nofiles.  Programs which
**	include this header file and use the following value may
**	not operate correctly if the system has been configured
**	to a different value.
*/

#define NOFILE  20

/*	The following represent the minimum and maximum values to
**	which the paramater NOFILES in the kernel master file may
**	be set.
*/

#define	NOFILES_MIN	 20
#define	NOFILES_MAX	100


/*
 * priorities
 * should not be altered too much
 */

#define PMASK   0377
#define	PCATCH	0400
#define	PNOSTOP	01000
#define	PSWP	0
#define PMEM	0
#define	PINOD	10
#define	PRIBIO	20
#define	PZERO	25
#define NZERO   20      /* basic nice value             */
#define NINTER  29      /* max interactive nice value   */
#define	PPIPE	26
#define	PWAIT	30
#define	PSLEP	39
#define	PUSER	60
#define PINTER  127
#define PIDLE   255

#define PINTERSEC 5 /* an interactive Process should react within 5 seconds */

/*
 * fundamental constants of the implementation--
 * cannot be changed easily
 */

#define	NBPW	sizeof(int)	/* number of bytes in an integer */

#define NPPS    256             /* number of pages per segment */
#define NBPS    0x100000        /* Number of bytes per segment */
#define NBPP    4096            /* Number of bytes per page */
#define NBPC    1024            /* Number of bytes per click */
#define NDPP    8               /* number of disk blocks per page */

#define DPPSHFT         3       /* LOG2(NDPP) if exact */
#define BPPSHIFT        12      /* LOG2(NBPP) if exact */
#define BPCSHIFT        10      /* LOG2(NBPC) if exact */
#define SOFFMASK        0xFFFFF /* mask for offset into segment */
#define POFFMASK        0xFFF   /* Mask for offset into page. */
#define COFFMASK        0x3FF   /* Mask for offset into click. */

#ifndef NULL
#define NULL    0L
#endif

#define MAXSUSE 255             /* maximum share count on swap */

#define CMASK   2               /* default mask for file creation */
#define	NODEV	(dev_t)(-1)
#define	NBPSCTR		512	/* Bytes per disk sector.	*/
#define SCTRSHFT	9	/* Shift for BPSECT.		*/

#define	UMODE	3		/* current Xlevel == user */
#define USERMODE(ps)  ((ps & PS_S)==0)
#define	BASEPRI(ps)	((ps & PS_IPL) != 0)

#define lobyte(X)       (((unsigned char *)&X)[1])
#define hibyte(X)       (((unsigned char *)&X)[0])
#define loword(X)       (((ushort *)&X)[1])
#define hiword(X)       (((ushort *)&X)[0])

/* Cadmus Stuff */
#define MAXPORTS 50             /* max network ports for Newcastle */

/* REMOTE -- whether machine is primary, secondary, or regular */
#define SYSNAME 9		/* # chars in system name */
#define PREMOTE 39

#define MSGBUFS 5000 /* was 256; changed for error logging (KS)         */

/* symbolic links */
#define NSYMBUF           8
#define MAXPATHLEN      256
