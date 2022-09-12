/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)kern-port:sys/types.h	10.6"*/

#ifndef __ATT_TYPES
#define __ATT_TYPES

typedef struct { long l[1]; }  *lphysadr;
typedef struct { int  r[1]; }  *physadr;
typedef struct { short r[1]; } *sphysadr;
typedef	long		daddr_t;	/* <disk address> type */
typedef	char *		caddr_t;	/* ?<core address> type */
typedef	unsigned char	unchar;
typedef	unsigned short	ushort;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;
typedef	ushort		ino_t;		/* <inode> type */
typedef	short		cnt_t;		/* ?<count> type */
typedef	long		time_t;		/* <time> type */
typedef long            label_t[9];
typedef	short		dev_t;		/* <old device number> type */
typedef	long		off_t;		/* ?<offset> type */
typedef	long		paddr_t;	/* <physical address> type */
typedef	int		key_t;		/* IPC key type */
typedef	unsigned char	use_t;		/* use count for swap.  */
typedef	short		sysid_t;
typedef	short		index_t;
typedef	short		lock_t;		/* lock work for busy wait */
typedef	unsigned int	size_t;		/* len param for string funcs */

struct utimbuf
{
	time_t actime;
	time_t modtime;
};

#ifndef _ENETADDR
#define _ENETADDR
/* Cadmus specific */
typedef struct {unsigned short hi,mi,lo;} enetaddr; /* Ethernet address */
typedef unsigned long ipnetaddr;                    /* IP address */
#endif

#endif __ATT_TYPES
