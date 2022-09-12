/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mbuf.h	7.2 (Berkeley) 9/11/86
 */

/*
 * Constants related to memory allocator.
 */
#define	MSIZE		128			/* size of an mbuf */

#define	MMINOFF		12			/* mbuf header length */
#define	MTAIL		4
#define	MMAXOFF		(MSIZE-MTAIL)		/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF-MTAIL)	/* mbuf data length */
#define NMBCLUSTERS	256			/* max # of clusters */
#define CLBYTES NBPP                            /* 1 Cluster = 1 Page = 32 mbufs */
#define	NMBPCL		(CLBYTES/MSIZE)		/* # mbufs per cluster */

/*
 * Macros for type conversion
 */

/* address in mbuf to mbuf head */
#define	dtom(x)		((struct mbuf *)((int)x & ~(MSIZE-1)))

/* mbuf head, to typed data */
#define	mtod(x,t)	((t)((int)(x) + (x)->m_off))

struct mbuf {
	struct	mbuf *m_next;		/* next buffer in chain */
	u_long  m_off;                  /* offset of data */
	short	m_len;			/* amount of data in this mbuf */
	short	m_type;			/* mbuf type (0 == free) */
	union {
		/*
		 * For sgi systems, clusters can be special.  Special
		 * clusters have their underlying memory somewhere else.
		 * The freefunc is used with the given arguments when the
		 * buffer is done being used.
		 */
		struct {
			int	(*mu_freefunc)();
			long	mu_farg;
			int	(*mu_dupfunc)();
			long	mu_darg;
		} m_us;
		u_char	mu_dat[MLEN];	/* data storage */
	} m_u;
	struct	mbuf *m_act;		/* link in higher-level mbuf list */
};
#define	m_dat		m_u.m_us.mu_dat		/* compatability */
#define	m_freefunc	m_u.m_us.mu_freefunc	/* lazy */
#define	m_farg		m_u.m_us.mu_farg
#define	m_dupfunc	m_u.m_us.mu_dupfunc
#define	m_darg		m_u.m_us.mu_darg

/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_ZOMBIE	9	/* zombie proc status */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */
#define	MT_RIGHTS	12	/* access rights */
#define	MT_IFADDR	13	/* interface address */
#define MT_MAX		14	/* 'largest' type */

/* flags to m_get */
#define	M_DONTWAIT	0
#define	M_WAIT		1

/* flags to m_pgalloc */
#define	MPG_MBUFS	0		/* put new mbufs on free list */
#define	MPG_CLUSTERS	1		/* put new clusters on free list */
#define	MPG_SPACE	2		/* don't free; caller wants space */

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

/*
 * m_pullup will pull up additional length if convenient;
 * should be enough to hold headers of second-level and higher protocols. 
 */
#define	MPULL_EXTRA	32

#define	MFREE(m, n)	(n) = m_free(m)
#define	MGET(m, i, t)	(m) = m_get(i, t)

/*
 * Mbuf statistics.
 */
struct mbstat {
	short	m_mbufs;	/* mbufs obtained from page pool */
	short	m_clusters;	/* clusters obtained from page pool */
	short	m_clfree;	/* free clusters */
	short	m_drops;	/* times failed to find space */
	short	m_mtypes[MT_MAX];	/* type specific mbuf allocations */
};

#ifdef	KERNEL
struct	mbstat mbstat;
struct	mbuf *mbufree;		/* work around the other 'mfree' in mount */
int	m_want;
char	mclrefcnt[NMBCLUSTERS + 1];

extern	struct mbuf	*m_get();	/* alloc an mbuf */
extern	struct mbuf	*m_getclr();	/* alloc a cluster */
extern	struct mbuf	*m_free();	/* free an mbuf/cluster */
extern	struct mbuf	*m_copy();	/* copy an mbuf */
extern	struct mbuf	*m_pullup();	/* condense an mbuf */
extern	int		m_clfree();	/* free a "normal" cluster */
extern	int		m_cldup();	/* share a "normal" cluster */
extern	struct mbuf	*m_clget();	/* get a cluster */
extern	caddr_t		m_pgget();	/* allocate a page */
#endif
