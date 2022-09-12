/* @(#)space.h  6.13 */

#ifndef KERNEL
#define	KERNEL
#endif
#include "sys/acct.h"
struct	acct	acctbuf;
struct	inode	*acctp;
short acctdev = NODEV;

#include "sys/tty.h"
struct	cblock	cfree[NCLIST];

#ifndef	B_READ
#include "sys/buf.h"
#endif
struct	buf	bfreelist;	/* head of the free list of buffers */
struct	pfree	pfreelist;	/* Head of physio header pool */
int		pfreecnt;	/* Count of free physio buffers.    */
struct	buf	pbuf[NPBUF];	/* Physical io header pool */
struct	hbuf	hbuf[NHBUF];	/* buffer hash table */

#include "sys/file.h"
struct	file	file[NFILE];	/* file table */

#include "sys/inode.h"
struct inode	*rootdir;	/* Inode for root directory. */
struct	inode	inode[NINODE];	/* inode table */

#include "sys/fs/s5inode.h"
struct s5inode s5inode[NINODE];

#ifdef DEBUG                    /* for inode lock debugging */
#define ILOGSIZE 20
struct ilog ilogs[ILOGSIZE];
int ilogsize =ILOGSIZE;
short ipid[NINODE];
#endif

#include "sys/proc.h"
proc_t	*runq;		/* Head of linked list of running procs. */
proc_t	*curproc;	/* The currently running proc.		 */
short   curpri;         /* Priority of currently running proc.   */
struct	proc	proc[NPROC];	/* process table */
int     switching;
time_t  bootime;

#if defined(ICC_ETH) || defined(ICC_UNISON) || defined(SOCKET)
#define ICC_LANCE_HBSZ  1520    /* buffer size, from icc_lance.h   */
#define ICC_LANCE_HBNO  48      /* # of buffers for ethernet packets    */
				/* do not raise above 64                */
short   icc_lance_hbno = ICC_LANCE_HBNO;
char    hostbuf [ICC_LANCE_HBNO] [ICC_LANCE_HBSZ];      /* buffers      */
#endif

#include "sys/map.h"
struct map sptmap[SPTMAP] = {mapdata(SPTMAP)};
struct map listmap[SPTMAP] = {mapdata(SPTMAP)};
struct map pgmap[PGMAP] = {mapdata(PGMAP)};
struct map dmamap[DMAMAP] = {mapdata(DMAMAP)};
#ifndef C20
struct map dma18map[DMA18MAP] = {mapdata(DMA18MAP)};
#endif

#include "sys/callo.h"
struct callo callout[NCALL];

#include "sys/mount.h"
int	remote_acct;		/* REMOTE flag for acctp */
int	bootstate;		/* DU boot flag */
short	dufstyp;		/* DU file system switch index */

struct mount mount[NMOUNT];

# ifdef MUNET

#include "sys/munet/mnport.h"
struct mnport mnport[2*NPROC];          /* MUNIX/NET port table */

#include "sys/munet/mnnode.h"
int maxnodes = NMOUNT;
struct uinode uinode[NMOUNT];         /* MUNIX/NET node information */
char remfork[NMOUNT];                 /* true if remote receiver must fork */

#include "sys/munet/mninfo.h"
struct munetinfo m_info[NPROC];         /* MUNIX/NET infos */
short mi_ptnos[NPROC*NMOUNT];           /* all port numbers of remote child receivers */

# endif

#include "sys/munet/munet.h"
#include "sys/munet/diskless.h"
#include "sys/munet/bpdata.h"
#ifdef DLSUPPORT

#ifdef MASTERNODE
struct dltable dltable[MAXDLNACTV];     /* diskless node accounting blocks */
struct bpdata bpdata[MAXDLNODES];	/* list of supported dlnodes */
#ifdef ONMASTER
ino_t minolist[1];                      /* list of inodes on master */
#endif ONMASTER
#else  MASTERNODE
struct dltable dltable[1];              /* diskless node accounting blocks */
struct bpdata bpdata[1];                /* configuration info from master */
#ifdef ONMASTER
ino_t minolist[NMINUMS];                /* list of inodes on master */
#endif ONMASTER
#endif MASTERNODE

#else DLSUPPORT
#ifdef MASTERNODE
struct dltable dltable[1];              /* diskless node accounting blocks */
struct bpdata bpdata[1];                /* configuration info from master */
#ifdef ONMASTER
ino_t minolist[1];                      /* list of inodes on master */
#endif ONMASTER
#endif MASTERNODE
#endif DLSUPPORT

#ifdef DLSUPPORT
struct mount rootmount;
#endif DLSUPPORT
/*
#include "sys/elog.h"
#include "sys/err.h"
struct	err	err = {
	NESLOT,
};
*/

#include "sys/sysinfo.h"
struct sysinfo sysinfo;
struct syswait syswait;
struct syserr syserr;
struct	dinfo	dinfo;		/* DU perf stat's */
struct	rcinfo	rcinfo;		/* DU perf stat's */
struct minfo minfo;
struct shlbinfo shlbinfo = { SHLBMAX, 0, 0 };

#include "sys/opt.h"

#ifdef SXT
#include "sys/sxt.h"
#define LINKSIZE (sizeof(struct Link) + sizeof(struct Channel)*(MAXPCHAN-1))
char sxt_buf[NSXT*LINKSIZE];
int sxt_cnt = NSXT;
#endif

#include "sys/var.h"
struct var v = {
	NBUF,
	NCALL,
	NINODE,
	(char *)(&inode[NINODE]),
	NFILE,
	(char *)(&file[NFILE]),
	NMOUNT,
	(char *)(&mount[NMOUNT]),
	NPROC,
	(char *)(&proc[1]),
	REGIONS,
	NCLIST,
	MAXUP,
	NHBUF,
	NHBUF-1,
	NPBUF,
	VHNDFRAC,
	MAXPMEM,
	AUTOUP,
	NOFILES,
	NQUEUE,
	NSTREAM,
	NBLK4096,
	NBLK2048,
	NBLK1024,
	NBLK512,
	NBLK256,
	NBLK128,
	NBLK64,
	NBLK16,
	NBLK4,
	NINODE,
	ULIMIT,
};


#include "sys/page.h"
#include "sys/region.h"
reg_t   region[REGIONS];
reg_t	ractive;
reg_t	rfree;


#include "sys/pfdat.h"
struct pfdat	phead;		/* Head of free page list. */
struct pfdat	*pfdat;		/* Page frame database. */
struct pfdat	*phash;		/* Page hash access to pfdat. */
struct pfdat	ptfree;		/* Head of page table free list. */
int		phashmask;	/* Page hash mask. */

#include	"sys/tuneable.h"
tune_t	tune = {
		GETPGSLO,	/* t_gpgslo - get pages low limit.	*/
		GETPGSHI,	/* t_gpgshi - get pages high limit.	*/
		AGERATE,	/* t_age */
		VHANDR,		/* t_handr - vhand wakeup rate.		*/
		999999,         /* t_vhandl - vhand steal limit.        */
				/* t_vhandl initialized elsewhere       */
		MAXSC,		/* t_maxsc - max contiguous swap cnt.	*/
		PREPAGE,	/* t_prep - max swap/file pre page factor */
		FLUSHRATE,      /* t_bdflushr - flush rate of delayed buffers */
		0,              /* t_availrmem - not yet implemented */
		50,             /* t_availsmem - min. available swap space in pages */
		MAXUMEM,        /* max available user virtual space in pages */
};

#include	"sys/swap.h"
#include "sys/fs/s5macros.h"
#include "sys/rbuf.h"

/* parameters for remote (RFS) network access (which uses buffers from 
 * local buffer pool) */
struct rbuf rbfreelist;
unsigned long lbuf_ct;
unsigned long rbuf_ct;
unsigned long nrbuf;
unsigned long nlbuf;
unsigned long maxbufage = 0;
int	rcache_enable = 0;
int	rcacheinit = 0;		/* RFS client caching initialized*/
unsigned long rfs_vcode = 1;	/* version code for RFS caching */

swpt_t	swaptab[MSFILES];
int	nextswap;
int	swapwant;

#include "sys/init.h"

#ifdef MESG
#include	"sys/ipc.h"
#include	"sys/msg.h"

struct map	msgmap[MSGMAP];
struct msqid_ds	msgque[MSGMNI];
char            msglock[MSGMNI];
struct msg	msgh[MSGTQL];
struct msginfo	msginfo = {
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	MSGSEG
};
#endif

#ifdef SEMA
#	ifndef IPC_ALLOC
#	include	"sys/ipc.h"
#	endif
#include	"sys/sem.h"
struct semid_ds	sema[SEMMNI];
struct sem	sem[SEMMNS];
struct map	semmap[SEMMAP];
struct	sem_undo	*sem_undo[NPROC];
#define	SEMUSZ	(sizeof(struct sem_undo)+sizeof(struct undo)*SEMUME)
int	semu[((SEMUSZ*SEMMNU)+NBPW-1)/NBPW];
union {
	short		semvals[SEMMSL];
	struct semid_ds	ds;
	struct sembuf	semops[SEMOPM];
}	semtmp;

struct	seminfo seminfo = {
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	SEMUSZ,
	SEMVMX,
	SEMAEM
};
#endif

#ifdef SHMEM
#	ifndef	IPC_ALLOC
#	include	"sys/ipc.h"
#	endif
#include	"sys/shm.h"
struct shmid_ds	*shm_shmem[NPROC*SHMSEG];
struct shmid_ds	shmem[SHMMNI];	
/*
struct	shmpt_ds	shm_pte[NPROC*SHMSEG];
*/
struct	shminfo shminfo = {
	SHMMAX,
	SHMMIN,
	SHMMNI,
	SHMSEG,
	SHMBRK,
	SHMALL
};
#endif

/*	Each process has 3 pregions (text, data, and stack) plus
 *	enough for the maximum number of shared memory segments.
 *	We also need one extra null pregion to indicate the end
 *	of the list.
 */

int	pregpp = 3 + SHMSEG + 1;

#ifdef FLOCK
/* file and record locking */
#include <sys/fcntl.h>
#include <sys/flock.h>
struct flckinfo flckinfo = {
	FLCKREC,
	0,
	0,
	0,
} ;
struct filock flox[FLCKREC];		/* lock structures */
#endif

#ifdef RFS
#include "sys/adv.h"
#include "sys/idtab.h"
#include "sys/nserve.h"
#include "sys/cirmgr.h"
#include "sys/sema.h"
#undef FAILURE
#include "sys/comm.h"
int     minserve = MINSERVE;           /* DU tunable: sever low water mark */
int     maxserve = MAXSERVE;           /* DU tunable: sever high water mark */
int     nservers;           /* total servers in system */
int     idleserver;         /* idle servers in system */
int     msglistcnt;         /* receive descriptors in msg queue */
char    rfheap[RFHEAP];
int     rfsize = RFHEAP;
int     nadvertise = NADVERTISE;
struct  advertise advertise[NADVERTISE];
int     maxgdp = MAXGDP;
struct  gdp gdp[MAXGDP];
struct  sndd    sndd[NSNDD];
struct  rcvd    rcvd[NRCVD];
struct  rd_user rd_user[NRDUSER];
int     nrcvd = NRCVD;
int     nsndd = NSNDD;
int     nrduser = NRDUSER;
int     rfs_vhigh = RFS_VHIGH;
int     rfs_vlow = RFS_VLOW;
struct  srmnt srmount[NSRMOUNT];
int     nsrmount = NSRMOUNT;
unsigned long nremote = NREMOTE;
unsigned long nlocal = NLOCAL;
struct  rhbuf rhbuf[NHBUF];
int     nrhbuf = NHBUF;
int     rhmask = NHBUF-1;
int     rc_time = RCACHETIME;
#endif

#ifdef STREAM
#include "sys/stream.h"
char strlofrac = STRLOFRAC;          /* low priority cutoff percentage */
char strmedfrac = STRMEDFRAC;         /* medium priority cutoff percentage */
struct stdata streams[NSTREAM];               /* table of streams */
queue_t queue[NQUEUE];                /* table of queues */
#define NSBLK (NBLK4096+NBLK2048+NBLK1024+NBLK512+NBLK256+NBLK128+NBLK64+NBLK16+NBLK4)
mblk_t  mblock[NSBLK + NSBLK/4];         /* table of msg blk desc */
dblk_t  dblock[NSBLK];                   /* table of data blk desc */
struct linkblk linkblk[NMUXLINK];       /* multiplexor link table */
struct strevent strevent[NSTREVENT];     /* table of stream event cells */
int strmsgsz = STRMSGSZ;                   /* maximum stream message size */
int strctlsz = STRCTLSZ;                   /* maximum size of ctl part of message */
int nmblock = NSBLK + NSBLK/4;             /* number of msg blk desc */
int nmuxlink = NMUXLINK;                   /* number of multiplexor links */
int nstrpush = NSTRPUSH;                   /* maxmimum number of pushes allowed */
int nstrevent = NSTREVENT;                  /* initial number of stream event cells */
int maxsepgcnt = MAXSEPGCNT;                 /* page limit for event cell allocation */

struct sp {
	queue_t *sp_rdq;		/* this stream's read queue */
	queue_t *sp_ordq;		/* other stream's read queue */
		} sp_sp[SPCNT];
int spcnt = SPCNT;

struct tim_tim {
	long 	 tim_flags;
	queue_t	*tim_rdq;
	mblk_t  *tim_iocsave;
} tim_tim[TIMCNT];
int tim_cnt = TIMCNT;

struct trw_trw {
	long 	 trw_flags;
	queue_t	*trw_rdq;
	mblk_t  *trw_mp;
} trw_trw[TRWCNT];
int trw_cnt = TRWCNT;

#include "sys/log.h"
struct log log_log[NLOG];         /* sad device state table */
int log_cnt = NLOG;                     /* number of configured minor devices */
int log_bsz = LOGBSIZE;                     /* size of internal buffer of log messages */

#endif  STREAM


int	maxpmem = MAXPMEM;	/* Max physical memory to use.  Used for */
				/* system performance measurement purposes */

/* Cadmus specific */
#ifdef DIALOG
/* Buffer for aunix */
char tbf [0xff00];
#endif

#ifdef SOCKET
/* 1=turn on IP checksums */
unchar ipcksum = 1;

/* 1=turn on TCP checksums */
int tcpcksum = 1;

/* 1=turn on UDP checksums */
int udpcksum = 1;

/* 1=send ICMP redirects */
int ipsendredirects = 1;

/* 1=do IP forwarding if > 1 interface */
int ipforwarding = 1;

/* treat hosts on subnetworks other than our own, as if they are on our
 *	local net.  (see in_localaddr() in netinet/in.c) */
int subnetsarelocal = 1;

/* use loopback interface for local traffic */
int useloopback = 1;
#endif
