/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* WARNING: if you change here, look at ml/userio.s and change there too! */

/*#ident	"@(#)kern-port:sys/proc.h	10.10"*/
/*	One structure allocated per active process. It contains all
**	data needed about the process while the process may be swapped
**	out.  Other per process data (user.h) may swapped with the
**	process but in fact it is not.
*/

typedef struct	proc	{
	short   p_pri;                  /* priority, negative is high */
	char	p_stat;			/* Status of process.	     */
	char    p_cpu;                  /* cpu usage for scheduling */
	char    p_inter;                /* interactive process */
	char	p_nice;			/* nice for cpu usage */
	uint	p_flag;			/* Flags defined below.	     */
	ushort	p_uid;			/* real user id */
	ushort	p_suid;			/* saved (effective) user id */
					/* from exec.		     */
	short	p_pgrp;			/* name of process group     */
					/* leader		     */
	short	p_pid;			/* unique process id */
	short	p_ppid;			/* process id of parent */
	short   p_addr;                 /* number of page with u structure */
	ushort	p_sgid;			/* saved (effective) group   */
					/* id from exec.	     */
	int	p_sig;			/* signals pending to this   */
					/* process		     */
	struct	proc	*p_flink;	/* linked list of processes */
	struct	proc	*p_blink;	/* linked list of processes */
	caddr_t	p_wchan;		/* Wait addr for sleeping   */
					/* processes.		    */
	struct	proc	*p_parent;	/* ptr to parent process    */
	struct	proc	*p_child;	/* ptr to first child process */
	struct	proc	*p_sibling;	/* ptr to next sibling	    */
					/* process on chain	    */
	int	p_clktim;		/* time to alarm clock signal */
	uint	p_size;			/* size of swappable image  */
					/* in pages.		    */
	time_t	p_utime;		/* user time, this proc */
	time_t	p_stime;		/* system time, this proc */
	struct  proc *p_mlink;		/* link list of processes    */
					/* sleeping on memwant or    */
					/* swapwant.	  	     */
	struct  pregion *p_region;      /* pointer to process regions */
	ushort	p_mpgneed;		/* number of memory pages    */
					/* needed in memwant.	     */
	char	p_time;			/* resident time for scheduling */
	unchar	p_cursig;		/* current signal */
	short	p_epid;			/* effective pid             */
					/* normally - same as p_pid  */
					/* if server - p_pid that sent msg */
	sysid_t p_sysid;		/* normally - same as sysid */
					/* if server - system that sent msg */
	struct	rcvd  *p_minwd;		/* server msg arrived on this queue */
	struct	proc  *p_rlink;		/* linked list for server */
	int	p_trlock;
	struct inode *p_trace;		/* pointer to /proc inode */
	long	p_sigmask;		/* tracing signal mask for /proc */
	int	p_hold;			/* hold signal bit mask */
	int	p_chold;		/* defer signal bit mask */
					/* sigset turns on this bit */
					/* signal does not turn on this bit */
	short	p_xstat;		/* exit status for wait */
	ushort	p_whystop;		/* Reason for process stop */
	ushort	p_whatstop;		/* More detailed reason */

# ifdef MUNET
	struct munetinfo *p_munetinfo; /* pointer to MUNIX/NET info or NULL */
#endif

} proc_t;

#define	p_link	p_flink

extern struct proc proc[];		/* the proc table itself */

/* stat codes */

#define	SSLEEP	1		/* Awaiting an event.		*/
#define	SRUN	2		/* Running.			*/
#define	SZOMB	3		/* Process terminated but not	*/
				/* waited for.			*/
#define	SSTOP	4		/* Process stopped by signal	*/
				/* since it is being traced by	*/
				/* its parent.			*/
#define	SIDL	5		/* Intermediate state in	*/
				/* process creation.		*/
#define	SONPROC	6		/* Process is being run on a	*/
				/* processor.			*/
#define SXBRK	7		/* process being xswapped       */

/* flag codes */

#define	SSYS	0x0001		/* System (resident) process.	*/
#define	STRC	0x0002		/* Process is being traced.	*/
#define	SWTED	0x0004		/* Stopped process has been	*/
				/* given to parent by wait	*/
				/* system call.  Don't return	*/
				/* this process to parent again	*/
				/* until it runs first.		*/
#define SNWAKE	0x0008		/* Process cannot wakeup by	*/
				/* a signal.			*/
#define SLOAD   0x00000010      /* in core                      */
#define SLOCK   0x00000020      /* Process cannot be swapped.   */
#define SRSIG   0x00000040      /* Set when signal goes remote  */
#define SPOLL   0x00000080      /* Process in stream poll       */
#define SPRSTOP 0x00000100      /* process is being stopped via /proc */
#define SPROCTR 0x00000200      /* signal tracing via /proc */
#define SPROCIO 0x00000400      /* doing I/O via /proc, so don't swap */
#define SSEXEC  0x00000800      /* stop on exec */
#define SPROPEN 0x00001000      /* process is open via /proc */
#define	SULOAD  0x00002000	/* u-block in core */
#define SRUNLCL 0x00004000      /* set process running on last /proc close */
#define	SNOSTOP	0x00008000	/* proc asleep, stop not allowed */
#define	SPTRX	0x00010000	/* process is exiting via ptrace(2) */
#define	SASLEEP	0x00020000	/* proc is stopped within a call to sleep() */
#define	SUSWAP	0x00040000	/* u-block is being swapped in or out */
#define	SUWANT	0x00080000	/* waiting for u-block swap to complete */
/* Cadmus specific */
# ifdef MUNET
#define SFSERV  0x00100000      /* process is a receiver */
#endif
#ifdef SELECT
#define SSEL    0x00200000      /* proc doing a select call/timeout     */
#endif

#define PTRACED(p)	((p)->p_flag&(STRC|SPROCTR|SSEXEC|SPROPEN))

/* Flags for newproc() */

#define NP_FAILOK	0x1	/* don't panic if cannot create process */
#define NP_NOLAST	0x2	/* don't use last process slot */
#define	NP_SYSPROC	0x4	/* system (resident) process */

/* Reasons for stopping (values of p_whystop) */

#define	REQUESTED	1
#define	SIGNALLED	2
#define	SYSENTRY	3
#define	SYSEXIT		4

/* Macro to reduce unnecessary calls to issig() */

#define	ISSIG(p, why) \
  ((p)->p_cursig || (((p)->p_sig || ((p)->p_flag & SPRSTOP)) && issig(why)))

/* Reasons for calling issig() */

#define	FORREAL		0	/* Usual side-effects */
#define	JUSTLOOKING	1	/* Don't stop the process */
