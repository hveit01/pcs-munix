/* @(#)flock.h  1.4 */

/* file segment locking set data type - information passed to system by user */
/* it is also found in fcntl.h */
#ifndef	F_RDLCK
struct	flock	{
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;		/* len = 0 means until end of file */
	short   l_pid;
};
#endif

/* file segment locking types */
#define	F_RDLCK	01	/* Read lock */
#define	F_WRLCK	02	/* Write lock */
#define	F_UNLCK	03	/* Remove lock(s) */

/* file locking structure (connected to file table entry) */
struct	filock	{
	struct	flock set;	/* contains type, start, and length */
	union	{
		int wakeflg;	/* for locks sleeping on this one */
		int blkpid;	/* pid of blocking lock
				 * (for sleeping locks only)
				 */
	}	stat;
	struct	filock *prev;
	struct	filock *next;
};

/* table to associate files with chain of locks */
struct	flino {
	dev_t	fl_dev;
	ino_t	fl_number;
	int	fl_refcnt;	 /* # of procs currently referencing this flino */
#if	u3b && !SIMPLEX
	sema_t	fl_sem;		 /* semaphore for AP machine */
#endif
	struct	filock *fl_flck; /* pointer to chain of locks for this file */
	struct	flino  *prev;
	struct	flino  *next;
};

/* file and record locking configuration structure */
/* record and file use totals may overflow */
struct flckinfo {
	long recs;	/* number of records configured on system */
	long fils;	/* number of file headers configured on system */
	long reccnt;	/* number of records currently in use */
	long filcnt;	/* number of file headers currently in use */
	long rectot;	/* number of records used since system boot */
	long filtot;	/* number of file headers used since system boot */
};

extern struct flckinfo	flckinfo;
extern struct filock  	flox[];
extern struct flino   	flinotab[];

/* copied from fcntl.h */
#define	F_DUPFD	0	/* Duplicate fildes */
#define	F_GETFD	1	/* Get fildes flags */
#define	F_SETFD	2	/* Set fildes flags */
#define	F_GETFL	3	/* Get file flags */
#define	F_SETFL	4	/* Set file flags */
#define F_GETLK 5       /* Get record lock*/
#define F_SETLK 6       /* Set record lock, return if blocked */
#define F_SETLKW 7      /* Get record lock, wait if blocked */
