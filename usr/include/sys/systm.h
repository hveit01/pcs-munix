/* @(#)systm.h	6.2 */
/*
 * Random set of variables used by more than one routine.
 */
extern struct inode *rootdir;	/* pointer to inode of root directory */
extern short cputype;		/* type of cpu = 40, 45, 70, 750, 780 */
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

extern	maxmem;			/* max available memory */
extern	maxumem;		/* max user proc virt mem */
extern	physmem;		/* physical memory on this CPU */
extern  availsmem;              /* available swap memory */
extern daddr_t swplo;		/* block number of swap space */
extern	nswap;			/* size of swap space */
extern dev_t rootdev;		/* device of the root */
extern dev_t swapdev;		/* swapping device */
extern dev_t pipedev;		/* pipe device */
extern char *panicstr;		/* panic string pointer */
extern	blkacty;		/* active block devices */
extern	pwr_cnt, pwr_act;
extern int (*pwr_clr[])();

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
struct inode *owner();
struct inode *maknode();
struct inode *namei();
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

int	uchar();

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	char	sy_narg;		/* total number of arguments */
	char	sy_setjmp;		/* set to 1 if systrap() should not do a setjmp() */
	int	(*sy_call)();		/* handler */
	char    *sy_fmt;                /* used for conversion of old syscalls */
} sysent[];
extern int sysentlen;

/* Cadmus specific */
extern int     hz, tz, dstflag; /* line frequency and  geographical timezone*/
extern int     haveclock;               /* true if microprocessor clock attached */
extern time_t  bootime;                 /* time at which booted */
extern short _ptbr[];           /* HW address of PTBR */

extern int master;              /* master/diskless node flag */
extern int ownswap;             /* owner of swap device flag */
extern int comswapsmi;          /* common swaptable entry    */

extern short _pcr;              /* processor control register */
extern short _esr;              /* bus error status */
extern short _ccr;              /* cache control register */
extern short _pbc;              /* cache control register */
extern int _dmapt[];            /* Q-Bus DMA map */
