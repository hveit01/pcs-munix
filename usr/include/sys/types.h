/* @(#)types.h	6.2 */
typedef struct { long l[1]; }  *lphysadr;
typedef struct { int  r[1]; }  *physadr;
typedef struct { short r[1]; } *sphysadr;

typedef unsigned char	use_t;

typedef	long		daddr_t;
typedef	char *		caddr_t;
typedef	unsigned int	uint;
typedef	unsigned short	ushort;
typedef unsigned long   ulong;
typedef	ushort		ino_t;
typedef short		cnt_t;
typedef	long		time_t;
typedef long            label_t[9];
typedef	short		dev_t;
typedef	long		off_t;
typedef	long		paddr_t;
typedef	long		key_t;

/* Cadmus specific */
typedef struct {unsigned short hi,mi,lo;} enetaddr; /* Ethernet address */
typedef unsigned long ipnetaddr;                    /* IP address */

/* for select                                                           */
#ifndef FD_SETSIZE
#define FD_SETSIZE 64                           /* >= NOFILE            */
#endif
#define SEL_NSET   ((FD_SETSIZE) >> 5)

typedef struct fd_set {long fds_bits [SEL_NSET];} fd_set;

#define FD_SET(fd, fsetp) (fsetp)->fds_bits[(fd) >> 5] |= 1L << ((fd) & 0x1f)
#define FD_CLR(fd, fsetp) (fsetp)->fds_bits[(fd) >> 5] &= ~(1L << ((fd) & 0x1f))
#define FD_ISSET(fd, fsetp) (((fsetp)->fds_bits[(fd) >> 5] & (1L << ((fd) & 0x1f))) != 0)
#define FD_ZERO(fsetp) memset(fsetp, 0, sizeof(*(fsetp)))

struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

