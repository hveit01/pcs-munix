/*
	Header file for the Ethernet Port Interface (see ethpt(4))
*/

struct ethptinfo {
	unsigned short  ei_proto;       /* Ethernet protocol type */
	char            ei_swapflag;    /* swap bytes in Ethernet packets */
	char            ei_rfuflag;     /* reserved for future use */
	enetaddr        ei_remadr;      /* address of remote Ethernet station
					 * write-only port: destination address
					 * read-only  port: source address of
					 *                  last packet received
					 */
	enetaddr        ei_locadr;      /* address of this Ethernet station
					 * write-only port: local address
					 * read-only  port: destination address of
					 *                  last packet received
					 */
};

/*
	IOCTL commands
*/
#define ETHPTSET        (('e'<<8)|1)    /* write port configuration info */
#define ETHPTGET        (('e'<<8)|2)    /* read port configuration info */
#define ETHPTSETBUF     (('e'<<8)|3)    /* set extended read ahead facility*/

/*
	Flags
*/
#define NOSWAP          0
#define SWAP            1

/*
 * definitions for extended read ahead facility
 */

#define MAXETHPT_RDAHEAD 32     /* max. # of read aheads per port */

typedef struct {
	short           pt_addr[MAXETHPT_RDAHEAD];
	short           pt_read_buf;
	short           pt_write_buf;
	short           pt_buffers;
} bufferinfo;
