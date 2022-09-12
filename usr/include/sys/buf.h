/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)kern-port:sys/buf.h	10.2"*/

/*
 *	Each buffer in the pool is usually doubly linked into 2 lists:
 *	the device with which it is currently associated (always)
 *	and also on a list of blocks available for allocation
 *	for other use (usually).
 *	The latter list is kept in last-used order, and the two
 *	lists are doubly linked to make it easy to remove
 *	a buffer from one list when it was found by
 *	looking through the other.
 *	A buffer is on the available list, and is liable
 *	to be reassigned to another disk block, if and only
 *	if it is not marked BUSY.  When a buffer is busy, the
 *	available-list pointers can be used for other purposes.
 *	Most drivers use the forward ptr as a link in their I/O active queue.
 *	A buffer header contains all the information required to perform I/O.
 *	Most of the routines which manipulate these things are in bio.c.
 *
 *      WARNING:  do not change size of the buf structure (sys/buf.h) without 
 *	making a corresponding size change in the rbuf structure (sys/rbuf.h), 
 *  	and vice versa).  The two structures define the two possible formats 
 *	of the buffer header (the format used depends on whether the buffer 
 *	contains local data or RFS data).  
 *
 *      This overlay of the two structures is an interim solution
 *	that is expected to change in a future release.  Users
 *	should be aware that the rbuf structure will probably go
 *	away at that time.
 */
typedef struct	buf
{
	int	b_flags;		/* see defines below */
	struct	buf *b_forw;		/* headed by d_tab of conf.c */
	struct	buf *b_back;		/*  "  */
	struct	buf *av_forw;		/* position on free list, */
	struct	buf *av_back;		/*     if not BUSY*/
	dev_t	b_dev;			/* major+minor device name */
	paddr_t b_paddr;                /* dma address of block */
#define paddr(X)        (paddr_t)(X->b_paddr)
	unsigned b_bcount;		/* transfer count */
	union {
	    caddr_t b_addr;		/* low order core address */
	    int	*b_words;		/* words for clearing */
	    daddr_t *b_daddr;		/* disk blocks */
	} b_un;
	daddr_t	b_blkno;		/* block # on device */
	char	b_error;		/* returned after I/O */
	unsigned int b_resid;		/* words not transferred after error */
	time_t	b_start;		/* request start time */
	int     b_dummy;
	struct  proc  *b_proc;		/* process doing physical or swap I/O */
	long    b_dummy_rfs[2];         /* so sizeof(struct buf) == sizeof(struct rbuf) */
	unsigned long	b_reltime;      /* previous release time */
} buf_t;

extern	struct	buf	buf[];		/* The buffer pool itself */
extern	struct	buf	bfreelist;	/* head of available list */
struct	pfree	{
	int	b_flags;
	struct	buf	*av_forw;
};
extern	struct	pfree	pfreelist;
extern	int		pfreecnt;
extern	struct	buf	pbuf[];
extern	char		*buffers[];


/*
 *	These flags are kept in b_flags.
 */
#define B_WRITE   0x0000	/* non-read pseudo-flag */
#define B_READ    0x0001	/* read when I/O occurs */
#define B_DONE    0x0002	/* transaction finished */
#define B_ERROR   0x0004	/* transaction aborted */
#define B_BUSY    0x0008	/* not on av_forw/back list */
#define B_PHYS    0x0010	/* Physical IO potentially using UNIBUS map */
#define B_MAP     0x0020	/* This block has the UNIBUS map allocated */
#define B_WANTED  0x0040	/* issue wakeup when BUSY goes off */
#define B_AGE     0x0080	/* delayed write for correct aging */
#define B_ASYNC   0x0100	/* don't wait for I/O completion */
#define B_DELWRI  0x0200	/* delayed write - wait until buffer needed */
#define B_OPEN    0x0400	/* open routine called */
#define B_STALE   0x0800
/* 3B stuff
#define B_VERIFY  0x1000
#define B_FORMAT  0x2000
#define B_RAMRD   0x4000
#define B_RAMWT	  0x8000
*/
#define B_PAGEIO  0x1000  /* IO operation is for a whole page from/to swap */
#define B_PT      0x2000  /* mark page (pfdat) done when I/O
				   completes (physical I/O) */
#define B_DMA22   0x4000  /* DMA-Controller has 22bit */
#define B_SWAP    0x8000  /* Buffer is for swap operation for diskless node */
#define B_REMOTE  0x10000       /* buffer contains remote (RFS) data */

/*
 *	Fast access to buffers in cache by hashing.
 */

#define bhash(d,b)      ((struct buf *)&hbuf[((int)(d)+(int)(b))&v.v_hmask])

struct	hbuf
{
	int	b_flags;
	struct	buf	*b_forw;
	struct	buf	*b_back;
	int	b_pad;			/* round size to 2^n */
};

extern	struct	hbuf	hbuf[];

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized code
 */
#define geterror(bp) \
{\
\
	if ((bp)->b_flags&B_ERROR)\
		if ((u.u_error = (bp)->b_error)==0)\
			u.u_error = EIO;\
}

/*
 * Unlink a buffer from the available list and mark it busy.
 * (internal interface)
 */
#define notavail(bp) \
{\
	register s;\
\
	s = spl6();\
ASSERT (bp->av_back->av_forw == bp);\
ASSERT (bp->av_forw->av_back == bp);\
	bp->av_back->av_forw = bp->av_forw;\
	bp->av_forw->av_back = bp->av_back;\
	bp->b_flags |= B_BUSY;\
	bfreelist.b_bcount--;\
	splx(s);\
}
/* Cadmus for old drivers, to be replaced by iobuf.h */
/*
 * special redeclarations for
 * the head of the queue per
 * device driver.
 */
#define	b_actf	av_forw
#define	b_actl	av_back
#define	b_active b_bcount
#define	b_errcnt b_resid
/* For diskless node queue head only */
#define b_dlindex  b_error
#define b_seqno b_dummy

/*
 * special redeclarations for
 * swap alloc and free from
 * diskless nodes
 */
#define b_swret   b_dev
#define b_swsize  b_bcount
#define b_swstart b_resid
#define b_swfunc  b_dummy
#define b_swdev   b_resid
