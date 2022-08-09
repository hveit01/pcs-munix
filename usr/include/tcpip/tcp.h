/****************************************************************
*                       T C P                                   *
*                                                               *
* The following include file describes the TCP data structures. *
*                                                               *
* change history :                                              *
*       17/03/86 jh     original version                        *
*       09/06/86 j&j    add TCB structure                       *
****************************************************************/

struct tcp_packet {
	Ushort  tcp_src_port;   /* source port number of packet */
	Ushort  tcp_dest_port;  /* dest port number of packet */
	Ulong   tcp_seq_nr;     /* Sequence number or ISN */
	Ulong   tcp_ack_nr;     /* acknowledgement number */
	Ushort  tcp_flags;      /* flags for this packet */
	Ushort  tcp_window;     /* window size */
	Ushort  tcp_checksum;   /* reliable data transmission is fine */
	Ushort  tcp_urg_point;  /* where is our urgent data ? */
	char    tcp_options;    /* variable length options
				   will be multiple of 4 long */
	/* here the data should follow */

};


struct tcp_options_struct {
	Uchar   tcp_opt_kind;   /* discriminator for the following options */
	Uchar   tcp_opt_len;    /* length value for specific options */
	Uchar   tcp_opt_val[2]; /* value for this option: odd_align possible */
}; /* tcp_options_struct */

#define TCP_END_OPT_LIST        0
#define TCP_NOP_OPT             1
#define TCP_MAXSEG_OPT          2


#define WNDW_LOWMARK            0      /* after incrementing the window size */
#define WNDW_HIGHMARK        1024      /* from lower than LOW to more than */
				       /* HIGH an ACK message will be sent */


#define OWN_MAX_SEGSIZE         1024            /* may be tuned */

#define QUEUE_SIZE      2048    /* size of recv/send queue */

#define RETRANS_VALUE   15
#define LIFETIME        60      /* max time, a tcp packet may live */
#define DEFAULT_TIMEOUT (2 * LIFETIME)
#define CLOSE_TIMEOUT 3         /*UH: should be 2*LIFETIME, but on ethernet
				      3 is sufficient */
/* in 5 minutes any connection without traffic will be terminated */
#define KEEP_TIMEOUT    5*60

#define adapt(oldval,roundtrip) MIN(2 * LIFETIME, (oldval + roundtrip)/2)


#define ISN_INCREMENT   50L     /* value that is added to initial seqno every
				   time a connection is requested */

#define MIN_TCP_HEADER_LENGTH 5 /* long values */

#define TCP_DATA_OFF(x) ((x >> 12) & 0xF)
#define TCP_URG_FLAG    0x20
#define TCP_ACK_FLAG    0x10
#define TCP_PSH_FLAG    0x08
#define TCP_RST_FLAG    0x04
#define TCP_SYN_FLAG    0x02
#define TCP_FIN_FLAG    0x01

/* the following structure describes the current state of any open
   tcp connection */

enum tcp_state {CLOSED, LISTEN, SYN_RECVD, SYN_SENT, ESTAB,
		FIN_WAIT1, FIN_WAIT2, CLOSE_WAIT, CLOSING, LAST_ACK,
		TIME_WAIT };

enum tcp_event {UNSPEC_PASSIVE_OPEN, FULL_PASSIVE_OPEN, ACTIVE_OPEN, ABORTX,
		SENDX, CLOSE, NET_DELIVER, RETRANS_TIMEOUT, ULP_TIMEOUT,
		TIME_WAIT_TIMEOUT };

enum tcp_open_mode { ACTIVE,PASSIVE };

enum tcp_syn_type {ALONE, WITH_ACK, WITH_DATA };

enum tcp_reset_type { NO_REPORT, RA, NF, SP, UT, UA, UC, SF, HU, KT,
		      CURRENT, SEG };

enum tcp_ge_type { LESS, GREATER, EQUAL };

enum tcp_status_type { NONE, VALID, INVAL };

struct security
{
	Ushort  security;
	Ushort  compartment;
	Ushort  handling;
	Uchar   tcc[3];
}; /* struct security */

struct tcp_queue
{
	Ushort  length;                 /* length of ring */
	Ushort  loglen;			/* logical length of queue */	
	char    *base;                  /* pointer to ring base */
	Ulong   start;                  /* start ring offset */
	Ulong   end;                    /* end ring offset */
}; /* struct tcp_queue */

#ifndef ON_UNIX
struct tcb
{
	/* link fields for buffer management */
	struct  tcb *next;              /* next tcb in list */
	struct  tcb *prev;              /* prev tcb in list */

	MESSAGE *msg;                   /* a waiting message in <> 0 */

	/* original tcb data */
	enum tcp_state state;           /* our machines current state */
	Ulong   source_addr;            /* ip source address, long type */
	Ushort  source_port;            /* tcp source port */
	Ulong   destination_addr;       /* ip dest addr */
	Ushort  destination_port;       /* tcp dest port */
	Uint    lcn;                    /* local connection name */
	enum tcp_open_mode open_mode;   /* the open mode for this connection */
	Uchar   original_precedence;
	Uchar   actual_precedence;
	struct  security sec;
	Ushort  ULP_timeout_action;     /* what's to do in case of timeout */
	Ulong   send_una;               /* sequence number of unacked bytes */
	Ulong   send_next;              /* seqno to be used next */
	Ulong   send_free;              /* first free byte in send queue */
	Ushort  send_wndw;              /* the window size */
	Ulong   send_urg;               /* last byte of urgent data in send */
	Ulong   send_push;              /* same for pushed data */
	Ulong   send_lastup1;           /* seqno last update */
	Ulong   send_lastup2;           /* ackno last update */
	Ulong   send_isn;               /* initial seq no */
	Ushort  send_max_seg;           /* max segment size */
	Ushort  last_id;                /* frame id of last segment */
	struct  tcp_queue send_queue;   /* the data itself */
	Ulong   recv_next;              /* seqno of next byte for receive */
	Ulong   recv_save;              /* seqno of next byte for ULP */
	Ushort  recv_wndw;              /* receive window size */
	Ushort  recv_alloc;             /* # bytes ULP is waiting for */
	Ulong   recv_urg;               /* seqno of last byte of urg in rcv */ 
	Ulong   recv_push;              /* same for push */
	Ulong   recv_isn;               /* initial sequence number for recv */ 
	struct  tcp_queue recv_queue;   /* the received data */

	/* some byte values are here because of alignement */
	Uchar   sequence_no;            /* seqno of original message */
	boolean recv_finflag;           /* FIN received ? */
	boolean send_finflag;           /* FIN sent ? */
	boolean first_sendfin;          /* FIN sending first ? */
	boolean recv_was_closed;        /* window size of zero was reported */

	/* some timer management stuff */
	/* for all timers : >0 : means current timer value, timer is running 
			    =0 : timer has expired, state handling to be called
			    <0 : timer not running */
	short   keep_timer;
	short   retrans_timer;          /* retransmission timer */
	short   wait_timer;             /* timer for time wait state */
	short   ulp_timer;              /* timer for ULP timeouts */
	Ushort  ulp_action;             /* action which caused timer windup */

	/* max. timer values */
	short   retrans_interval;
	short   wait_interval;
	short   ulp_interval;

	/* round trip delay calculating */
	Ulong   round_trip_time;        /* send time of last measured byte */

}; /* struct tcb */
#endif

/* errors returned by TCP */

#define TCP_E_NO_ERROR  0               /* no error occured */
#define TCP_E_NOT_IMPL  201             /* function not yet implemented */
#define TCP_E_NO_CONN   ENOTCONN        /* no connection with given name */
#define TCP_E_CONN_EXISTS EADDRINUSE    /* connection already exists */
#define TCP_E_INSUF_RES ENOBUFS         /* insufficient resources */
#define TCP_E_SEC_PREC_UNALLOWED 205    /* security/precedence not allowed */
#define TCP_E_ILL_REQ   206             /* illegal request */
#define TCP_E_CONN_CLOSING 207          /* connection is closing */
#define TCP_E_REMOTE_ABORT ECONNABORTED
#define TCP_E_NETWORK_FAIL ENETDOWN
#define TCP_E_SEC_PREC_MIS 210
#define TCP_E_ULP_ABORT    211
#define TCP_E_ULP_TIMEOUT  ETIMEDOUT
#define TCP_E_ULP_CLOSE    213
#define TCP_E_SVC_FAIL     214
#define TCP_E_OPEN_FAIL    215
#define TCP_E_CONN_DOWN    216
#define TCP_E_PEER_TIMEOUT ETIMEDOUT

void tcp_timer_proc(), tcp_abort(), tcp_status(), tcp_recv(),
     tcp_open(), tcp_close(), tcp_send(), tcp_unspec_open(),
     tcp_send_event(), tcp_close_event(), tcp_abort_event(),
     tcp_recv_event(), tcp_retrans_timeout(), tcp_ulp_timeout(),
     tcp_time_wait_timeout();
