/* @(#)space.h  6.13 */

#define	KERNEL
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
struct	buf	pbuf[NPBUF];	/* Physical io header pool */

struct	hbuf	hbuf[NHBUF];	/* buffer hash table */

#include "sys/file.h"
struct	file	file[NFILE];	/* file table */

#include "sys/inode.h"
struct	inode	inode[NINODE];	/* inode table */
/***** CADMUS: John Bass record locking *****/
struct	locklist locklist[NFLOCKS]; /* lockf table */

#include "sys/proc.h"
struct	proc	proc[NPROC];	/* process table */

#if defined(ICC_ETH) || defined(ICC_UNISON)
#define ICC_LANCE_HBSZ  1520    /* buffer size, from icc_lance.h   */
#define ICC_LANCE_HBNO  48      /* # of buffers for ethernet packets    */
				/* do not raise above 64 (HOSTBUFNO)    */
short   icc_lance_hbno = ICC_LANCE_HBNO;
char    hostbuf [ICC_LANCE_HBNO] [ICC_LANCE_HBSZ];      /* buffers      */
#endif

# ifdef MUNET

#include "sys/munet/mnport.h"
struct mnport mnport[2*NPROC];          /* MUNIX/NET port table */

#include "sys/munet/mninfo.h"
struct munetinfo m_info[NPROC];         /* MUNIX/NET infos */

#include "sys/munet/mnnode.h"
struct uinode uinode[UIMAXNODES];       /* MUNIX/NET node information */

# endif

#include "sys/munet/munet.h"
#include "sys/munet/diskless.h"
#include "sys/munet/bpdata.h"
#ifdef DLSUPPORT

#ifdef MASTERNODE
struct dltable dltable[MAXDLNACTV];     /* diskless node accounting blocks */
struct bpdata bpdata[MAXDLNODES];	/* list of supported dlnodes */
ino_t minolist[1];                      /* list of inodes on master */
#else  MASTERNODE
struct dltable dltable[1];              /* diskless node accounting blocks */
struct bpdata bpdata[1];                /* configuration info from master */
ino_t minolist[NMINUMS];                /* list of inodes on master */
#endif MASTERNODE

#else DLSUPPORT
#ifdef MASTERNODE
struct dltable dltable[1];              /* diskless node accounting blocks */
struct bpdata bpdata[1];                /* configuration info from master */
ino_t minolist[1];                      /* list of inodes on master */
#endif MASTERNODE
#endif DLSUPPORT

#include "sys/map.h"
struct map sptmap[SPTMAP] = {mapdata(SPTMAP)};
struct map listmap[SPTMAP] = {mapdata(SPTMAP)};
struct map pgmap[PGMAP] = {mapdata(PGMAP)};
struct map dmamap[DMAMAP] = {mapdata(DMAMAP)};
struct map dma18map[DMA18MAP] = {mapdata(DMA18MAP)};

#include "sys/callo.h"
struct callo callout[NCALL];

#include "sys/mount.h"
struct mount mount[NMOUNT];

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

#include "sys/opt.h"

#include "sys/sxt.h"
#define LINKSIZE (sizeof(struct Link) + sizeof(struct Channel)*(MAXPCHAN-1))
char sxt_buf[NSXT*LINKSIZE];
int sxt_cnt = NSXT;

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
	NFLOCKS,
	AUTOUP,
	NQUEUE,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
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
};

#include	"sys/swap.h"
swpt_t	swaptab[MSFILES];
int	nextswap;
int	swapwant;

#include "sys/init.h"

#if MESG==1
#include	"sys/ipc.h"
#include	"sys/msg.h"

struct map	msgmap[MSGMAP];
struct msqid_ds	msgque[MSGMNI];
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

#if SEMA==1
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

#if SHMEM==1
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

struct minfo minfo;

/*	Each process has 3 pregions (text, data, and stack) plus
 *	enough for the maximum number of shared memory segments.
 *	We also need one extra null pregion to indicate the end
 *	of the list.
 */

int	pregpp = 3 + SHMSEG + 1;

#ifdef FLOCK
/* file and record locking */
#include <sys/flock.h>
struct flckinfo flckinfo = {
	FLCKREC,
	FLCKFIL,
	0,
	0,
	0,
	0
} ;

struct filock flox[FLCKREC];		/* lock structures */
struct flino flinotab[FLCKFIL];	/* inode to lock assoc. structures */
#endif

int	maxpmem = MAXPMEM;	/* Max physical memory to use.  Used for */
				/* system performance measurement purposes */

/* Cadmus specific */
/*
 * This table tracks the active file servers for a port.
 */
unsigned short	fs_pid[NPROC];	/* file server pid */
unsigned short 	fs_dev[NPROC];	/* dev that this fs supports */

	
#ifdef DIALOG
/* Buffer for aunix                                                     */
char tbf [0xff00];
#endif
