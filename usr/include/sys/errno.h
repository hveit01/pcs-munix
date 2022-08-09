/* @(#)errno.h	6.2 */
/*
 * Error codes
 */

#define	EPERM	1	/* Not super-user			*/
#define	ENOENT	2	/* No such file or directory		*/
#define	ESRCH	3	/* No such process			*/
#define	EINTR	4	/* interrupted system call		*/
#define	EIO	5	/* I/O error				*/
#define	ENXIO	6	/* No such device or address		*/
#define	E2BIG	7	/* Arg list too long			*/
#define	ENOEXEC	8	/* Exec format error			*/
#define	EBADF	9	/* Bad file number			*/
#define	ECHILD	10	/* No children				*/
#define	EAGAIN	11	/* No more processes			*/
#define	ENOMEM	12	/* Not enough core			*/
#define	EACCES	13	/* Permission denied			*/
#define	EFAULT	14	/* Bad address				*/
#define	ENOTBLK	15	/* Block device required		*/
#define	EBUSY	16	/* Mount device busy			*/
#define	EEXIST	17	/* File exists				*/
#define	EXDEV	18	/* Cross-device link			*/
#define	ENODEV	19	/* No such device			*/
#define	ENOTDIR	20	/* Not a directory			*/
#define	EISDIR	21	/* Is a directory			*/
#define	EINVAL	22	/* Invalid argument			*/
#define	ENFILE	23	/* File table overflow			*/
#define	EMFILE	24	/* Too many open files			*/
#define	ENOTTY	25	/* Not a typewriter			*/
#define	ETXTBSY	26	/* Text file busy			*/
#define	EFBIG	27	/* File too large			*/
#define	ENOSPC	28	/* No space left on device		*/
#define	ESPIPE	29	/* Illegal seek				*/
#define	EROFS	30	/* Read only file system		*/
#define	EMLINK	31	/* Too many links			*/
#define	EPIPE	32	/* Broken pipe				*/
#define	EDOM	33	/* Math arg out of domain of func	*/
#define	ERANGE	34	/* Math result not representable	*/
#define	ENOMSG	35	/* No message of desired type		*/
#define	EIDRM	36	/* Identifier removed			*/
#define	ECHRNG	37	/* Channel number out of range		*/
#define	EL2NSYNC 38	/* Level 2 not synchronized		*/
#define	EL3HLT	39	/* Level 3 halted			*/
#define	EL3RST	40	/* Level 3 reset			*/
#define	ELNRNG	41	/* Link number out of range		*/
#define	EUNATCH 42	/* Protocol driver not attached		*/
#define	ENOCSI	43	/* No CSI structure available		*/
#define	EL2HLT	44	/* Level 2 halted			*/
#define	ENOSWP	45	/* Out of swap space			*/
#define	EXPATH	46	/* Path continues onto another machine	*/
#define EXREDO  47      /* MUNIX/NET request for a retry        */
#define EDEADLK 48      /* Record locking deadlock              */
#define ENOUARP	49	/* Could not resolve IP addr, host down */
#define ENOUGW	50	/* No available gateway in route table	*/
#define ELOOP   51      /* symbolic links form endless loop     */
#define EXSYMPATH 99    /* symbolic link to another machine	*/


/* the following errnos were taken from 4.2bsd for TCP/IP */
/* the values were all augmented by 100, so that there won't be a conflict */


/* non-blocking and interrupt i/o */
#define EWOULDBLOCK     135             /* Operation would block */
#define EINPROGRESS     136             /* Operation now in progress */
#define EALREADY        137             /* Operation already in progress */
/* ipc/network software */

	/* argument errors */
#define ENOTSOCK        138             /* Socket operation on non-socket */
#define EDESTADDRREQ    139             /* Destination address required */
#define EMSGSIZE        140             /* Message too long */
#define EPROTOTYPE      141             /* Protocol wrong type for socket */
#define ENOPROTOOPT     142             /* Protocol not available */
#define EPROTONOSUPPORT 143             /* Protocol not supported */
#define ESOCKTNOSUPPORT 144             /* Socket type not supported */
#define EOPNOTSUPP      145             /* Operation not supported on socket */
#define EPFNOSUPPORT    146             /* Protocol family not supported */
#define EAFNOSUPPORT    147  /* Address family not supported by protocol fam */
#define EADDRINUSE      148             /* Address already in use */
#define EADDRNOTAVAIL   149             /* Can't assign requested address */

	/* operational errors */
#define ENETDOWN        150             /* Network is down */
#define ENETUNREACH     151             /* Network is unreachable */
#define ENETRESET       152           /* Network dropped connection on reset */
#define ECONNABORTED    153             /* Software caused connection abort */
#define ECONNRESET      154             /* Connection reset by peer */
#define ENOBUFS         155             /* No buffer space available */
#define EISCONN         156             /* Socket is already connected */
#define ENOTCONN        157             /* Socket is not connected */
#define ESHUTDOWN       158             /* Can't send after socket shutdown */
#define ETOOMANYREFS    159             /* Too many references: can't splice */
#define ETIMEDOUT       160             /* Connection timed out */
#define ECONNREFUSED    161             /* Connection refused */

/* should be rearranged */
#define EHOSTDOWN       164             /* Host is down */
#define EHOSTUNREACH    165             /* No route to host */
#define ENOTEMPTY       166             /* Directory not empty */


