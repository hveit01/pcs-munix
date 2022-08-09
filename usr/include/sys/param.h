#define PCS 1
#define M32 1
#define MUNET 1                 /* inserts code for MUNIX/NET */
#define DISKLESS 1              /* diskless node code         */
#define SELECT 1                /* code for SELECT            */
/* @(#)param.h	6.4 */

/*
 * fundamental variables
 * don't change too often
 */

#define NOFILE  50              /* max open files per process */
#define	MAXPID	30000		/* max process id */
#define	MAXUID	60000		/* max user id */
#define	MAXLINK	1000		/* max links */

#define MAXBLK  15              /* max pages possible for phys IO */
#define SSIZE   1               /* initial stack size (*4096 bytes) */
#define SINCR   1               /* increment of stack (*4096 bytes) */
#define USIZE   1               /* size of user block (*4096 bytes) */
#define USRSTACK (0x40000000-0x800000-ptob(USIZE)) /* Start of user stack */

#define	CANBSIZ	256		/* max size of typewriter line	*/
#define HZ      hz              /* Ticks/second of the clock */
#define NCARGS  10240           /* # characters in exec arglist */

#define SCHMAX  127

/*
 * priorities
 * should not be altered too much
 */

#define PMASK   0377
#define	PCATCH	0400
#define	PSWP	0
#define PMEM	0
#define	PINOD	10
#define	PRIBIO	20
#define	PZERO	25
#define NZERO   20      /* basic nice value             */
#define NINTER  29      /* max interactive nice value   */
#define	PPIPE	26
#define	PWAIT	30
#define	PSLEP	39
#define	PUSER	60
#define PINTER  127
#define PIDLE   255

#define PINTERSEC 5 /* an interactive Process should react within 5 seconds */

/*
 * fundamental constants of the implementation--
 * cannot be changed easily
 */

#define	NBPW	sizeof(int)	/* number of bytes in an integer */


#define NPPS    256             /* number of pages per segment */
#define NBPS    0x100000        /* Number of bytes per segment */
#define NBPP    4096            /* Number of bytes per page */
#define NBPC    1024            /* Number of bytes per click */
#define NDPP    8               /* number of disk blocks per page */

#define DPPSHFT         3       /* LOG2(NDPP) if exact */
#define BPPSHIFT        12      /* LOG2(NBPP) if exact */
#define BPCSHIFT        10      /* LOG2(NBPC) if exact */
#define SOFFMASK        0xFFFFF /* mask for offset into segment */
#define POFFMASK        0xFFF   /* Mask for offset into page. */
#define COFFMASK        0x3FF   /* Mask for offset into click. */

#define NULL    (char *)0

#define MAXUMEM 4096            /* max pages per proc (i.e. 16MB)(to be changed later,UH) */
#define MAXSUSE 255             /* maximum share count on swap */

#define CMASK   2               /* default mask for file creation */
#define CDLIMIT 8388607         /* default max write address, value can not be larger */
#define	NODEV	(dev_t)(-1)
#define	ROOTINO	((ino_t)2)	/* i number of all roots */
#define	SUPERBOFF	512	/* byte offset of the super block */
#define	DIRSIZ	14		/* max characters per directory */
#define	NICINOD	100		/* number of superblock inodes */
#define	NICFREE	50		/* number of superblock free blocks */
#ifndef	FsTYPE
#define	FsTYPE	3
#endif

#if FsTYPE==1
	/* Original 512 byte file system */
#define	BSIZE	512		/* size of file system block (bytes) */
#define	SBUFSIZE	BSIZE	/* system buffer size */
#define	BSHIFT	9		/* LOG2(BSIZE) */
#define	NINDIR	(BSIZE/sizeof(daddr_t))
#define	BMASK	0777		/* BSIZE-1 */
#define	INOPB	8		/* inodes per block */
#define	INOSHIFT	3	/* LOG2(INOPB) if exact */
#define	NMASK	0177		/* NINDIR-1 */
#define	NSHIFT	7		/* LOG2(NINDIR) */
#define	FsBSIZE(dev)	BSIZE
#define	FsBSHIFT(dev)	BSHIFT
#define	FsNINDIR(dev)	NINDIR
#define	FsBMASK(dev)	BMASK
#define	FsINOPB(dev)	INOPB
#define	FsLTOP(dev, b)	b
#define	FsPTOL(dev, b)	b
#define	FsNMASK(dev)	NMASK
#define	FsNSHIFT(dev)	NSHIFT
#define	FsITOD(dev, x)	itod(x)
#define	FsITOO(dev, x)	itoo(x)
#endif

#if FsTYPE==2
	/* New 1024 byte file system */
#define	BSIZE	1024		/* size of file system block (bytes) */
#define	SBUFSIZE	BSIZE	/* system buffer size */
#define	BSHIFT	10		/* LOG2(BSIZE) */
#define	NINDIR	(BSIZE/sizeof(daddr_t))
#define	BMASK	01777		/* BSIZE-1 */
#define	INOPB	16		/* inodes per block */
#define	INOSHIFT	4	/* LOG2(INOPB) if exact */
#define	NMASK	0377		/* NINDIR-1 */
#define	NSHIFT	8		/* LOG2(NINDIR) */
#define	FsBSIZE(dev)	BSIZE
#define	FsBSHIFT(dev)	BSHIFT
#define	FsNINDIR(dev)	NINDIR
#define	FsBMASK(dev)	BMASK
#define	FsINOPB(dev)	INOPB
#define	FsLTOP(dev, b)	(b<<1)
#define	FsPTOL(dev, b)	(b>>1)
#define	FsNMASK(dev)	NMASK
#define	FsNSHIFT(dev)	NSHIFT
#define	FsITOD(dev, x)	itod(x)
#define	FsITOO(dev, x)	itoo(x)
#endif

#if FsTYPE==3
	/* Dual system */
#define	BSIZE	512		/* size of file system block (bytes) */
#define	SBUFSIZE	(BSIZE*2)	/* system buffer size */
#define	BSHIFT	9		/* LOG2(BSIZE) */
#define	NINDIR	(BSIZE/sizeof(daddr_t))
#define	BMASK	0777		/* BSIZE-1 */
#define	INOPB	8		/* inodes per block */
#define	INOSHIFT	3	/* LOG2(INOPB) if exact */
#define	NMASK	0177		/* NINDIR-1 */
#define	NSHIFT	7		/* LOG2(NINDIR) */
#define Fs2BLK  0x4000
#define	FsLRG(dev)	(dev&Fs2BLK)
#define	FsBSIZE(dev)	(FsLRG(dev) ? (BSIZE*2) : BSIZE)
#define	FsBSHIFT(dev)	(FsLRG(dev) ? 10 : 9)
#define	FsNINDIR(dev)	(FsLRG(dev) ? 256 : 128)
#define	FsBMASK(dev)	(FsLRG(dev) ? 01777 : 0777)
#define	FsBOFF(dev, x)	(FsLRG(dev) ? ((x)&01777) : ((x)&0777))
#define	FsBNO(dev, x)	(FsLRG(dev) ? ((x)>>10) : ((x)>>9))
#define	FsINOPB(dev)	(FsLRG(dev) ? 16 : 8)
#define	FsLTOP(dev, b)	(FsLRG(dev) ? b<<1 : b)
#define	FsPTOL(dev, b)	(FsLRG(dev) ? b>>1 : b)
#define	FsNMASK(dev)	(FsLRG(dev) ? 0377 : 0177)
#define	FsNSHIFT(dev)	(FsLRG(dev) ? 8 : 7)
#define	FsITOD(dev, x)	(daddr_t)(FsLRG(dev) ? \
	((unsigned)x+(2*16-1))>>4 : ((unsigned)x+(2*8-1))>>3)
#define	FsITOO(dev, x)	(daddr_t)(FsLRG(dev) ? \
	((unsigned)x+(2*16-1))&017 : ((unsigned)x+(2*8-1))&07)
#define	FsINOS(dev, x)	(FsLRG(dev) ? \
	((x&~017)+1) : ((x&~07)+1))
#endif

#define USERMODE(ps)  ((ps & PS_S)==0)
#define	BASEPRI(ps)	((ps & PS_IPL) != 0)

#define lobyte(X)       (((unsigned char *)&X)[1])
#define hibyte(X)       (((unsigned char *)&X)[0])
#define loword(X)       (((ushort *)&X)[1])
#define hiword(X)       (((ushort *)&X)[0])


/* Cadmus Stuff */
#define UIMAXNODES 32           /* max network nodes for MUNIX/NET */
#define MAXPORTS 50             /* max network ports for Newcastle */
#define MSGBUFS 1024 /* was 256; changed for error logging (KS)         */

/* symbolic links */
#define NSYMBUF           8
#define MAXPATHLEN      256

