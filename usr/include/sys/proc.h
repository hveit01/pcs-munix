/* @(#)proc.h	6.4 */
/*
 * One structure allocated per active process. It contains all data needed
 * about the process while the process may be swapped out.
 * Other per process data (user.h) is swapped with the process.
 */

struct	proc {
	char	p_stat;
	unsigned p_flag;
	short   p_pri;          /* priority, negative is high */
	char    p_inter;        /* interactive process */
	char	p_time;		/* resident time for scheduling */
	char	p_cpu;		/* cpu usage for scheduling */
	char	p_nice;		/* nice for cpu usage */
	ushort	p_uid;		/* real user id */
	ushort	p_suid;		/* set (effective) user id */
	short	p_pgrp;		/* name of process group leader */
	short	p_pid;		/* unique process id */
	short	p_ppid;		/* process id of parent */
	short   p_addr;         /* number of page with u structure */
	struct pregion *p_region; /* pointer to process regions */
	short   p_size;         /* size of swappable image (pages) */
	long	p_sig;		/* signals pending to this process */
	caddr_t p_wchan;        /* event process is awaiting */
	ushort	p_nvfault;	/* number of page valid faults */
	char	p_frate;	/* page fault rate */
	struct proc *p_link;	/* linked list of running processes */
	long    p_clktim;       /* time to alarm clock signal */
/* Cadmus specific */
	short   p_cursig;       /* current signal proccessing */
	long	p_sigmask;	/* current signal mask */
	long	p_sigignore;	/* signals being ignored */
	long	p_sigcatch;	/* signals being caught by user */
# ifdef MUNET
	struct munetinfo *p_munetinfo; /* pointer to MUNIX/NET info or NULL */
#endif
};

extern struct proc proc[];	/* the proc table itself */

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */
#define	SXBRK	7		/* process being xswapped */
#define	SXSTK	8		/* process being xswapped */
#define	SXTXT	9		/* process being xswapped */

/* flag codes */
#define	SLOAD	01		/* in core */
#define	SSYS	02		/* scheduling process */
#define	SLOCK	04		/* process cannot be swapped */
#define	SSWAP	010		/* process is being swapped out */
#define	STRC	020		/* process is being traced */
#define	SWTED	040		/* another tracing flag */
#define	STEXT	0100		/* text pointer valid */
#define	SSPART	0200		/* process is partially swapped out */
/* Cadmus specific */
# ifdef MUNET
#define	SFSERV	0400		/* process is a receiver */
#endif
#define	SOMASK	02000		/* restore old mask after taking signal */
#define	SOUSIG	04000		/* using old signal mechanism */
#ifdef SELECT
#define SSEL    010000          /* proc doing a select call/timeout     */
#endif

/*
 * parallel proc structure
 * to replace part with times
 * to be passed to parent process
 * in ZOMBIE state.
 */
#ifndef NPROC
struct	xproc {
	char	xp_stat;
	unsigned xp_flag;
	short   xp_pri;         /* priority, negative is high */
	char    xp_inter;        /* interactive process */
	char	xp_time;	/* resident time for scheduling */
	char	xp_cpu;		/* cpu usage for scheduling */
	char	xp_nice;	/* nice for cpu usage */
	ushort	xp_uid;		/* real user id */
	ushort	xp_suid;	/* set (effective) user id */
	short	xp_pgrp;	/* name of process group leader */
	short	xp_pid;		/* unique process id */
	short	xp_ppid;	/* process id of parent */
	short	xp_addr;
	struct pregion *xp_region;
	short	xp_size;
	short	xp_xstat;	/* Exit status for wait */
	time_t	xp_utime;	/* user time, this proc */
	time_t	xp_stime;	/* system time, this proc */
};
#endif
