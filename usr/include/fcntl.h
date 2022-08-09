/* Flag values accessible to open(2) and fcntl(2) */
/*  (The first three can only be set by open) */
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR	2
#define	O_NDELAY	04	/* Non-blocking I/O */
#define	O_APPEND	010	/* append (writes guaranteed at the end) */
#define O_SYNC	 020	/* synchronous write option */

/* Flag values accessible only to open(2) */
#define	O_CREAT	00400	/* open with file create (uses third open arg)*/
#define	O_TRUNC	01000	/* open with truncation */
#define	O_EXCL	02000	/* exclusive open */

/* fcntl(2) requests */
#define	F_DUPFD	0	/* Duplicate fildes */
#define	F_GETFD	1	/* Get fildes flags */
#define	F_SETFD	2	/* Set fildes flags */
#define	F_GETFL	3	/* Get file flags */
#define	F_SETFL	4	/* Set file flags */
#define F_GETLK 5       /* Get record lock*/
#define F_SETLK 6       /* Set record lock, return if blocked */
#define F_SETLKW 7      /* Set record lock, wait if blocked */

/* file segment locking set data type - information passed to system by user */
struct flock {
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;		/* len = 0 means until end of file */
	short   l_pid;
};

/* file segment locking types (advisory) */
	/* Read lock */
#define	F_RDLCK	01
	/* Write lock */
#define	F_WRLCK	02
	/* Remove lock(s) */
#define	F_UNLCK	03

/* lockf(2) modes (mandatory, John Bass) */
#define	F_ULOCK		0	/* command type passed in mode */
#define	F_LOCK		1	/* command type passed in mode */
#define	F_TLOCK		2	/* command type passed in mode */
#define	F_TEST		3	/* command type passed in mode */
