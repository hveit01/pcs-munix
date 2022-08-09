/*	mtio.h	4.7	81/11/03	*/
/*
 * Structures and definitions for mag tape io control command
 */

/* mag tape io control commands */
#define MTIOCTOP        (('M'<<8)|'T')    /* do a mag tape op */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define MTWEOF	0	/* write an end-of-file record */
#define MTFSF	1	/* forward space file */
#define MTBSF	2	/* backward space file */
#define MTFSR	3	/* forward space record */
#define MTBSR	4	/* backward space record */
#define MTREW	5	/* rewind */
#define MTOFFL	6	/* rewind and put the drive offline */
#define MTNOP	7	/* no operation, sets status only */
#define MTSWAP  8       /* swap bytes from tape */
#define MTNOSWAP 9      /* do not swap bytes from tape */
