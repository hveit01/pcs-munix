/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)kern-port:sys/user.h	10.15"*/

/*
 * The user structure.
 * One allocated per process.
 * Contains all per process data that doesn't need to be referenced
 * while the process is swapped.
 * The user block is USIZE*NBPP bytes long; resides at virtual kernel
 * 0x3f7ff000 (Cadmus M32 ) resp. 0xf87ff000 (C20)
 * contains the system stack per process;
 * is cross referenced with the proc structure for the same process.
 */
 
#ifndef FPVECSIZE
#include "reg.h"
#endif

#define PSARGSZ 40      /* Space in u-block for exec arguments. */
			/* Used by ps command.			*/

#define PSCOMSIZ        DIRSIZ  /* For the time being set PSCOMSIZ */
				/* to DIRSIZ until we can get rid of */
				/* struct direct u_dent */
#define SYSMASKLEN      4       /* number of entries in sysent / 32 rounded up */

typedef	struct	user
{
	label_t	u_rsav;			/* save info when exchanging stacks */
	label_t	u_qsav;			/* label variable for quits and interrupts */
	long	u_bsize;		/* block size of device */
	char	u_psargs[PSARGSZ];	/* arguments from exec system call */
	char	*u_tracepc;	/* Return PC if tracing enabled */
	int	u_sysabort;	/* Debugging: if set, abort syscall */
	int	u_systrap;	/* Are any syscall mask bits set? */ 
	long	u_entrymask[SYSMASKLEN]; /* syscall stop-on-entry mask */
	long	u_exitmask[SYSMASKLEN];	/* syscall stop-on-exit mask */
	long 	u_rcstat; 	/* Client cache status flags */

	void(*u_signal[NSIG])();        /* disposition of signals */

	char	u_segflg;	/* IO flag: 0:user D; 1:system;	*/
				/*          2:user I		*/

	short   u_error;        /* return error code */

	ushort	u_uid;		/* effective user id */
	ushort	u_gid;		/* effective group id */
	ushort	u_ruid;		/* real user id */
	ushort	u_rgid;		/* real group id */

	struct proc *u_procp;	/* pointer to proc structure */

	int	*u_ap;		/* pointer to arglist */

	union {			/* syscall return values */
		long r_val;
		off_t	r_off;
		time_t	r_time;
	} u_r;

	caddr_t	u_base;		/* base address for IO */
	unsigned u_count;	/* bytes remaining for IO */
	off_t	u_offset;	/* offset in file for IO */
	short	u_fmode;	/* file mode for IO */
	ushort	u_pbsize;	/* Bytes in block for IO */
	ushort	u_pboff;	/* offset in block for IO */
	dev_t	u_pbdev;	/* real device for IO */
	daddr_t	u_rablock;	/* read ahead block address */
	daddr_t	u_xablock;		/* read ahead ahead block addr */
	short	u_errcnt;	/* syscall error count */

	struct inode *u_cdir;	/* current directory */

	struct inode *u_rdir;	/* root directory */
	caddr_t	u_dirp;		/* pathname pointer */
	struct direct u_dent;	/* current directory entry */
	struct inode *u_pdir;	/* inode of parent directory	*/
				/* of dirp			*/


	short   u_nsegs;        /* number of ptbr entries */
	struct  ptbrmap *u_pmap;/* pointer to precomputed ptbr entries */
	int     u_arg[6];       /* arguments to current system call */

	unsigned u_tsize;	/* text size (clicks) */
	unsigned u_dsize;	/* data size (clicks) */
	unsigned u_ssize;	/* stack size (clicks) */

	time_t	u_utime;	/* this process user time */
	time_t	u_stime;	/* this process system time */
	time_t	u_cutime;	/* sum of childs' utimes */
	time_t	u_cstime;	/* sum of childs' stimes */

	struct exvec *u_exvec;          /* address of users exception vector*/
	char    u_m881;                 /* if true then fp context must be saved */
	char    u_fpsaved;              /* if true then fp regs are saved */
	struct  fp_vec u_fpvec;         /* here the 68881 state is stored */
	char    u_fpstate[4+180];       /* result of fstore instruction */
	short   u_m68341[5+8*6];        /* work area for m68341 code */

	struct prof {                   /* profile arguments */
		short   *pr_base;       /* buffer base */
		long     pr_size;       /* buffer size */
		long     pr_off;        /* pc offset */
		ulong    pr_scale;      /* pc scaling */
	} u_prof;
	short  *u_ttyp;			/* pointer to pgrp in "tty" struct */
	dev_t	u_ttyd;			/* controlling tty dev */
	struct inode *u_ttyip;		/* inode of controlling tty (streams) */

	long   u_execsz;

	/*
	 * Executable file info.
	 */
	struct exdata {
		short	ux_mag;		/* magic number */
		short	ux_stamp;	/* stamp */
		ulong   ux_tsize;       /* text size */
		ulong   ux_dsize;       /* data size */
		ulong   ux_bsize;       /* bss size */
		ulong   ux_entloc;      /* entry location */
		ulong   ux_txtorg;      /* start addr. of text in memory */
		ulong   ux_datorg;      /* start addr. of data in memory */
					/* up to here like aouthdr */
		long    ux_lsize;       /* lib size     */
		long    ux_nshlibs;     /* number of shared libs needed */
		long    ux_toffset;     /* file offset to raw text      */
		long    ux_doffset;     /* file offset to raw data      */
		long    ux_loffset;     /* file offset to lib sctn      */
		struct  inode  *ip;
	} u_exdata;

	char	u_comm[PSCOMSIZ];

	time_t	u_start;
	time_t	u_ticks;
	long	u_mem;
	long	u_ior;
	long	u_iow;
	long	u_iosw;
	long	u_ioch;
	char	u_acflag;

	short	u_cmask;		/* mask for file creation */
	daddr_t	u_limit;		/* maximum write address */

	short	u_lock;			/* process/text locking flags */

	char    *u_pofile;              /* per-process flags of open files */

/* MUNIX/NET */
# ifdef MUNET
        struct inode *u_namei_rv;       /* namei return value */
        struct file *u_getf_rv;         /* getf return value */
        char u_locate_flag;             /* set if namei or getf is done */
	short u_namircode;              /* return code in struct argnamei */

	dev_t u_cdirdev;		/* remote lan device if cdir points to
						network */
	dev_t u_crootdev;		/* remote lan device if cdir points to
						network */
	struct inode *u_execp;		/* pointer to inode in use by exec
						across lan */
	char    *u_rofile;              /* remote file index on network */
	char    u_rrarg;                /* argument return for MUNIX/NET */
	char    u_rrflag;               /* flag return for MUNIX/NET */
	long    u_munet;                /* internal argument passer */
	dev_t   u_rmt_dev;              /* for shared code via MUNIX/NET     */
	ino_t   u_rmt_ino;              /*              "                    */
	ushort  u_rmt_mode;             /*              "                    */
	dev_t   u_rmt_id;               /*              "                    */
	char    u_idflag;               /* save value for UIISEXEC   */
	long    u_isid;                 /* save value for UIISEXEC  */
# endif
#ifdef INODEBUG
	char	u_filler1[40];	/* DON'T TOUCH--this is used by
				 * conditionally-compiled code in iget.c
				 * which checks consistency of inode locking
				 * and unlocking.  Name change to follow in
				 * a later release.
				 */
#endif
	char	u_nshmseg;	/* Nbr of shared memory		*/
				/* currently attached to the	*/
				/* process.			*/

 	struct rem_ids {		/* for exec'ing REMOTE text */
 		ushort	ux_uid;		/* uid of exec'd file */
 		ushort	ux_gid;		/* group of exec'd file */
 		ushort	ux_mode;	/* file mode (set uid, etc. */
 	} u_exfile;
	char	*u_comp;	/* pointer to current component */
	char	*u_nextcp;	/* pointer to beginning of next */
					/* following for Distributed UNIX */
	ushort          u_rflags;       /* flags for distribution */
	int             u_callno;       /* system call number */
	int             u_syscall;      /* system call number for RFS */
	int		u_mntindx;	/* mount index from sysid */
	struct sndd	*u_gift;	/* gift from message      */
	struct response	*u_copymsg;	/* copyout unfinished business */
	struct msgb	*u_copybp;	/* copyin premeditated send    */
	char 		*u_msgend;	/* last byte of copymsg + 1    */
					/* end of Distributed UNIX */
	
	struct file *u_ofile[1];        /* pointers to file structures of open files */

	int	u_stack[1];
					/* kernel stack per user
					 * extends from u + ptob(USIZE)
					 * backward not to reach here
					 */
} user_t;

extern struct user u;

#define u_exuid u_exfile.ux_uid
#define u_exgid u_exfile.ux_gid
#define u_exmode u_exfile.ux_mode

#define u_rval1 u_r.r_val
#define	u_roff	u_r.r_off
#define	u_rtime	u_r.r_time

/* rcstat values: for client caching */

#define	U_RCACHE	0x1

/* ioflag values: Read/Write, User/Kernel, Ins/Data */

#define	U_WUD	0
#define	U_RUD	1
#define	U_WKD	2
#define	U_RKD	3
#define	U_WUI	4
#define	U_RUI	5

/* Definitions for pofile */
#define	EXCLOSE	01

# ifdef MUNET
#define FIRSTBL 04              /* PCS for MUNIX/NET */
#define LASTBL  010             /* PCS for MUNIX/NET */
#define REMPIPE 020             /* remote inode mode for delayed lseek */
#define MUSTLS  040             /* receiver must do lseek before I/O */
# endif

/* distribution: values for u_rflags */
#define FREMOTE	0x0002	/* file is remote  */

#define	U_RSYS		0x0004	/* system call has gone remote */
#define	U_DOTDOT	0x0200
#define U_RCOPY		0x0400	/* used by copyout for non-delay copy */

#ifndef C20
#define U_ADDR  0x3f7ff000
#else
#define U_ADDR  0xf87ff000
#endif
