/********************************************************/
/* Socketemulation for TCP/IP Software                  */
/*                                                      */
/*                s o c k e m u . h                     */
/*                                                      */
/* This is the include file for the socket emulation    */
/* as mentioned in the TCP/IP-user interface manual on  */
/* host-programming.                                    */
/*                                                      */
/* Version 1.0, 28 Apr 86       j&j                     */
/*  V1.2  07 Aug 86  josch   values of ioctl's for the  */
/*                           admin and socket devices   */
/*                           modified                   */
/*  V1.1  25 Jun 86  josch   mods for asynchron message */
/*                           passing between host+icc   */
/********************************************************/

/* currently we are working with one icc only, so as a constant */
extern short arpa_icc;
#define ARPA_ICC        arpa_icc
#define NR_ICCS         1

/* the socket emulation is based on two kinds of devices:       */

#define SOCK_ADMIN      "/dev/arpa/admin"      /* 1) the administrator */
#define SOCK_SOCKET     "/dev/arpa/socket??"   /* 2) all the sockets itself */

#define NR_EXITS        32              /* nr of installable protocol exits */

#define NR_OF_SOCKETS   25              /* be sure to install all devices */
#define MIN_USER_PORT   IPPORT_RESERVED /* first assignable port number */
#define NR_ASSIGNABLE_PORTS     1024    /* nr of ports we may auto-assign */
#define MAX_USER_PORT   (MIN_USER_PORT + NR_ASSIGNABLE_PORTS - 1)

#define NR_ADMIN_READERS        2       /* only the logging/memdump readers */
#define SO_LOGGING_READER       0
#define SO_MEMDUMP_READER       1


#define MAX_REQUEST_TIME 120  /* allow 2 min between rq(socket) and the open */
#define ICC_WAIT_TIME   180*50          /* allow icc to work 3 minutes */
#define HOST_WAIT_TIME  180000L         /* allow host to work 3 minutes */

#define L5_TIMER_GRANULE     5000L      /* interval to check for keepalive */
#define KEEPALIVE_INTERVAL  60000L      /* send a packet every minute */


#define LOPRI   PPIPE           /* sleep with low priority */



/* the main work is done by issueing 'ioctl(2)' calls to these devices */

					/* a=the admin, s=one of the sockets */
#define SOIOC_GETSOCKET ( ('a'<<8)+ 0)  /* a-  get number of free sockets */
#define SOIOC_REINIT    ( ('a'<<8)+ 1)  /* a-  reinitialization of icc */
#define SOIOC_FORCE_INIT (('a'<<8)+ 2)  /* a-  complete icc-reinitialization */
#define SOIOC_KILLER    ( ('a'<<8)+ 3)  /* a-  kill of a sleeping process */
#define SOIOC_ALLRESET  ( ('a'<<8)+ 4)  /* a-  reset all sockets (host + icc)*/
#define SOIOC_SETREADER ( ('a'<<8)+ 5)  /* a-  install a process as a reader */
#define SOIOC_CLRREADER ( ('a'<<8)+10)  /* a-  uninstall a kernel a reader */
#define SOIOC_xxx       ( ('a'<<8)+15)  /* let there be enough readers */

#define SOIOC_SOCKET    ( ('s'<<8)+ 0)  /* -s  initialize socket device */
#define SOIOC_CONNECT   ( ('s'<<8)+ 1)  /* -s  perform 'connect(3)' call */
#define SOIOC_ACCEPT    ( ('s'<<8)+ 2)  /* -s  perform 'accept(3)' call */
#define SOIOC_RECEIVE   ( ('s'<<8)+ 3)  /* -s  switch for read in DGRAM-mode */
#define SOIOC_SEND      ( ('s'<<8)+ 4)  /* -s  switch for write in DGRAM-mode*/
#define SOIOC_SOCKADDR  ( ('s'<<8)+ 5)  /* -s  perform 'socketaddr(3)' call */
#define SOIOC_READADDR  ( ('s'<<8)+ 6)  /* -s ask for sender's addr after rcv*/
#define SOIOC_PEERADDR  ( ('s'<<8)+ 7)  /* -s  perform 'peeraddr(3)' call */
#define SOIOC_READPSBL  ( ('s'<<8)+ 8)  /* -2  ask whether read is possible */

/* error numbers for some socket calls */

	/* see /usr/include/errno.h */

/* information for reading from the kernel */

#define ADRD_UNUSED     0       /* this reader entry isn't used */
#define ADRD_IDLE       1       /* this entry is installed but not waited for*/
#define ADRD_SIGN       2       /* signal received from the icc */
#define ADRD_REQ        3       /* request sent to the icc */
#define ADRD_RSP        4       /* response received by the icc */
#define ADRD_ERROR      5       /* some error occurred (e.g. EINTR) */


struct memdump_reader {
	long    icc;            /* ICC to work with */
	caddr_t startaddr;      /* start address to install */
};

struct admin_reader {
	Uchar   state;          /* state of the automaton */
	Uchar   type;           /* type of reader entry */
	Uchar   iccs;           /* bit-array of iccs to transact with */
	Uchar   allowed;        /* bit-array of allowed iccs; */
	short   lasticc;        /* for round-robin-implementations */
	short   pid;            /* pid of logging-user */
	caddr_t buffer;         /* buffer sent to icc */
	Ushort  length;         /* datalength of last message */
	Ulong   data[2];        /* miscellaneous data for readers */
}; /* endstruct admin_reader */



struct sock_killer {
	Ushort  so_pid;         /* process id of process to kill, input */
	boolean so_killit;      /* flag whether to kill/check only, input */
	Uchar   so_sleep;       /* reason why process slept, output */
#define SLEEP_OFF       0       /* process not sleeping */
#define SLEEP_ON_OPEN   0x01    /* proc sleeping on close operation */
#define SLEEP_ON_CLOSE  0x02    /* proc sleeping during connect/accept */
#define SLEEP_ON_IOCTL  0x04    /* proc sleeping during send/receive */
#define SLEEP_ON_READ   0x08    /* proc sleeping during read */
#define SLEEP_ON_WRITE  0x10    /* proc sleeping during write */
#define SLEEP_IN_READ   0x20    /* proc sleeping in critical read */
#define SLEEP_IN_WRITE  0x40    /* proc sleeping in critical write */
#define SLEEP_IN_BUFRQ  0x80    /* proc sleeping for buffer */
}; /* endstruct sock_killer */

/* some structures to hand over the arguments to the ioctl */

struct sock_ioctl {
	Ushort  so_port;        /* connection number for THIS operation */
	Ulong   so_addr;        /* internet address of communication partner */
	short   so_type;        /* STREAM or DATAGRAM mode (TCP/UDP) */
	Ushort  so_options;     /* options used with 'socket(3)' call */
}; /* endstruct sock_ioctl */


/* data to be held per socket */

#define SSTAT_UNUSED  0 /* until a 'socket' call comes along */
#define SSTAT_MARKED  1 /* used until open(socket) */
#define SSTAT_OPENED  2 /* when the real open is executed */
#define SSTAT_TYPED   3 /* after type is specified by ioctl(socket) */
#define SSTAT_CONNCT  4 /* when a connect/accept has been done */
#define SSTAT_INTERR  5 /* internal error due to sequence nbrs */
#define SSTAT_LINGER  6 /* own close, but read possible */
#define SSTAT_HLFOPN  7 /* half open: foreign closed, but wrt psbl */
#define SSTAT_CLOSNG  8 /* socket closing, next state is: unused */

enum socmd_state {SOCMD_IDLE,   /* no such operation in progress */
		  SOCMD_REQUEST,/* waiting for a buffer to become free */
		  SOCMD_WAIT,   /* waiting for operation to become possible */
		  SOCMD_SENT,   /* command sent to partner, no response */
		  SOCMD_OK,     /* command was acked: operation ok */
		  SOCMD_ERROR,  /* command did not succed */
		  SOCMD_INHIB,  /* further execution of command impossible */
		  SOCMD_KILLED, /* execution was killed by the killer */
		  SOCMD_INTRUP, /* execution was interrupted -> EINTR */
};

struct sock_cmd_fsm {   /* finite state machine of socket cmd execution */
	Uchar   so_state;	/* state of command execution */
	boolean so_ready;	/* command may be executed one more time */
	Ushort  so_seqno;	/* sequence number of last command */
#ifdef ON_UNIX
	UNIX_MESSAGE *so_pendmsg; /* pending host's message for this cmd */
	struct packet *so_pendpack; /* packet address for the host */
	Ushort  so_errno;	/* errno value of the last cmd execution */
	Ushort  so_pid;		/* process executing this command */
#endif
#ifdef ON_ICC
	MESSAGE *so_pendmsg;	/* pending host's message for this cmd */
#endif
};


struct sock_info {
/* general state stuff */
	Uchar   so_state;       /* global state of socket */
	Uchar   so_icc;         /* destination icc for this socket */
	Ushort  so_number;      /* number of this socket (for convenience) */
	Ushort  so_type;        /* (protocol)type of socket: STREAM or DGRAM */
	Ushort  so_options;     /* options of this socket */
	Ulong   so_readbytes;   /* bytes read from this socket */
	Ulong   so_writebytes;  /* nr bytes written to here */

/* some net connection stuff */
	Ushort  so_thisport;    /* port number on this side of the connection*/
	Ulong   so_thisaddr;    /* local Internet address */
	Ushort  so_destport;    /* the communication partner's port number */
	Ulong   so_destaddr;    /* destination Internet address */
	Ushort  so_lingertime;  /* time to hold a closed connection open */

#ifdef ON_UNIX
/* protocol entry/exit management */
	short   so_firstexit;   /* pointer to first installed exit */
			    /* other protentry info is contained in the msgs */
#endif

	struct sock_cmd_fsm     /* one state for each major driver call */
		so_open,        /* open: executable only once */
		so_read,        /* read: should be executable more than once */
		so_write,       /* write: */
		so_ioctl,       /* ioctl: everything else */
		so_close;       /* close: maybe half closing a socket */

#ifdef ON_UNIX
	Uint	so_pendread;    /* flag/count if there is read data pending */
/*	Ushort	so_pendread;    /* flag/count if there is read data pending */
	Ulong   so_lastip;      /* address of last incoming msg's sender */
	Ushort  so_lastport;    /* port # of that sender */
	Uint    so_lastcount;   /* nr of bytes transported in last msg */
/*	Ushort  so_lastcount;   /* nr of bytes transported in last msg */
	Ulong   so_rqtime;      /* system time of last request */
#endif
#ifdef ON_ICC
	PORT    so_waitport;    /* Buffer for non-TCP-Data frames */
	short   so_waitmsgs;    /* nr of messages waiting in that port */
	TIME    so_lastaccs;    /* systime of last read or write */
#endif
}; /* endstruct sock_info */

