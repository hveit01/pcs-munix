/* Diskless node definitions */

#define DLBUFSIZ NBPP           /* buffer size for dltable buffers */
#define NMINUMS 10              /* max. number of inodes defined on master */
#define SWMAPSIZ (120)          /* size of diskless node swap allocation area */
				/* this should be the NPROC + NTEXT of the node */
#define BUFMASK ((long)3)       /* assumes 1k logical block numbers */
#define BUFBASE(x) ((x) & ~BUFMASK)
#define BUFOFF(x)  ((x) & BUFMASK)


struct dlpacket {
	enetaddr e_dest;		/* destination network address */
	enetaddr e_source;		/* source network address */
	ushort e_type;			/* ethernet packet type */
	ushort ip_word1;		/* IP version, IHL, TOS values */
	ushort ip_len;			/* IP datagram total length value */
	ushort ip_id;			/* IP id from fragmentation (unused) */
	ushort ip_word4;		/* IP Flags, and fragment offset */
	ushort ip_word5;		/* IP TTL and Protocol */
	ushort ip_cksum;		/* IP simple checksum on HEADER */
	ipnetaddr ip_srcaddr;		/* contains IP source address */
	ipnetaddr ip_destaddr;		/* contains IP destintation address */
	ushort rt_type;			/* packet type */
	short rt_command;		/* Command code */
	short rt_error;			/* Error returned by master */
	dev_t rt_dev;			/* Device code for i/o */
	short rt_size;			/* Size for swap operations */
#define rt_swdev rt_size		/* Swap device for DLINIT */
	short rt_start;			/* Start for swap operations */
	short rt_return;		/* Return value */
	short rt_index;			/* Index into dltable */
	short rt_seqno;			/* Sequence number of request */
	daddr_t rt_blkno;		/* Block number for i/o */
	short rt_pageio;		/* Part of page i/o to/from swap */
	char rt_data[UIMAXDATA_NEW_FRAG];    /* Data block buffer */
};

/* Command codes */
#define SWALLOC         1       /* malloc(swapmap, size) */
#define SWALLOCCAT      2       /* malloccat(swapmap, size, start) */
#define SWALLOCCL       3       /* alloccl (total,count) */
#define SWFREE          4       /* mfree(swapmap, size, start) */
#define RMTRBLK         5       /* read disk blocks */
#define RMTWBLK         6       /* write disk blocks */
#define DLINIT          7       /* tell master dlnode is there */
#define DLMPOLL         8       /* poll to find master node */
#define DLMREAD         9       /* unguarded read of master rootdev */
#define DLBOOT          10      /* poll to find bootserver node */
#define DLMASTER        11      /* first answer from master node */
#define DLALLOC         12      /* allocate disk block on root */
#define DLFREE          13      /* free disk block on root */
#define DLIALLOC        14      /* allocate inode on root */
#define DLIFREE         15      /* enter free inode in root superblock*/
#define DLWCSUP         16      /* write superblock of common root fs */
#define DLIREAD         17      /* read inode from server file system */
#define DLIUPDAT        18      /* update inode in common root fs     */
#define DLCLEAR         19      /* remove server and dltab entry      */
#define SWFREE_M32      20      /* free chunk of dbd-swap pages       */
#define DLSHUTDOWN      21      /* write superblock of common root fs */
				/* and shut dlserver down !           */

extern enetaddr master_id;	/* ethernet address of master */
extern ipnetaddr master_ip;	/* ip address of master */
extern int maxdlnactv;          /* max number of active dlnodes */
extern long req_q;		/* bit map of pending dl requests */

struct dltable {		/* dlnode accounting table */
	ushort dl_flags;	/* flags (see definitions below) */
	enetaddr dl_addr;	/* node ethernet address */
	ipnetaddr dl_ipaddr;	/* node ip address */
	dev_t dl_rootdev;	/* root device code */
	dev_t dl_swapdev;	/* swap device code */
	ushort dl_seqno;	/* sequence number of last packet received */
	unsigned dl_cmd;	/* Current command */
	int dl_error;		/* Last error code */
	ushort dl_bufid;	/* Buf id of received packet */
	time_t dl_lastin;	/* Time last request received */
/***    union {  **********************************************/
	  struct {		/* Swap operations */
	    unsigned dlsw_ret;		/* Return value */
	    unsigned dlsw_size;		/* Size */
	    unsigned dlsw_start;	/* Start */
	  } dl_sw;
#define dl_ret   /*dl_un.*/dl_sw.dlsw_ret
#define dl_size  /*dl_un.*/dl_sw.dlsw_size
#define dl_start /*dl_un.*/dl_sw.dlsw_start
	  struct {		/* I/O operations */
	    dev_t dlio_dev;		/* Device code */
	    daddr_t dlio_blkno;		/* Block number */
	    short dlio_pageio;		/* Page operation */
	    struct buf *dlio_bp;	/* Buf returned by read */
	  } dl_io;
#define dl_dev   /*dl_un.*/dl_io.dlio_dev
#define dl_blkno /*dl_un.*/dl_io.dlio_blkno
#define dl_pageio /*dl_un.*/dl_io.dlio_pageio
#define dl_swdev dl_pageio
#define dl_bp    /*dl_un.*/dl_io.dlio_bp
/***    } dl_un;  **********************************************/
	long * dlswapmap;               /* bitmap of allocate swap-space*/
	daddr_t dl_wbufbase;		/* Swap block number of wbuf */
	char dl_wbuffer[DLBUFSIZ];	/* Buffer to hold write data */
};

extern struct dltable dltable[];
extern ino_t minolist[];

/*
 * dltable flags
 */
#define DL_CONNECTED  01        /* dlnode is connected                   */
#define DL_PROCESSING 02	/* request is pending or being processed */
#define DL_RETRY      04        /* retry request received                */
#define DL_TEMP      010        /* entry only temporary used             */


extern dev_t rmtrootdev;        /* device of the remote root */
extern dev_t rmtswapdev;        /* remote swapping device */
extern dev_t rmtpipedev;        /* remote pipe device */
extern dev_t comrootdev;        /* common root device for diskless nodes */
extern char  rem_version;       /* common version of master and dlnode */

