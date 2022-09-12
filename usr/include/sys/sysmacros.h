/* @(#)sysmacros.h      1.2 */
/*
 * Some macros for units conversion
 */
/* Pages to segments and vice versa */
#define ptos(X)   (((X)+(NPPS-1))/NPPS)   /* pages to segments */
#define	ptots(X)  ((X)/NPPS)		/* pages to truncated segments */
#define stopg(x) ((long)(x)*NPPS)               /* segments to pages */
#define stob(x) ((long)(x) << 20)               /* segments to bytes */
#define btos(x) (((ulong)(x)+(NBPS-1)) >> 20)   /* bytes to segments */
#define btots(x) ((ulong)(x) >> 20)             /* bytes to truncated segments */

/* Pages to clicks */   /* 4 clicks = 1 page */
#define ctop(X) (((X) + 3) >> 2)
#define ctotp(X) ((X) >> 2)
#define ptoc(X) ((X) << 2)
#define ctod(X) (X)

/* Pages to disk blocks */
#define dtop(X)	(((X) + (NDPP-1)) >> DPPSHFT)
#define ptod(X) ((long)(X) << DPPSHFT)

/* bytes to pages, pages to bytes */
#define ptob(x) ((long)(x)<<BPPSHIFT)
#define btop(x) (((long)(x)+(NBPP-1))>>BPPSHIFT)
#define btotp(x)        ((long)(x)>>BPPSHIFT)

/* bytes to clicks, clicks to bytes */
#define ctob(x) ((long)(x)<<BPCSHIFT)
#define btoc(x) (((long)(x)+(NBPC-1))>>BPCSHIFT)
#define btotc(x)        ((long)(x)>>BPCSHIFT)

/* bytes to longs */
#define btol(x) (((long)(x)+3)>>2)

/* inumber to disk address */
#ifdef INOSHIFT
#define itod(x) (daddr_t)(((unsigned)(x)+(2*INOPB-1))>>INOSHIFT)
#else
#define itod(x) (daddr_t)(((unsigned)(x)+(2*INOPB-1))/INOPB)
#endif

/* inumber to disk offset */
#ifdef INOSHIFT
#define itoo(x) (int)(((unsigned)(x)+(2*INOPB-1))&(INOPB-1))
#else
#define itoo(x) (int)(((unsigned)(x)+(2*INOPB-1))%INOPB)
#endif

/* major part of a device */
#define major(x)        (int)((unsigned)(x)>>8)
/* Adjust the bmajor macro for anding of ( (Fs2BLK >> 8) - 1 ) */
#define bmajor(x)       (int)(((unsigned)(x)>>8)&0x3f)
#define brdev(x)        ((x)&0x3fff)

/* minor part of a device */
#define minor(x)        (int)((x)&0377)
#define fsys(x)		((x) & 0xF)
#define physical(x)	(((x) >> 4) & 0x3)

/* make a device number */
#define	makedev(x,y)	(dev_t)(((x)<<8) | (y))

/*
 * misc macros
 */

#define critical(ps)    (((ps) & PS_IPL) != 0)
#define decay(a,b,c) a=((int)((a)*((c)-1)+(b)))/(c);
#define poff(X)   ((ulong)(X) & POFFMASK)       /* page offset */
#define soff(X)   ((ulong)(X) & SOFFMASK)       /* segment offset */
#ifndef C20
#define snum(X) (((ulong)(X)>>20) & 0x3ff)      /* segment number */
#define mkpte(bits,pno) ((bits)|(pno))
#else
#define snum(X) ((ulong)(X)>>20)                /* segment number */
#define mkpte(bits,pno) ((bits)|((pno)<<12))
#endif

/*
 * Check page bounds
 */
#define pfdrnge(pfd, s) if (((pfd)-pfdat) < firstfree || ((pfd)-pfdat) >= maxfree){ \
	printf("bad range %d %d %d\n", (pfd) - pfdat, firstfree, maxfree);\
	panic(s);}
/*
 * Calculate user priority (Performance)
 */
#define PMAX    255
#define calcppri(p) { \
		register int a; \
 \
		a = ((p)->p_cpu /**>> 1**/ ) + \
			((p)->p_inter ? 0 : PINTER) + \
			(p)->p_nice + (PUSER-NZERO); \
		if (a > PMAX) a = PMAX; \
		(p)->p_pri = a; \
		curpri = (p)->p_pri; \
	}
#define resetpri(p, o)	calcppri(p)

/*
 * Increment page use count
 */
#define	MAXMUSE		32000	/* Maximum share count on a page	*/
#define pfdinc(pfd, s)          if ((pfd)->pf_use++ == MAXMUSE) \
				panic(s)

/* Cadmus specific */
#define atox(x)  (int)(((long)(x)>>16)&3)         /* bits 16 and 17 */
#define a2der(x)  (int)((long)(x)>>18)            /* bits 18 to 21 */

/* MUNIX/NET specific */
#define GET_IPADDR(x)   (((x)[0] << 8) + (((x)[1] >> 16) & 0xff))

/*
 *  Evaluate to true if the process is a server - Distributed UNIX
 */
#define	server()	(u.u_procp->p_sysid != 0)

/*
 * Defined for RFS client caching
 */

extern int rcacheinit;			/* RFS client caching flag */
extern unsigned long rfs_vcode;		/* version code for RFS caching */

#define CLOSEI(ip, flag, count, offset) \
{ \
	if (rcacheinit && (ip)->i_ftype == IFREG && (count) == 1 \
		&& !server() && (flag) & FWRITE) { \
		(ip)->i_wcnt--; \
		if ((ip)->i_flag & IWROTE && (ip)->i_wcnt == 0) { \
			(ip)->i_flag &= ~IWROTE; \
			if ((ip)->i_rcvd) \
				enable_cache((ip)->i_rcvd); \
		} \
	} \
	FS_CLOSEI(ip, flag, count, offset); \
}

#define OPENI(ip, mode) \
{ \
	if (rcacheinit && (ip)->i_ftype == IFREG && !server() && (mode) & FWRITE) \
		(ip)->i_wcnt++; \
	FS_OPENI(ip, mode); \
}

#define WRITEI(ip) \
{ \
	if (rcacheinit && (ip)->i_ftype == IFREG) { \
		if (!server()) \
			(ip)->i_flag |= IWROTE; \
		(ip)->i_vcode = ++rfs_vcode; \
		if ((ip)->i_rcvd) \
			disable_cache(ip); \
	} \
	FS_WRITEI(ip); \
}

#define ITRUNC(ip) \
{ \
	if (rcacheinit && (ip)->i_ftype == IFREG) { \
		if (!server()) \
			(ip)->i_flag |= IWROTE; \
		(ip)->i_vcode = ++rfs_vcode; \
		if ((ip)->i_rcvd) \
			disable_cache(ip); \
	} \
	FS_ITRUNC(ip); \
}

#define IPUT(ip) \
{ \
	if (rcacheinit && (ip)->i_nlink <= 0 && (ip)->i_count == 1) { \
		(ip)->i_vcode = ++rfs_vcode; \
	} \
	FS_IPUT(ip); \
}

#define FREESP(ip, arg, flag, offset) \
{ \
	if (rcacheinit && (ip)->i_ftype == IFREG) { \
		if (!server()) \
			(ip)->i_flag |= IWROTE; \
		(ip)->i_vcode = ++rfs_vcode; \
		if ((ip)->i_rcvd) \
			disable_cache(ip); \
	} \
	FS_FCNTL(ip, F_FREESP, arg, flag, offset); \
}
