/* @(#)buf.h	6.2 */
/*
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * the device with which it is currently associated (always)
 * and also on a list of blocks available for allocation
 * for other use (usually).
 * A buffer is on the available list, and is liable
 * to be reassigned to another disk block, if and only
 * if it is not marked BUSY.  When a buffer is busy, the
 * available-list pointers can be used for other purposes.
 * Most drivers use the forward ptr as a link in their I/O active queue.
 * A buffer header contains all the information required to perform I/O.
 * Most of the routines which manipulate these things are in bio.c.
 */
struct buf
{
	int	b_flags;		/* see defines below */
	struct	buf *b_forw;		/* headed by d_tab of conf.c */
	struct	buf *b_back;		/*  "  */
	struct	buf *av_forw;		/* position on free list, */
	struct	buf *av_back;		/*     if not BUSY*/
	dev_t	b_dev;			/* major+minor device name */
	int     b_dmd;                  /* dma map descriptor */
	paddr_t b_paddr;                /* dma address of block */
#define paddr(X)        (paddr_t)(X->b_paddr)
	unsigned b_bcount;		/* transfer count */
	union {
	    caddr_t b_addr;             /* buffer virtual address */
	    int	*b_words;		/* words for clearing */
	    struct filsys *b_filsys;	/* superblocks */
	    struct dinode *b_dino;	/* ilist */
	    daddr_t *b_daddr;		/* indirect block */
	} b_un;
	daddr_t	b_blkno;		/* block # on device */
	char	b_error;		/* returned after I/O */
	unsigned int b_resid;		/* words not transferred after error */
	int     b_dummy;
	time_t	b_start;		/* request start time */
	struct  proc  *b_proc;		/* process doing physical or swap I/O */
};

extern struct buf bfreelist;		/* head of available list */
extern struct buf pbuf[];		/* Physio header pool */
struct pfree {
	int	b_flags;
	struct	buf *av_forw;
};
extern struct pfree pfreelist;		/* head of physio pool */



/*
 * These flags are kept in b_flags.
 */
#define	B_WRITE	0	/* non-read pseudo-flag */
#define	B_READ	01	/* read when I/O occurs */
#define	B_DONE	02	/* transaction finished */
#define	B_ERROR	04	/* transaction aborted */
#define	B_BUSY	010	/* not on av_forw/back list */
#define	B_PHYS	020	/* Physical IO potentially using UNIBUS map */
#define	B_MAP	040	/* This block has the UNIBUS map allocated */
#define	B_WANTED 0100	/* issue wakeup when BUSY goes off */
#define	B_AGE	0200	/* delayed write for correct aging */
#define	B_ASYNC	0400	/* don't wait for I/O completion */
#define	B_DELWRI 01000	/* don't write till block leaves available list */
#define	B_OPEN	02000	/* open routine called */
#define	B_STALE 04000
#define B_CTRL  010000  /* used in rx2.c */
#define B_PAGEIO 010000 /* IO operation is for a whole page from/to swap */
#define B_PT    020000  /* mark page (pfdat) done when I/O
				   completes (physical I/O) */
#define B_DMA22 040000  /* DMA-Controller has 22bit */
#define B_SWAP 0100000  /* Buffer is for swap operation for diskless node */


/*
 * Fast access to buffers in cache by hashing.
 */

#define	bhash(d,b)	((struct buf *)&hbuf[((int)d+(int)b)&v.v_hmask])

struct hbuf
{
	int	b_flags;
	struct	buf *b_forw;
	struct	buf *b_back;
};

extern struct hbuf hbuf[];

#define	geterror(bp)	\
{\
	if (bp->b_flags&B_ERROR)\
		if ((u.u_error = bp->b_error)==0)\
			u.u_error = EIO;\
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

