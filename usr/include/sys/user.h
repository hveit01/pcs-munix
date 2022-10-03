/* @(#)user.h   6.4 */
/*
 * The user structure.
 * One allocated per process.
 * Contains all per process data that doesn't need to be referenced
 * while the process is swapped.
 * The user block is USIZE pages long; resides at virtual kernel
 * loc 0x100000-0x1000=0xff000;
 * contains the system stack per user; is cross referenced
 * with the proc structure for the same process.
 */
 
#ifndef FPVECSIZE
#include "reg.h"
#endif

#define PSARGSZ		40

struct	user
{
	label_t	u_rsav;			/* save info when exchanging stacks */
	label_t	u_qsav;			/* label variable for quits and interrupts */
	label_t	u_ssav;			/* label variable for swapping */
	char	u_segflg;		/* IO flag: 0:user D; 1:system; 2:user I */
	ushort  u_error;                /* return error code */
	ushort	u_uid;			/* effective user id */
	ushort	u_gid;			/* effective group id */
	ushort	u_ruid;			/* real user id */
	ushort	u_rgid;			/* real group id */
	struct proc *u_procp;		/* pointer to proc structure */
	long    *u_ap;                  /* pointer to arglist */
	union {				/* syscall return values */
		long    r_val;
		off_t	r_off;
		time_t	r_time;
	} u_r;
	caddr_t	u_base;			/* base address for IO */
	ulong   u_count;                /* bytes remaining for IO */
	off_t	u_offset;		/* offset in file for IO */
	short	u_fmode;		/* file mode for IO */
	ushort	u_pbsize;		/* bytes in block for IO */
	ushort	u_pboff;		/* offset in block for IO */
	dev_t	u_pbdev;		/* real device for IO */
	daddr_t	u_rablock;		/* read ahead block addr */
	daddr_t	u_xablock;		/* read ahead ahead block addr */
	struct inode *u_cdir;		/* current directory of process */
	struct inode *u_rdir;		/* root directory of process */
	caddr_t	u_dirp;			/* pathname pointer */
	struct direct u_dent;		/* current directory entry */
	struct inode *u_pdir;		/* inode of parent directory of dirp */
	struct file *u_ofile[NOFILE];	/* pointers to file structures of open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
	short   u_nsegs;                /* number of ptbr entries */
	struct  ptbrmap *u_pmap;        /* pointer to precomputed ptbr entries */
	long    u_arg[15];              /* arguments to current system call */
	ulong   u_tsize;                /* text size (pages) */
	ulong   u_datorg;               /* start virtual address for data */
	ulong   u_dsize;                /* data size (pages) */
	ulong   u_ssize;                /* stack size (pages) */
	long    u_signal[NSIG];         /* disposition of signals */
	time_t	u_utime;		/* this process user time */
	time_t	u_stime;		/* this process system time */
	time_t	u_cutime;		/* sum of childs' utimes */
	time_t	u_cstime;		/* sum of childs' stimes */
	time_t	u_agetime;		/* last time process regions */
					/* were aged: utime + stime */
	struct exvec *u_exvec;          /* address of users exception vector*/
	char    u_m881;                 /* if true then fp context must be saved */
	char    u_fpsaved;              /* if true then fp regs are saved */
	struct  fp_vec u_fpvec;         /* here the 68881 state is stored */
	char    u_fpstate[4+180];       /* result of fstore instruction */
	struct prof {                   /* profile arguments */
		short   *pr_base;       /* buffer base */
		long     pr_size;       /* buffer size */
		long     pr_off;        /* pc offset */
		ulong    pr_scale;      /* pc scaling */
	} u_prof;
	char	u_intflg;		/* catch intr from sys */
	char	u_sep;			/* flag for I and D separation */
	short	*u_ttyp;		/* pointer to pgrp in "tty" struct */
	dev_t	u_ttyd;			/* controlling tty dev */
	struct ufhd {			/* header of executable file */
		short	ux_mag;		/* magic number */
		short	ux_stamp;	/* stamp */
		ulong   ux_tsize;       /* text size */
		ulong   ux_dsize;       /* data size */
		ulong   ux_bsize;       /* bss size */
		ulong   ux_entloc;      /* entry location */
		ulong   ux_txtorg;
		ulong   ux_datorg;
	} u_exdata;
	ulong   ux_tstart;
	char	u_comm[DIRSIZ];		/* command name for accounting */
	char    u_psargs[PSARGSZ];      /* command and args printed by ps */
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

	char	u_callno;		/* current syscall entry number */
					/* used by network for dispatching */

/* MUNIX/NET */
# ifdef MUNET
        struct inode *u_namei_rv;       /* namei return value */
        struct file *u_getf_rv;         /* getf return value */
        char u_locate_flag;             /* set if namei or getf is done */

	dev_t u_cdirdev;		/* remote lan device if cdir points to
						network */
	dev_t u_crootdev;		/* remote lan device if cdir points to
						network */
	struct inode *u_execp;		/* pointer to inode in use by exec
						across lan */
	char	u_rofile[NOFILE];	/* remote file index on network */
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

/* Berkeley 4.1 signal stuff */
	char	u_eosys;		/* special action on end of syscall */
	short	u_errcnt;		/* syscall error count */
	long	u_sigmask[NSIG];	/* signals to be blocked */
	long	u_sigonstack;		/* signals to take on sigstack */
	long	u_oldmask;		/* saved mask from before sigpause */
	struct	sigstack u_sigstack;	/* sp & on stack state variable */
#define	u_onstack	u_sigstack.ss_onstack
#define	u_sigsp		u_sigstack.ss_sp

/*      short pad;      /* for the benefit of crash(8), let sizeof(u) be
			   a multiple of 4 */

	int	u_stack[1];
					/* kernel stack per user
					 * extends from u + ptob(USIZE)
					 * backward not to reach here
					 */
};
extern struct user u;

#define	u_rval1	u_r.r_val
#define	u_roff	u_r.r_off
#define	u_rtime	u_r.r_time

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

/* u_eosys values */
#define	JUSTRETURN	0
#define	RESTARTSYS	1

/* u.u_sep values */
#define NOSEPID         0       /* no separate I/D space */
#define SEPID           1       /* separate I/D space */
#define	EGRESS		6	/* mask for the following conditions */
#define DSEXAL          2       /* execution from data space allowed */
#define	TSWRAL		4	/* text space write allowed */
