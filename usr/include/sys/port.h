/*
 *	Associated with each minor device is one of these structures, which
 *	is configured with the ioctl system call.
 */

struct portinfo {
	short           pi_type;        /* type of port, unused for Ethernet */
	unsigned short  pi_inport;      /* bb_port number by which this port
					 * is addressed by other stations 
					 */
	short           pi_station;     /* destination ring station */
	unsigned short  pi_outport;     /* destination bb_port number */
	unsigned short  pi_accept;      /* acceptable source station number */
	enetaddr        pi_ethadr;      /* destination ethernet address */
};

/*
 *	special values for pi_accept
 */
#define ANYONE  0xffff
#define NOONE	0

/*
 *	special value for pi_inport
 */
#define	DYNAMIC	1000	/* Uses dynamically allocated bb port. When ioctl
			 * sees this value in pi_inport on command BBPSET
			 * it allocates a new unique pi_inport value that
			 * is returned in subsequent BBPGET commands.
			 */
/*
 *	The following structure is for obtaining information about
 *	the current status of a port with the BBPENQ command in
 *	ioctl.
 */
struct portenq {
	short           pn_sender;      /* station number of sender of received block */
	short           pn_sendport;    /* port number of sender of received block */
	char		pn_xrslt;	/* last block transmission result */
	char		pn_blkavail;	/* a block is available to be read */
	enetaddr        pn_sendadr;     /* ethernet address of sender */
};

/*
 *	structure for statistics gathering
 */
struct bbstat {
	long    sb_rcvd;               /* blocks received */
	long    sb_txed;               /* blocks transmitted */
	long    sb_jams;               /* number of jams */
};

/*
 *	Block transmission results
 */
#define	BB_ACCEPTED	0
#define BB_ERROR        1
#define BB_ABSENT       2

/*
 *	ioctrl commands
 */
#define	BBPENQ	(('b'<<8)|1)		/* Enquire current status of port */
#define BBPSET	(('b'<<8)|2)		/* Write port configuration info */
#define BBPGET	(('b'<<8)|3)		/* read port configuration info */
#define BBPSIZE	(('b'<<8)|4)		/* set the size of the receiver
					 * buffer pool - super user only
					 */
#define BBPGSTAT (('b'<<8)|5)		/* collect statistics */
#define BBPZSTAT (('b'<<8)|6)		/* zero statistics - super user only */
