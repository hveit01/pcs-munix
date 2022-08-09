/*
 *      Header for the SBP driver
 *
 *      This is a simple driver for Interprocess-communication (IPC)
 *      A process will open a port by opening /dev/sbp0, /dev/sbp1,...,
 *      /dev/sbp49 (if MAXPORT == 50), until the open returns a valid
 *      file descriptor. If the open returns with errno EACCES, the port is
 *      already open.
 *      The port is then configured by a call ioctl(fd,BBPSET,&pi), where
 *      pi is struct portinfo pi; This will specify an input- and an
 *      output- port number. The input port number must be unique and
 *      identifies the port. Via the output port number the process can
 *      send data to another port, whose input port number must be equal
 *      to the output port number of this port.
 *      A unique port number can be obtained by writing the value
 *      DYNAMIC into pi_inport. The allocated number can be inquired with
 *      ioctl(fd,BBPGET,&pi);
 *      A block of data can be sent over the port to another process
 *      with a simple write(fd,buf,cnt) where cnt must be <= SBUFSIZE (see
 *      param.h).
 *      The write will be delayed if already another block has been sent
 *      to the port and not yet been read.
 *      A read will be delayed until a block is sent to this port.
 *      After a read the call ioctl(fd,BBPENQ,&pe) (given struct portenq pe)
 *      will return in pn_sendport the sender of the read block.
 *      After the call ioctl(fd,BBPENQ,&pe) pe will contain
 *      a nonzero value in pn_blkavail if a block is availablle for reading.
 *      After a write a call ioctl(fd,BBPENQ,&pe) will return in pn_xrslt
 *      the value BB_ACCEPTED if the block has been delivered to the port,
 *      BB_ABSENT otherwise.
 *
 */

struct portinfo {
	unsigned short  pi_inport;      /* port number by which this port
					 * is addressed by other stations 
					 */
	unsigned short  pi_outport;     /* destination port number */
};

/*
 *	special value for pi_inport
 */
#define DYNAMIC 1000    /* Uses dynamically allocated port. When ioctl
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
	short           pn_sendport;    /* port number of sender of received block */
	char		pn_xrslt;	/* last block transmission result */
	char		pn_blkavail;	/* a block is available to be read */
};

/*
 *	Block transmission results
 */
#define	BB_ACCEPTED	0
#define BB_ABSENT       2

/*
 *      ioctl commands
 */
#define	BBPENQ	(('b'<<8)|1)		/* Enquire current status of port */
#define BBPSET	(('b'<<8)|2)		/* Write port configuration info */
#define BBPGET	(('b'<<8)|3)		/* read port configuration info */
