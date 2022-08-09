/*
 *      Header for the Ethernet driver ether.c
 *
 *      The driver combines Basic Block Ports (bbp's) and
 *      general Ethernet Ports (ethpt's)
 */

/*
 *	Configuration parameters.
 */
#define NETHPT          20      /* number of ethpt's */
#define NPORT           MAXPORTS+NETHPT /* number of ports: bbp's + ethpt's */
#define MAXETHDATA      1500    /* Max block data length in bytes */
#define MINETHDATA      46      /* Min block data length in bytes */
#define ETHRETRIES      16      /* No of packet transmission retries before failure */

#define MIN_BBP_RQ      1       /* min. # of read aheads per bbp port */
#define MAX_BBP_RQ      8       /* max. # of read aheads per bbp port */
#define MIN_ETHPT_RQ    1       /* min. # of read aheads per ethpt port */
#define MAX_ETHPT_RQ    32      /* max. # of read aheads per ethpt port */
#define FREEMIN         1001    /* When a port is configured with DYNAMIC for
				 * the pi_inport value using ioctl( ,BBPSET, )
				 * a new unique bb_port number in this range is allocated */
#define	FREEMAX		4095
#define BBPTYPE         (('B'<<8)|('B'&0377))   /* bbp protocol type */
/*
 *	Flag definitions used in all structures
 */
#define	ROPEN	01		/* the structure is open */
#define	RREAD	02		/* is used as read */
#define	RWRITE	04		/* is used as write */

/*
 *      some useful macros
 */
#define ETHAEQ(a,b)     (a.hi==b.hi && a.mi==b.mi && a.lo==b.lo)    /* a==b */
#define OPENMODE(x)     (x&(RWRITE|RREAD))
#define ISBBP(x)        (x<MAXPORTS)    /* is it a bbp? */
#define ISETHPT(x)      (x>=MAXPORTS)   /* is it a ethpt? */
#define EVEN(x)         (x&1)?x+1:x     /* make odds even */

/*
 * Access to the net is via a fixed number of 'ports' which map onto
 * minor device numbers. These are not to be confused with Basic Block
 * Protocol port numbers (the number in the second minipacket of a Basic
 * block); such numbers are referred to as bb_ports.
 */

struct port {
	unsigned short  pt_flag;        /* internal flag */
	short  pt_mrc;                  /* max. # of read aheads */
	short  pt_rc;                   /* current # of read aheads */
	struct rbq     *pt_rbufd;       /* first block in the receive queue*/
	struct portinfo pt_info;	/* configurable info for this port */
	struct portenq	pt_enq;		/* enquiry structure for this port */

};

/*
 * 	pt_flag values
 *	ROPEN, RREAD, RWRITE
 */
#define	P_READSLEEP	010		/* user read is sleeping on this port */
#define	P_WRITESLEEP	020		/* user write is sleeping on this port */
#define	P_WTDONE	040		/* write successfully completed */
#define	P_WTERR		0100		/* error on write */
#define P_CONFIG	0400		/* port info has been loaded */
#define P_SWAP          01000           /* swap bytes in ethernet packets */
#define P_LONG_BUF      02000           /* extended read ahead facility */

/*
 *	synonyms
 */
#define pt_station      pt_info.pi_station
#define pt_ethadr       pt_info.pi_ethadr
#define	pt_outport	pt_info.pi_outport
#define	pt_accept	pt_info.pi_accept
#define	pt_inport	pt_info.pi_inport
#define pt_type         pt_info.pi_type
#define pt_sendport     pt_enq.pn_sendport
#define pt_sender	pt_enq.pn_sender
#define pt_sendadr      pt_enq.pn_sendadr
#define pt_xrslt	pt_enq.pn_xrslt
#define pt_blkavail	pt_enq.pn_blkavail

/*
 * Overall Port control
 */
struct portctrl {
	unsigned int    flag;           /* control flag */
	short           openct;         /* number of open ports */
	short		bb_free;	/* next free bb_port number */
};

/*
 *      portctrl commands
 */
#define POPEN   0                       /* a port has been opened */
#define PCLOSE  1                       /* a port has been closed */

#define PORTOPEN        (portctrl.flag & ROPEN)

/*
 * Format of Ethernet Packets
 */
struct ethpacket {
	enetaddr dest;                  /* destination address */
	enetaddr source;                /* source address */
	unsigned short type;            /* packet type ??? */
	unsigned short sourceid;        /* source id */
	unsigned short destid;          /* destination id */
	unsigned short sourceport;      /* source port */
	unsigned short destport;        /* destination port */
	unsigned short datalen;         /* length of user data */
	unsigned short data;            /* start of user data */
};
#define ETH_HDR_SIZE    14
#define CRC_SIZE 4
#define BBP_HDR_SIZE    (5*sizeof(short))       /* 10 */
#define PK_HDR_SIZE (sizeof(struct ethpacket) - sizeof(short))
