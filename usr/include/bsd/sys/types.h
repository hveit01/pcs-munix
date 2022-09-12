/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _TYPES_
#define	_TYPES_

#include "//usr/include/sys/types.h"

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

typedef	struct	_quad { long val[2]; } quad;
typedef	long	swblk_t;

typedef	u_short	uid_t;
typedef	u_short	gid_t;

#define	NBBY	8		/* number of bits in a byte */
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 64                           /* >= NOFILE            */
#endif
#define SEL_NSET   ((FD_SETSIZE) >> 5)

typedef struct fd_set {long fds_bits [SEL_NSET];} fd_set;

#define FD_SET(fd, fsetp) (fsetp)->fds_bits[(fd) >> 5] |= 1L << ((fd) & 0x1f)
#define FD_CLR(fd, fsetp) (fsetp)->fds_bits[(fd) >> 5] &= ~(1L << ((fd) & 0x1f))
#define FD_ISSET(fd, fsetp) (((fsetp)->fds_bits[(fd) >> 5] & (1L << ((fd) & 0x1f))) != 0)
#define FD_ZERO(fsetp) memset(fsetp, 0, sizeof(*(fsetp)))
#endif
