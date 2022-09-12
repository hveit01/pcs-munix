/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)kern-port:sys/systm.h	10.2"*/

/*
 * Random set of variables used by more than one routine.
 */

extern struct inode *rootdir;	/* pointer to inode of root directory */
extern short cputype;		/* type of cpu = 40, 45, 70, 780, 0x3b5 */
extern time_t lbolt;		/* time in HZ since last boot */
extern time_t time;		/* time in sec from 1970 */
extern int      bdflushcnt;     /* counter for t_bdflushr */
extern uint     sxbrkcnt;       /* count of procs whose current stat is SXBRK   */

extern char runin;		/* scheduling flag */
extern char runout;		/* scheduling flag */
extern char runrun;		/* scheduling flag */
extern short curpri;            /* current priority */
extern struct proc *curproc;	/* current proc */
extern struct proc *runq;	/* head of linked list of running processes */

extern		maxmem;		/* max available memory (clicks) */
extern          maxumem;        /* max user proc virt mem */
extern		physmem;	/* physical memory (clicks) on this CPU */
extern          availsmem;                      /* available swap memory */
extern daddr_t	swplo;		/* block number of start of swap space */
extern		nswap;		/* size of swap space in blocks*/
extern dev_t	rootdev;	/* device of the root */
extern dev_t	swapdev;	/* swapping device */
extern dev_t	pipedev;	/* pipe device */
extern char	*panicstr;	/* panic string pointer */
extern char	qrunflag;
extern		blkacty;	/* active block devices */
extern		pwr_cnt, pwr_act;
extern int 	(*pwr_clr[])();

/*
 * Nlandev is the number of entries
 * (rows) in the network switch. It is
 * set in linit by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */
int	nlandev;

dev_t getmdev();
daddr_t	bmap();
struct inode *ialloc();
struct inode *iget();
struct inode *namei();
struct inode *owner();
struct inode *maknode();
struct inode *remote_call();
struct buf *alloc();
struct buf *getblk();
struct buf *geteblk();
struct buf *bread();
struct buf *breada();
struct file *getf();
struct file *falloc();
union pte *ublkptaddr();
caddr_t sptalloc();
caddr_t pgalloc();
caddr_t listalloc();
long fslong(), fulong();
int	upath();
int     spath();
struct proc *prfind();

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	char	sy_narg;		/* total number of arguments */
	char	sy_setjmp;		/* set to 1 if systrap() should not do a setjmp() */
	int	(*sy_call)();		/* handler */
	char    *sy_fmt;                /* used for conversion of old syscalls */
	short   sy_ducallno;            /* to be put in u.u_syscall for RFS*/
} sysent[];
extern int sysentlen;


/* Cadmus specific */
extern int      hz, tz, dstflag; /* line frequency and  geographical timezone*/
extern int      haveclock;      /* true if microprocessor clock attached */
extern time_t   bootime;        /* time at which booted */
extern int      _dmapt[];       /* Q-Bus DMA map */
extern int      in_interrupt;   /* are we called from an interrupt handler? */
extern char     netisrflag;     /* software interrupt simulator for socket code */

extern int master;              /* master/diskless node flag */
extern int ownswap;             /* owner of swap device flag */
extern int comswapsmi;          /* common swaptable entry    */

#ifndef C20
extern short _ptbr[];           /* megapage table on cpu board (page table base registers) */
extern short _pcr;              /* processor control register */
extern short _esr;              /* bus error status register */
extern short _ccr;              /* cache control register */
extern short _pbc;              /* P-Bus configuration register */
#elif C18
extern long _ptbr[];            /* first mmu level descriptor table */
extern long  _pcr;              /* processor board control register */
extern long  _esr;              /* processor board status register */
#else
extern long _ptbr[];            /* first mmu level descriptor table */
extern short _pcr;              /* processor board control register */
extern short _esr;              /* processor board status register */
extern short _vmecr;            /* VME control register  */
extern long _mpr;               /* memory parity register */
extern char _idr;               /* memory identification register */
extern int _dmapt[];            /* VME-Bus DMA map */
#endif
