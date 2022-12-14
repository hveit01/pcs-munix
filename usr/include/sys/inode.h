/* @(#)inode.h	6.3 */

/*
 *	The I node is the focus of all
 *	file activity in unix. There is a unique
 *	inode allocated for each active file,
 *	each current directory, each mounted-on
 *	file, text file, and the root. An inode is 'named'
 *	by its dev/inumber pair. (iget/iget.c)
 *	Data, from mode on, is read in
 *	from permanent inode on volume.
 */

#define	NADDR	13
#define	NSADDR	(NADDR*sizeof(daddr_t)/sizeof(short))

struct	inode
{
	struct	inode	*i_forw;	/* inode hash chain */
	struct	inode	*i_back;	/* '' */
	struct	inode	*av_forw;	/* freelist chain */
	struct	inode	*av_back;	/* '' */
	short	i_flag;
	cnt_t	i_count;	/* reference count */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	ushort	i_mode;
	short	i_nlink;	/* directory entries */
	ushort	i_uid;		/* owner */
	ushort	i_gid;		/* group of owner */
	off_t	i_size;		/* size of file */
	struct mount	*i_mount; /* ptr to mount dev inode resides on	*/
	struct stdata	*i_sptr;  /* Associated stream.			*/
	struct {
		union {
			daddr_t i_a[NADDR];	/* if normal file/directory */
			short	i_f[NSADDR];	/* if fifio's */
		} i_p;
		daddr_t	i_l;		/* last logical block read (for read-ahead) */
	}i_blks;
	long	*i_map;		/* Ptr to the block number map for the file */
	struct locklist *i_locklist; /* CADMUS: locked region list */
};


extern struct inode inode[];	/* The inode table itself */

/* flags */
#define	ILOCK	01		/* inode is locked */
#define	IUPD	02		/* file has been modified */
#define	IACC	04		/* inode access time to be updated */
#define	IMOUNT	010		/* inode is mounted on */
#define	IWANT	020		/* some process waiting on lock */
#define	ITEXT	040		/* inode is pure text prototype */
#define	ICHG	0100		/* inode has been changed */
#define ISYN	0200		/* do synchronous write for iupdate */
#define ILAND	0400		/* inode is a special network directory */
#ifdef SELECT
#define ISEL    01000           /* inode is polled by select */
#endif

/* modes */
#define	IFMT	0170000		/* type of file */
#define		IFDIR	0040000	/* directory */
#define		IFCHR	0020000	/* character special */
#define		IFBLK	0060000	/* block special */
#define		IFREG	0100000	/* regular */
#define		IFLAN	0160000	/* lan block special */
#define         IFLNK   0120000 /* symbolic link */
#define		IFIFO	0010000	/* fifo special */
#define	ISUID	04000		/* set user id on execution */
#define	ISGID	02000		/* set group id on execution */
#define ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read, write, execute permissions */
#define	IWRITE	0200
#define	IEXEC	0100

#define DONT_FOLLOW 0100        /* or to second namei param if not to follow
				   symbolic links */

#define	i_addr	i_blks.i_p.i_a
#define	i_lastr	i_blks.i_l
#define	i_rdev	i_blks.i_p.i_a[0]
#define	i_netx	i_blks.i_p.i_a[1]

#define	i_faddr	i_blks.i_p.i_a
#define	NFADDR	10
#define	PIPSIZ	NFADDR*BSIZE
#define	i_frptr	i_blks.i_p.i_f[NSADDR-5]
#define	i_fwptr	i_blks.i_p.i_f[NSADDR-4]
#define	i_frcnt	i_blks.i_p.i_f[NSADDR-3]
#define	i_fwcnt	i_blks.i_p.i_f[NSADDR-2]
#define	i_waite	i_blks.i_p.i_f[NSADDR-3]
#define	i_waitf	i_blks.i_p.i_f[NSADDR-2]
#define	i_fflag	i_blks.i_p.i_f[NSADDR-1]
#define	IFIR	01
#define	IFIW	02

/***** CADMUS: John Bass record locking *****/
struct	locklist {			/* one allocated for each lock */
	/* NOTE: link must be first in struct */
	struct locklist *ll_link;	/* link to next lock region */
	int	ll_flags;		/* miscellaneous flags */
	struct proc *ll_proc;		/* process which owns region */
	off_t	ll_start;		/* starting offset */
	off_t	ll_end;			/* ending offset, zero is eof */
};

extern	struct locklist locklist[];	/* the lock table itself */
/********************************************/
