/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)kern-port:sys/file.h	10.4"*/
/*
 * One file structure is allocated for each open/creat/pipe call.
 * Main use is to hold the read/write pointer associated with
 * each open file.
 */

typedef struct file
{
	short   f_flag;
	cnt_t	f_count;		/* reference count */
	short   f_msgcount;             /* references from message queue */
	char    f_vtty1,f_vtty2;         /* vtty:               dwm@HAYES
					  * f_vtty1 == F_UNUSED if normal,
					  * else direction of vtty pipe.
					  * f_vtty2 is descriptor index.
					  * values in sys/vtty.c.
					  */
	union {
		struct inode *f_uinode;	/* pointer to inode structure */
		struct file  *f_unext;	/* pointer to next entry in freelist */
	} f_up;
	off_t	f_offset;		/* read/write character pointer */
} file_t;

#define f_inode		f_up.f_uinode
#define f_next		f_up.f_unext

extern struct file file[];	/* The file table itself */
extern struct file *ffreelist;	/* Head of freelist pool */

/* flags */

#define	FOPEN	0xffffffff
#define	FREAD	0x01
#define	FWRITE	0x02
#define	FNDELAY	0x04
#define	FAPPEND	0x08
#define FSYNC	0x10
#define FINOD   0x20            /* for open(dev,ino,mode) */
#define FRCACH  0x40            /* Used for file and record locking cache*/
#define	FMASK	0x7f		/* FMASK should be disjoint from FNET */
#define FNET	0x80		/* needed by 3bnet */
/* next 3 from BSD4.3 */
#define FMARK   0x1000          /* mark during gc() */
#define FDEFER  0x2000          /* defer for next gc pass */
#define FASYNC  0x4000          /* signal pgrp when data ready */

/* open only modes */

#define	FCREAT	0x100
#define	FTRUNC	0x200
#define	FEXCL	0x400

/*
 * initialization value for f_vtty1
 */
#define F_UNUSED 0
