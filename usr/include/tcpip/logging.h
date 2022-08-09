/********************************************************/
/* (Error-)Logging for TCP/IP Software                  */
/*                                                      */
/*                l o g g i n g . h                     */
/*                                                      */
/* This is the include file for obtaining logging in-   */
/* formation out of the kernel.                         */
/*                                                      */
/* Version 1.0, 22 Jul 86       j&j                     */
/********************************************************/

/*
 * USAGE of this module is as follows:
 * There is a need to catch all situations that 'can never occur'. This is
 * done by issueing a call:
 *
 *      Do_logging( sender_id, data_addr, data_len, urgent_flg );
 *              short   sender_id;
 *              caddr_t data_addr;
 *              int     data_len;
 *              boolean urgent_flg;
 *
 * All this logging info is recorded in a big buffer until a logging is
 * encountered, that has the urgent_flg set to TRUE. Then a signal is set
 * to indicate the logging process that logging data is there. The info will
 * then be read out by a special 'arpalogger' process and will be put into an
 * error-logging file.
 *
 * The error-logging file can later (or even on-line) be analyzed revealing
 * hints to debug the system. To keep the computational effort as low as
 * possible, the use of 'push logging info' should be restricted to the
 * really fatal situations. That program's name is 'analog'.
 *
 * Since there may be many different formats of data to be logged, every
 * logging-record consists of an id-field ( == sender_id ), a length-count
 * and the logging data itself.
 *
 * To handle the different formats in a user-friendly mode this file
 * ('logging.h') can be modified if there is need to add a tracepoint for
 * example. THERE IS NO NEED TO MODIFY THE 'analog'ER, IT ONLY MUST BE
 * RECOMPILED WITH THE NEW 'logging.h'.
 *
 * To install a new 'tracepoint' a few entries have to be made.
 * - a unique sender-id has to be created
 * - the format of the data at the tracepoint has to be defined
 * - an ascii-text must be written that is output by the 'analog' program
 *      whenever a report from that tracepoint is to be analyzed.
 *
 * The sender id is an unsigned short with the fields high-byte and low-byte.
 *      the high-byte contains a layer-code (xxxMOD, see below) and an id
 *      within that layer (1...255)     (the code 0x??00 must be reserved)
 *
 * The format of data is one of (DUMP_MSG, DUMP_TCB, ... (see below)) Each of
 *      these entries corresponds to a dump-routine in the 'analog'. If
 *      you add a new type you must add a dump-routine in the 'analog' pro-
 *      gram. Since it is possible to start 'analog' from a pipe after the
 *      'arpalogger' it is wise not to trace ALL logging infos but only the
 *      most eminent of these. Such an 'unconditional' trace-output can be
 *      forced if the dump-type is bitwise-ORed with the constant 'ONLINE'.
 *      If the 'analog'er is started per se, that canstant has no effect.
 *
 * The ascii-string merely has mnemonic function.
 *
 *
 * Example:
 * Add a log at the ip-layer receive-process where the situation 'wrong command
 *      message' is encounterd:
 *
 *      1) Add a statement like the following in the ip-section namely OUTSIDE
 *         the ifdef-endif construct.
 *              #define LOG_IPRCVERR    0x0301
 *         (assumed that the code 0x301 is free)
 *
 *      2) within the ifdef-endif of the ip-section the mnemonic string and
 *         the dumptype are specified.
 *              "IP: Received wrong command", DUMP_MSG | ONLINE,
 *         This string must be the first within the array 'ipcomments' since
 *         we choosed the number 0x??01 for that logpoint. Note also that
 *         the dump-code was ORed with ONLINE to show a fatal error that MUST
 *         NOT happen.
 *
 *      3) Alter 'ip.c' to include the correct version of 'logging.h'
 *
 *      4) Recompile 'analog.c'
 *
 *      5) Make a new 'iccprogram' and start it together with the 'arpalogger'
 */


/* here we will show the senders of a logging message together with the */
/* structure of their data. */

#define ICC_LOGFILE     "/usr/adm/tcpiplog"


/* structure of the header prepended to each logging info data portion */

struct logmsghdr {
	Ushort  sender;         /* code identifying the sender of the log */
	Ushort  len;            /* length of data portion THAT FOLLOWS */
}; /* endstruct logmsghdr */

#define NOPUSH  0               /* data not urgent */
#define PUSHLOG 1               /* urgent data */

/* definitions for the type of possible dumps */

#define ONLINE          0x8000  /* output these records iff in online mode */
#define DUMP_IGNORE     0       /* internally: suppress any output */
#define DUMP_NODUMP     1       /* simply for a trace that we were there */
#define DUMP_MSG        2       /* dump a message (icc-Format) */
#define DUMP_TCB        3       /* data portion for dump is a tcb */
#define DUMP_ASCII      4       /* human-readable text */
#define DUMP_HEX        5       /* if format is not known */
#define DUMP_UNIXMSG    6       /* dump a message (unix-Format) */



#define HOSTMOD 0x0000
#define TCPMOD  0x0100
#define UDPMOD  0x0200
#define IPMOD   0x0300
#define ICMPMOD 0x0400
#define ARPMOD  0x0500
#define LANCEMOD 0x0600
#define PROTMOD 0x0700
#define LOGMOD  0x0800
#define HELPMOD 0x0900
#define UNIXMOD 0x0a00


struct loganalyz {
	char    *comment;
	int     dumptype;
};
#define LS      sizeof(struct loganalyz)

/********************************************************/

/* loggings for the host interface (iccs side) */
#ifdef ANALYZING
struct loganalyz hostcomments[] = {
	"Host_Snd: Wrong msg to send to unix",  DUMP_MSG | ONLINE,
	"Host_Rcv: Illegal ioctl cmd-msg",      DUMP_MSG | ONLINE,
	"Host_Rcv: Illegal cmd_msg",            DUMP_MSG | ONLINE,
	"Host_Rcv: Host-cmd aborted with err",  DUMP_MSG | ONLINE,
	"L5_Rcv: Couldn't execute a message",   DUMP_MSG | ONLINE,
	"L5_Mgmt: Unknown cmd received",        DUMP_MSG | ONLINE,
	0L, DUMP_IGNORE
}; /* end hostcomments */
#define NR_HOSTCOM      ((sizeof hostcomments)/LS - 1)
#endif

/*              reserved 0x0000 */
#define LOG_HOSTSND      0x0001      /* msg icc->unix */
#define LOG_HOSTRCVIOCTL 0x0002      /* ioctl-msg unix->icc */
#define LOG_HOSTRCVCMD   0x0003      /* msg unix->icc */
#define LOG_HOSTRCVERR   0x0004      /* cmd unix->icc didn't succeed */
#define LOG_HOSTL5ERR    0x0005      /* l4-answer couldn't be processed */
#define LOG_HOSTMGMTERR  0x0006      /* illegel mgmt cmd received */


/********************************************************/

/* loggings for the tcp software */
#ifdef ANALYZING
struct loganalyz tcpcomments[] = {
	"Tcp: Send process gets wrong msg",             DUMP_MSG | ONLINE,
	"Tcp: Gen_syn with invalid syn_type",           DUMP_HEX | ONLINE,
	"Tcp: invalid event in state machine",          DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_send_event",         DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_close_event",        DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_abort_event",        DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_recv_event",         DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_retransmit_timeout", DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_ulp_timeout",        DUMP_HEX | ONLINE,
	"Tcp: invalid state in tcp_time_wait_timeout",  DUMP_HEX | ONLINE,
	0L, DUMP_IGNORE
}; /* end tcpcomments */
#define NR_TCPCOM       ((sizeof tcpcomments)/LS - 1)
#endif

/*             reserved 0x0100 */
#define LOG_TCPSND      0x0101          /* message with illegal cmd */
#define LOG_TCPGENSYN   0x0102          /* syn_type */
#define LOG_TCPSTATE    0x0103          /* event */
#define LOG_TCPSENDEV   0x0104          /* state */
#define LOG_TCPCLOSEEV  0x0105          /* state */
#define LOG_TCPABORTEV  0x0106          /* state */
#define LOG_TCPRECVEV   0x0107          /* state */
#define LOG_TCPRETRTO   0x0108          /* state */
#define LOG_TCPULPTO    0x0109          /* state */
#define LOG_TCPTIMETO   0x010a          /* state */


/********************************************************/

/* loggings for the udp software */
#ifdef ANALYZING
struct loganalyz udpcomments[] = {
	"Udp: Send process gets wrong msg",             DUMP_MSG | ONLINE,
	"Udp: Receive save failed",                     DUMP_MSG,
	0L, DUMP_IGNORE
}; /* end udpcomments */
#define NR_UDPCOM       ((sizeof udpcomments)/LS - 1)
#endif

/*              reserved 0x0200 */
#define LOG_UDPSND       0x0201         /* message with illegal cmd */
#define LOG_UDPRCVSAV    0x0202         /* message save failed */


/********************************************************/

/* loggings for the ip software */
#ifdef ANALYZING
struct loganalyz ipcomments[] = {
	"Ip: Request to send the following options",    DUMP_HEX | ONLINE,
	"Ip: Received the following options",           DUMP_HEX | ONLINE,
	"Ip: Received fragmented frame",                DUMP_HEX | ONLINE,
	"Ip: Received frame contains error",		DUMP_HEX,
	0L, DUMP_IGNORE
}; /* end ipcomments */
#define NR_IPCOM        ((sizeof ipcomments)/LS - 1)
#endif

/*             reserved 0x0300 */
#define LOG_IPSNDOPT    0x0301          /* dump hex of desired options */
#define LOG_IPRCVOPT    0x0302          /* dump hex the incoming options */
#define LOG_IPRCVFRAG   0x0303          /* dump hex the incoming ip packet */
#define LOG_IPRCVERR	0x0304		/* dump the incoming packet */


/********************************************************/

/* loggings for the icmp software */
#ifdef ANALYZING
struct loganalyz icmpcomments[] = {
	0L, DUMP_IGNORE
}; /* end icmpcomments */
#define NR_ICMPCOM      ((sizeof icmpcomments)/LS - 1)
#endif

/*              reserved 0x0400 */


/********************************************************/

/* loggings for the arp software */
#ifdef ANALYZING
struct loganalyz arpcomments[] = {
	0L, DUMP_IGNORE
}; /* end arpcomments */
#define NR_ARPCOM       ((sizeof arpcomments)/LS - 1)
#endif

/*              reserved 0x0500 */


/********************************************************/

/* loggings for the lance software */
#ifdef ANALYZING
struct loganalyz lancecomments[] = {
	"Lance: Should transmit 0-Packet",              DUMP_MSG | ONLINE,
	"Lance: Interrupt would free 0-Packet",         DUMP_HEX | ONLINE,
	"Lance: Received packet contains error",	DUMP_HEX | ONLINE,
	0L, DUMP_IGNORE
}; /* end lancecomments */
#define NR_LANCECOM     ((sizeof lancecomments)/LS - 1)
#endif

/*              reserved 0x0600 */
#define LOG_LANSNDNOP    0x0601         /* lance snd should xmit 0-Packet */
#define LOG_LANINTNOP    0x0602         /* lance intr did mix up packets */
#define LOG_LANRECERR	 0x0603	        /* packet received with error */
   /* last byte contains error code */
#   define LAN_CRC	1		/* crc error */
#   define LAN_FRAM	2		/* framing error */
#   define LAN_BUFFR	3		/* buffer not ready in time */
#   define LAN_LEN	4		/* packet was too long */

/********************************************************/

/* loggings for the protocol entry/exit software */
#ifdef ANALYZING
struct loganalyz protcomments[] = {
	0L, DUMP_IGNORE
}; /* end protcomments */
#define NR_PROTCOM      ((sizeof protcomments)/LS - 1)
#endif

/*              reserved 0x0700 */


/********************************************************/

/* loggings for the logging software */
#ifdef ANALYZING
struct loganalyz logcomments[] = {
	0L, DUMP_IGNORE
}; /* end logcomments */
#define NR_LOGCOM       ((sizeof logcomments)/LS - 1)
#endif

/*              reserved 0x0800 */


/********************************************************/

/* loggings for help functions */
#ifdef ANALYZING
struct loganalyz helpcomments[] = {
	"Helpfkt: Free-packet invalid pack-addrs",      DUMP_HEX | ONLINE,
	"Helpfkt: Free-packet already free",            DUMP_HEX | ONLINE,
	0L, DUMP_IGNORE
}; /* end helpcomments */
#define NR_HELPCOM      ((sizeof helpcomments)/LS - 1)
#endif

/*              reserved 0x0900 */
#define LOG_HLPFRE1PACK  0x0901         /* released packet not a packet */
#define LOG_HLPFRE2PACK  0x0902         /* released packet already free */


/********************************************************/

/* loggings for the unix side */
#ifdef ANALYZING
struct loganalyz unixcomments[] = {
	0L, DUMP_IGNORE
}; /* end unixcomments */
#define NR_UNIXCOM      ((sizeof unixcomments)/LS - 1)
#endif

/*              reserved 0x0a00 */


/********************************************************/


/* some general management stuff for the logging analyzer */

#ifdef ANALYZING
struct loganalyz *comtab[] = {
	hostcomments,
	tcpcomments,
	udpcomments,
	ipcomments,
	icmpcomments,
	arpcomments,
	lancecomments,
	protcomments,
	logcomments,
	helpcomments,
	unixcomments,
}; /* end comtab */

int lentab[] = {
	NR_HOSTCOM,
	NR_TCPCOM,
	NR_UDPCOM,
	NR_IPCOM,
	NR_ICMPCOM,
	NR_ARPCOM,
	NR_LANCECOM,
	NR_PROTCOM,
	NR_LOGCOM,
	NR_HELPCOM,
	NR_UNIXCOM,
}; /* end lentab */

#endif

