/*
 * kmem.h: all about ioctl calls to /dev/kmem
 */

/* the structure returned by each /dev/kmem ioctl */
struct kmeminfo {
	caddr_t	km_adr;		/* address of first one in kmem */
	short	km_size;	/* size of each structure in bytes */
	short	km_count;	/* number of these structures in kernel */
};

/* command codes that /dev/kmem ioctl's know about */
#define	K_VAR	(('K'<<8)|0)	/* var structure */
#define	K_PROC	(('K'<<8)|1)	/* proc structures */
#define	K_TEXT	(('K'<<8)|2)	/* text structures */
#define	K_BUF	(('K'<<8)|3)	/* buf structures */
#define	K_FILE	(('K'<<8)|4)	/* file structures */
#define	K_INODE	(('K'<<8)|5)	/* inode structures */
#define	K_MOUNT	(('K'<<8)|6)	/* mount structures */
#define	K_CALLO	(('K'<<8)|7)	/* callout structures */
#define	K_ROOTD	(('K'<<8)|8)	/* root device */
#define	K_SWAPD	(('K'<<8)|9)	/* swap device */
#define	K_NSWAP	(('K'<<8)|10)	/* nswap */
#define	K_SWPLO	(('K'<<8)|11)	/* swplo */
#define	K_HERTZ	(('K'<<8)|12)	/* hertz */
#define	K_PORT	(('K'<<8)|13)	/* port structures */
#define	K_DLTAB	(('K'<<8)|14)	/* dltable structures */
