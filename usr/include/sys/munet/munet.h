/* @(#) munet.h         1.2     Apr. 27 1987    */

/*
 * Include file for MUNIX/NET parameters and structures.
 */

/* Definitions for statistic functions */
#define STAT
#define FRAGM

/*
 * Configuration parameters.
 */

#undef  VERBOSE         1       /* specifies whether MUNIX/NET errors are printed */
#undef  TIMEDIFF                /* for research only */
#define UIMAXPATH	100	/* Max characters in path specification */
#define UIMAXDATA_OLD   1024    /* Old max block data length in bytes */
#define UIMINPACKET     60      /* OLD minimum packet size for mnpacket */
#define UIMIN_MNPKT     90      /* Minimum packet size for mnpacket */
				/* E_HEADER_SIZE+IP_HEADER_SIZE+UU_HEADER_SIZE */
#define UIMAXDATA_NEW_NOFR 1424 /* New max block data length in bytes */
				/* UIMAXPACKET-UIMIN_MNPKT */
#define UIMAXDATA_NEW_FRAG 4096 /* New max block data length in bytes   */
				/* with fragmentation                   */
#define UIMAXPACKET	1514	/* Maximum packet size for ethernet */
#define MAXREMCORE      25      /* Maximum size of remote core (pages) */


/*
 * Modes for uipacket calls.
 */

#define UIREAD		1	/* Read and sleep till satisfied */
#define UIGUARD         2       /* Check for an open port */
#define UIREADT		3	/* Test read ahead, return w/ or wo/ data */
#define UIRDSW          4       /* Process received packet */
#define UIWRITE         5       /* Write packet */
#define UIKILL          6       /* Send signal */
#define UIURUNC		7	/* Connect backwards for remote execution */
#define UIMONITOR	8	/* Internal monitor, collect n records */
#define UIXNAME		9	/* Internal get all utsnames form net */
#define UIWATCHDOG	10	/* Disconnect nodes that are no longer here */
#define UICERBERUS	11	/* Tell other nodes that we are still here */
#define UIBPDATA	12	/* Pass in bpdata structures for dlsupport */
#define UIOMITAB	13	/* Get internal inode table for onmaster */
#define UIGROUTE	14	/* Munge gateway entries in routing table */
#ifdef ONMASTER
#define UIOCMINO	15	/* Build master inode list */
#endif
#define UIGETPORT       16      /* Set input port number and put up read ahead */
#define UICLOSEPORT     17      /* Close input port */
#define UIWTSW          18      /* Process packet to be transmitted */
#define UIWRITER        19      /* Write packet and wait for reply */
#ifdef STAT
#define UISTREAD        20      /* Read statistics from kernel    */
#define UISTEN          21      /* Enable collection of stat.data */
#define UISTDIS         22      /* Disable collection of stat. data     */
#define UISTCLR         23      /* Clear statistic variables in kernel  */
#endif STAT
#define UIVERSION       24      /* check receiver and kernel version    */
#define UINDINFO        25      /* get info about nodes                 */
#define UIVERS          26      /* get node version number              */
#define UINETTIME       27      /* get network standard time            */
#define UIPSARGS        28      /* set receiver arguments for ps        */
#define UIUNITE		29	/* read in mapping informations		*/

/*
 * Format of a complete MUNIX/NET packet as it exists on the lan
 */

struct mnpacket {
/* Ethernet header */
	enetaddr e_dest;		/* destination Ethernet address */
	enetaddr e_source;		/* source Ethernet address */
	ushort e_type;			/* Ethernet packet type */
/* IP header */
	ushort ip_word1;		/* IP version, IHL, TOS values */
	ushort ip_len;			/* IP datagram total length value */
	ushort ip_id;			/* IP id from fragmentation (unused) */
	ushort ip_word4;		/* IP flags, and fragment offset */
	ushort ip_word5;		/* IP TTL and protocol */
	ushort ip_cksum;		/* IP simple checksum on header */
	ulong ip_srcaddr;		/* contains IP source address */
	ulong ip_destaddr;		/* contains IP destination address */
/* MUNIX/NET data */
	struct uupacket {
		ushort	uu_type;	/* packet type (must be first) */
		ipnetaddr uu_node;	/* destination/source node */
		short   uu_toport;      /* destination port */
		short   uu_fromport;    /* source port */
		short	uu_callno;	/* system call number */
		short	uu_datalen;	/* length of uupacket */
		short	uu_error;	/* return error code */
		short   uu_fd;          /* file descriptors */
		dev_t	uu_dev;		/* device codes */
		off_t	uu_offset;	/* file offsets */
		long	uu_rwcnt;	/* transfer count for r/w */
		short	uu_count;	/* byte counts */
		short	uu_mode;	/* file modes */
		short	uu_cmask;	/* file creation masks */
		short   uu_altport;     /* parent/child ports */
		ushort  uu_euid;        /* effective uid */
		ushort  uu_egid;        /* effective gid */
		ushort  uu_stamp;       /* retry stamp */
		time_t  uu_timestamp;   /* timestamp for boottime */
		short   uu_signal;      /* signal number */
		struct	inode *uu_ip;	/* inode pointers */
		union {
			short pi[2];	/* miscellaneous shorts */
			long pl[1];	/* miscellaneous longs */
		} uu_args;
#define uu_argi uu_args.pi
#define uu_argl uu_args.pl
		char uu_data[UIMAXDATA_NEW_FRAG];/* data or path names */
	} uu;
};

/* definitions relevant to the Ethernet level 0 header */
#define E_HEADER_SIZE	14		/* Ethernet header size in bytes */
#define ETHERPUP_IPTYPE	0x0800		/* IP protocol in "e_type" field */
#define ETHERPUP_ARPTYPE 0x0806		/* ARP protocol in "e_type" field */

/* definitions relevant to the IP header */
#define IP_HEADER_SIZE	20 		/* IP header size in bytes */
#define IP_VERSION	0x4000		/* IP version */
#define IP_IHL		0x0500		/* IP header length (the minimum) */
#define IP_TOS		0x0058		/* IP type of service */
#define IP_TTL		0x0800		/* IP time to live */
#define IP_FFLAG	0x0600		/* IP fragmentation flag (don't) */
#define IP_PROTO_MUNET  0x0044          /* IP sub-protocol type MUNIX/NET */
#define IP_PROTO_ICMP	0x0001		/* IP sub-protocol type ICMP */

/* definitions of codes recognized in the uu_type field */
#define UUTYPE (('U' << 8) | 'U')       /* MUNIX/NET RPC-packets */
#define BCTYPE (('B' << 8) | 'C')	/* cerberus I-am-here packets */
#define RTTYPE (('R' << 8) | 'T')	/* diskless node packets */
#define UPTYPE (('U' << 8) | 'P')	/* dlupdate sync packets */
#define UNTYPE (('U' << 8) | 'N')	/* uxname packets */


struct upt_info {                       /* used with uidoit call UIGETPORT */
	short   portno;                 /* my port number */
	short   acc_port;               /* port number of partner */
	ipnetaddr acc_ipaddr;           /* address of partner */
	short   flag;                   /* indicates server if set */
};


/* definition of bits in pt_flag field */
#define	RTHERE 		0x0001		/* remote machine is there */
#define	WTHERE 		0x0002		/* remote machine was there */

/*
 * Miscellaneous details.
 */

/* external declarations */
extern enetaddr bcname;			/* ethernet address for broadcast */
extern enetaddr noname;			/* ethernet address for no one */
#define ipbcname ((ipnetaddr)-1)	/* ip address for broadcast */
#define ipnoname ((ipnetaddr)0)		/* ip address for no one */

/* commonly used macros */
#define splnet() spldisk()
#define UISAME(a,b) ((a)->lo==(b)->lo && (a)->mi==(b)->mi && (a)->hi==(b)->hi)
#define bufnum(buf) (buf & 0x00ff)	/* buffer number from buffer id */
#define bufdev(buf) (buf & 0xff00)	/* major part of dev from buffer id */
#define bufmaj(buf) (bufdev(buf) >> 8)	/* same as bufdev but for indexing */
#define FILE_IS_REMOTE(x)       ((x->i_flag & ILAND) != 0)      /* true if remote file */
#define FILE_IS_LOCAL(x)        ((x->i_flag & ILAND) == 0)      /* true if local file */
#define LAN_DEV(x)              x->i_mnton->m_dev               /* minor device number of lan special file where x is mounted on */
#define BREAK_ON_ERROR          if (u.u_error != 0) { suword(&upkptr->uu_error,u.u_error);break;}
#define INC_SEQN(x)     if (++x == 65535 ) x = 1        /* increment sequence number */
/* flag parameter in uisend */
#define RPC             0       /* implicit connect followed by a remote procedure call */
#define IMPLURUN        1       /* only implicit connect for urun command */

/* Definitions for locate */
#define NO_RESULT 0             /* namei or getf return value in user-structure invalid */
#define RESULT 1                /* return value of namei or getf in user-structure */
#define LOCAL 0                 /* local systemcall */
#define REMOTE 1                /* remote systemcall */
#define FAILURE -1              /* systemcall caused error condition */

/* Definitions for sending and receiving packets */

#define WPRI (PZERO-6)  /* sleep priority for xmit completion */
#define BPRI (PZERO-1)	/* sleep priority for xmit buffer allocation */
#define RPRI (PZERO+1)	/* sleep priority for packet reading */

#define WDOGT   90      /* disconnect remote node if we do not get a message by this time */
#define WSTIME (2 * hz) /* if write is not complete by this time wakeup */
#define RSTIME (5 * hz) /* if there is no answer within this time wakeup */
			/* RSTIME is used to create a random to_value which
			   lies between RSTIME and 2*RSTIME                   */
#define LGTIME (60* hz) /* long timeout for "wait_for_ever" systemcalls */

#define MXREDO 4        /* max number of redo for uisend */
#define NXPORTS 20      /* max number of concurrent transmissions */

#define XPTAKEN	0x0001	/* xmit port being used */
#define XPERROR	0x0002	/* xmit port error flag */
#define XPDONE	0x0004	/* xmit port completed transmission */
#define XPTIMEO	0x0008	/* xmit port has run out of time */
#define XPNOSLP 0x0010  /* don't wakeup, we don't sleep */

