/****************************************************************
*                       message.h                               *
*                                                               *
* The following include file describes the structure of the     *
* various messages that are used between the processes on the   *
* ICC and the host.                                             *
*                                                               *
* Date : 28.04.86                                               *
****************************************************************/

/* sub id field to determine the real receiver */

/* the sub ids for socket calls */
#define SOCK_OPEN       0       /* These messages are inited by the host */
#define SOCK_CLOSE      1
#define SOCK_IOCTL      2
#define SOCK_READ       3
#define SOCK_WRITE      4
#define SOCK_STATUS     5       /* inited mainly by the ICC to inform host */

/* parameters for socket calls */

struct so_open_msg
{
	Ushort  so_number;              /* the number of the opened socket */
	Ushort  so_seqno;               /* sequence number for rq/rsp-mapping*/
	short   so_retcode;             /* <> 0 : error code */
}; /* struct so_open_msg */

struct so_close_msg
{
	Ushort  so_number;              /* the socket number to close */
	Ushort  so_seqno;               /* sequence number for rq/rsp-mapping*/
	short   so_retcode;             /* <> 0 : error code */
}; /* struct so_close_msg */

struct so_ioctl_msg
{
	Ushort  so_number;
	Ushort  so_seqno;               /* sequence number for rq/rsp-mapping*/
	short   so_retcode;             /* <> 0 : error code */
	Ushort  so_ioctl;               /* ioctl value */
	Ushort  so_port;                /* src or dest port of the connection*/
	Ulong   so_ip_adr;              /* ip address */
	Ushort  so_options;             /* the options for this socket */
	Ushort  so_type;                /* stream (TCP) or datagram (UDP) */
#define so_lowbound   so_port  /* just redefine some locations for protexits */
#define so_highbound  so_options
}; /* struct so_ioctl_msg */

struct so_write_msg
{
	Ushort  so_number;
	Ushort  so_seqno;               /* sequence number for rq/rsp-mapping*/
	short   so_retcode;             /* <> 0 : error code */
	Ushort  so_len;                 /* length of data to transmit */
	caddr_t so_data;                /* where is it ? (phys UNIX address) */
	boolean so_lastblock;           /* flag whether last of multiple blks*/
}; /* struct so_write_msg */

struct so_read_msg
{
	Ushort  so_number;
	Ushort  so_seqno;               /* sequence number for rq/rsp-mapping*/
	short   so_retcode;             /* <> 0 : error code */
	Ushort  so_len;                 /* max. len, real on return */
	caddr_t so_data;
	Ushort  so_port;                /* port number of last read */
	Ulong   so_ipaddr;              /* source ip addr of last rcvd pack */
	boolean so_lastblock;           /* flag whether last of multiple blks*/
}; /* struct so_read_msg */


#define SOSTAT_CONNDOWN         0x0001  /* connection is down */
#define SOSTAT_CONNUP           0x0002  /* connection is up (again) */
#define SOSTAT_RECVPSBL         0x0004  /* read is possible, see parameters */
#define SOSTAT_SENDBLCK         0x0008  /* send is (momentarily) blocked */
#define SOSTAT_SENDCONT         0x0010  /* continuing to send */
#define SOSTAT_ERROR            0x0020  /* report an error on a prev. cmd */

struct so_status_msg
{
	Ushort  so_number;
	Ushort  so_seqno;               /* sequence number for rq/rsp-mapping*/
	short   so_retcode;
	Ushort  so_flags;               /* one or more of the status bits */
	Ushort  so_char_read;           /* number of avail. chars for user */
	Ushort  so_char_write;          /* number of characters waiting */
#define so_msgs_read so_char_write      /* overlay !! */
#define so_cmd       so_char_write      /* another overlay */
	Ushort  so_own_port;            /* own port number */
	Ushort  so_for_port;            /* foreign port number */
	Ulong   so_own_ip_addr;         /* own ip address */
	Ulong   so_for_ip_addr;         /* foreign ip address */
}; /* struct so_status_msg */



/* the sub-id fields for Exit and Entries */
#define PROT_LANCE      0
#define PROT_IP         1
#define PROT_IP_GATE    2

/* command field values */
#define PEX_INSTALL     0       /* install protocol exit */
#define PEX_CANCEL      1       /* uninstall exit */
#define PEX_READ        2       /* read a buffer of data when there is some */
#define PEX_WRITE       3       /* write a buffer to the net */
#define PEX_STATUS      4       /* say that there is data to read */

struct prot_msg
{
	Ushort    command;                /* action to be taken on this prot */
	Ushort    ret_code;               /* return value of specific protocol */
	Ushort    len;                    /* length */
	caddr_t data;                   /* and actual data */
	Ushort    low_bound;              /* low bound port/prot number */
	Ushort    high_bound;             /* upper bound */
	Ushort    ident;                  /* identifying value to host software*/
}; /* struct prot_msg */


/* the sub-id fields for the managers */
	/* the actual message structure can be found at the end */
	/* of this section                                      */

#define MAN_LANCE       0
#define MAN_ARP         1
#define MAN_IP          2
#define MAN_TCP_UDP     3
#define MAN_L5          4
#define MAN_PROT        5
#define MAN_LOGGING     6


/* the structure layouts for the different manager functions */

/* command layout for LANCE layer */
#define LANCE_SET_ADR   0       /* define ethernet address of ICC */
#define LANCE_GET_ADR   1       /* get the current ethernet address */
#define LANCE_CONNECT   2       /* connect the  board to ethernet */
#define LANCE_DISCONNECT 3      /* disconnect it from net */
#define LANCE_HW_INIT   4       /* init the ethernet hardware */
#define LANCE_GET_STAT  5       /* get the various statistics */
#define LANCE_LOG_OFF   6       /* turn message logging off */
#define LANCE_LOG_ON    7       /* turn it on */
#define LANCE_PROT_ON   8       /* use protocol exit for not IP/ARP packets */
#define LANCE_PROT_OFF  9       /* don't use it */

struct lance_mgmt_msg
{
	Uchar   ether_addr[6];  /* requested hardware-address */
	caddr_t stat_msg;       /* pointer to big statistics-msg */
}; /* struct lance_mgmt_msg */


/* command layout for ARP layer */
#define ARP_INIT        0       /* init the arp layer */
#define ARP_GET_ENTRY   1       /* get the current value for an IP-addr */
#define ARP_SET_ENTRY   2       /* set it */
#define ARP_KILL_ENTRY  3       /* kill it */
#define ARP_GET_STAT    4       /* get the statistic values */
#define ARP_LOG_OFF     5       /* logging off */
#define ARP_LOG_ON      6       /* logging on */
#define ARP_REQUEST_ENTRY 7     /* request explicit an entry */

struct arp_mgmt_msg
{
	Ulong ip_addr;          /* ip address of entry (even the own) */
	Uchar ether_addr[6];    /* corresponding ethernet address */
	Ushort  perm;             /* <> 0 : set permanent entry */
	caddr_t stat_msg;       /* pointer to statistics buffer */
}; /* struct arp_mgmt_msg */


/* command layout for IP layer */
#define IP_INIT         0       /* init IP */
#define IP_SET_ROUTE    1       /* set a routing entry */
#define IP_GET_ROUTE    2       /* get it */
#define IP_KILL_ROUTE   3       /* kill an entry */
#define IP_ICMP_ON      4       /* turn ICMP message generation on */
#define IP_ICMP_OFF     5       /* or off */
#define IP_LOG_ON       6       /* turn logging on */
#define IP_LOG_OFF      7       /* and off */
#define IP_GET_STAT     8       /* get statistics */
#define IP_PROT_ON      9       /* send all packets to protocol exit */
#define IP_PROT_OFF     10      /* use onboard TCP/UDP/ICMP */
#define IP_GET_ADDR     11      /* get IP-Address out of kernel-data */
#define IP_GET_NUMBER   12      /* get ICC-no out of kernel-data */

struct ip_mgmt_msg
{
	Ulong ip_addr;          /* own ip address (during init) or target's */
	Ulong path;             /* gateway for target */
	caddr_t stat_msg;       /* address of routing table or statistics */
}; /* struct ip_mgmt_msg */


/* command layout for TCP / UDP layer */
#define TCP_INIT        0       /* init TCP and UDP */
#define TCP_LOG_ON      1       /* turn logging on TCP/UDP */
#define TCP_LOG_OFF     2       /* an off */
#define TCP_CANCEL      3       /* cancel a connection */
#define TCP_FEATURE     4       /* control test functions */
#define TCP_GET_STAT    5       /* get statistics */
#define UDP_CANCEL      6       /* cancel a udp connection */
#define UDP_GET_STAT    7       /* get udpm statistics */

#define TCP_ECHO        0       /* echo all incoming data */
#define TCP_SINK        1       /* forget all incoming data */
#define TCP_TEXT        2       /* generate text */

struct tcp_udp_mgmt_msg
{
	char    test_function;  /* what test function */
	char    on_or_off;      /* turn on (==0) or off (!=0) */
	caddr_t stat_msg;       /* address of statistics message */
	Ushort  src_port;       /* tcp port of source */
	Ushort  dest_port;      /* tcp port of dest, or local conn name */
	Ulong   ip_addr;        /* ip address of target */
}; /* struct tcp_udp_mgmt_msg */

/* command layout for level 5 */
#define L5_INIT         0       /* init level 5 */
#define L5_GET_STAT     1       /* get statistics */
#define L5_RESET        2       /* reset the complete layer by reinitializing*/
#define L5_ABORT        3       /* connect debugger without error */


struct l5_mgmt_msg
{                                                                     
	caddr_t stat_msg;       /* address of statistics message */
	short   nr_sockets;     /* installable nr of sockets for the icc */
	short   buffer_size;    /* maximum size of packet between host/icc */
}; /* struct l5_mgmt_msg */


/* command layout for logging messages */

#define LOGGING_IND     0       /* indication from icc->host that there data */
#define LOGGING_REQ     1       /* request for data from host->icc */
#define LOGGING_RSP     2       /* resonse sent from icc->host */
#define LOGGING_CAN     3       /* cancel-message instead of rq/rsp */
#define LOGGING_ON      4       /* start-message from host->icc */
#define MEMDUMP_REQ     5       /* memory dump request */
#define MEMDUMP_RSP     6       /* answer to such a request */


struct log_mgmt_msg
{
	short   reader_no;      /* to which reader the message pertains */
	short   length;         /* length of data portion (request/resopnse */
	caddr_t buffer;         /* buffer address in host-address-space */
	caddr_t startaddr;      /* startaddress of a memory request */
	boolean more_flg;       /* if there is more data to read */
}; /* endstruct log_mgmt_msg */



/* command layout for protocol exits */
/* (currently mapped to calls on L5-Manager) */
#define PROT_EXIT       0       /* install a protocol exit */


struct mgmt_data
{
	Ushort msg_code; /* the desired function (op code) */
	Ushort ret_code; /* return code for desired function */
	union
	  {
		struct lance_mgmt_msg        lance_mgmt_msg;
		struct arp_mgmt_msg          arp_mgmt_msg;
		struct ip_mgmt_msg           ip_mgmt_msg;
		struct tcp_udp_mgmt_msg      tcp_udp_mgmt_msg;
		struct l5_mgmt_msg           l5_mgmt_msg;
		struct log_mgmt_msg          log_mgmt_msg;
	  } u;
}; /* struct mgmt_data */

/****************************************************************
*       Inter - Layer - Message - Formats                       *
****************************************************************/

/* inter layer messages have the id[1] field set, to allow demultiplexinig
   of different data streams.
   A this moment the only required usage is from TCP/UDP to Level5,
   because both protocols use the same port.
   Layers, where setting is done are marked. */

#define FROM_LANCE      0       
#define FROM_ARP        1       
#define FROM_IP         2
#define FROM_ICMP       3
#define FROM_TCP        4
#define FROM_UDP        5       /* uses setting */
#define FROM_L5         6
#define FROM_UNIX       7
#define FROM_PROT       8
#define FROM_MUNET      9

/* Message structure between L5 and the TCP/UDP layer */

/* TCP service primitives */

#define TCP_OPEN        0       /* open a connection */
#define TCP_SEND        1       /* send messages to destination port */
#define TCP_RECV        2       /* wait for a message to arrive */
#define TCP_CLOSE       3       /* close a connection */
#define TCP_ABORT       4       /* kill a connection */
#define TCP_STATUS      5       /* its obvious */

struct tcp_open_msg
{
	Ushort  local_port;     /* own tcp port number */
	Ushort  foreign_port;   /* port to connect to */
	Ulong   foreign_ip;     /* ip address of target */
	Uchar   active_conn;    /* is this an active connection request ? */
	Uchar   timeout_sec;    /* max. wait time (0 == default ) */
	Uchar   timeout_act;    /* 0 = report and continue, 1 = shutdown */
	/*Ushort    precedence;     /* see IP header: precedence field */
	/*struct secur *security; /* see IP header: security fields */
	/*struct tcpopt *options; /* ??? */
	/* rearrange this structure if more data is to be transported */
}; /* struct tcp_open_msg */


struct tcp_send_msg
{
	Ushort  byte_count;     /* length of data message */
	caddr_t msg;            /* start of Message in core mem */
	Ushort  bytes_done;     /* nr of bytes processed by TCP */
	Uchar   push_flg;       /* immediate send message */
	Uchar   urgent_flg;     /* this is real urgent data */
	Uchar   timeout_sec;    /* max. wait time (0 == default ) */
}; /* struct tcp_send_msg */


struct tcp_recv_msg
{
	Ushort    byte_count;     /* length of data msg (max. or effective) */
	struct  packet *msg;    /* start of Message in core mem */
	boolean push_flg;       /* immediate sent message */
	boolean urgent_flg;     /* this is real urgent data */
	boolean more_flg;       /* the read could have transported more data */
}; /* struct tcp_recv_msg */


struct tcp_close_msg
{
	Ushort    dummy_for_the_compiler; /* no comment */
}; /* struct tcp_close_msg */


struct tcp_status_msg
{
	Ushort  local_port;     /* own tcp port number */
	Ushort  foreign_port;   /* connected port */
	Ulong   foreign_ip;     /* ip address of target */
	Ushort  send_window;    /* send window size */
	Ushort  recv_window;    /* receive window size */
	Ushort  send_count;     /* no bytes waiting for xmit */
	Ushort  recv_count;     /* no bytes ready to forward to l5 */
	Uchar   timeout_sec;    /* max. wait time (0 == default ) */
	boolean urgent_rdy;     /* flag whether urgent data is ready */
	/* rearrange this structure if more data is to be transported */
}; /* struct tcp_status_msg */


struct tcp_abort_msg
{
	Ushort    dummy_for_the_compiler; /* no comment */
}; /* struct tcp_abort_msg */



struct tcp_msg
{
	Uchar   svc_prim;       /* one of the above TCP_xxx */
	Uchar   sequence_no;    /* internal nbr for reference purposes */
	Ushort    local_conn;     /* local connection name */
	Ushort  ret_code;       /* function return code */

	union
	  {
	    struct tcp_open_msg   tcp_open_msg;
	    struct tcp_send_msg   tcp_send_msg;
	    struct tcp_recv_msg   tcp_recv_msg;
	    struct tcp_close_msg  tcp_close_msg;
	    struct tcp_status_msg tcp_status_msg;
	    struct tcp_abort_msg  tcp_abort_msg;
	  } u; /* end union */
}; /* struct tcp_msg */

/* service functions for UDP */

#define UDP_OPEN        0       /* open a UDP receive port */
#define UDP_CLOSE       1       /* close a UDP receive port */
#define UDP_SEND        2       /* send a UDP frame */
#define UDP_RECV        3       /* and receive a frame */
#define UDP_STATUS      4       /* give status to level 5 */

struct udp_msg
{
	Ushort  ret_code;       /* return value */
	Ushort  conn;           /* connection identifier */
	Ushort  length;         /* number of available  */
	struct packet *packet;  /* pointer to data buffer */
	Ushort  src_port;       /* UDP source port field */
	Ushort  dest_port;      /* UDP destination port field */
	Ulong   ip_addr;        /* ip address */
	Uchar   svc_prim;       /* one of the above UDP_xxx */

}; /* struct udp_msg */


/****************************************************************
*       Internet - Protocol - Message - Formats                 *
****************************************************************/

/* Message structure between the TCP/UDP and IP layers */

#define IP_SEND         0       /* Message/Datagram send request */
#define IP_RECV         1       /* and the opposite direction */


/* IP service primitives */


struct ip_send_msg
{
	Uchar   type_of_svc;    /* throughput, reliability, precedence, ... */
	Uchar   time_to_live;   /* maximum life-time of a frame */
	Ushort    stream_id;      /* used within SATNET and the like */
	caddr_t options;        /* some ip options to use */
	Uchar   dont_fragment;  /* fragmentation isn't allowed for this pack */
}; /* struct ip_send_msg */


struct ip_recv_msg
{
	caddr_t options;        /* some ip options to use */
	Uchar   type_of_svc;    /* throughput, reliability, precedence, ... */
	Uchar   time_to_live;   /* max life time of a segment */
}; /* struct ip_recv_msg */


struct ip_msg
{
	Uchar   svc_prim;       /* one of the above IP_xxx */
	Uchar   sequence_no;    /* internal nbr for reference purposes */
	Ushort  ret_code;       /* return value of called primitive */
	struct packet *packet;  /* address of an ethernet buffer */
				/* within the buffer a pseudo header resides */
	union
	  {
	    struct ip_send_msg   ip_send_msg;
	    struct ip_recv_msg   ip_recv_msg;
	  } u; /* end union */

}; /* struct ip_msg */


/****************************************************************
*       Internet - Control - Message - Protocol - Requests      *
****************************************************************/

#define ICMP_SEND       0       /* a request to send a message */
#define ICMP_RECV       1       /* the indication of an incoming frame */

/* ICMP service primitives */

struct icmp_msg {
	Uchar   svc_prim;       /* one of the above IP_xxx */
	Uchar   sequence_no;    /* internal nbr for reference purposes */
	Ushort    ret_code;       /* return value of called primitive */
	Uchar   icmp_type;      /* type of icmp-msg to send */
	Uchar   icmp_code;      /* explanation of the above */
	union {
		Uchar  ptr;     /* pointer to erroneous byte */
		Ulong  ip_addr; /* ip address of gateway */
		Ushort id[2];   /* the parameters of en echo/id reply */
	} icmp;
	struct packet *pack;    /* msg to be handled/erroneous frame */
}; /* endstruct icmp_msg */


/****************************************************************
*       Address - Resolution - Protocol - Requests              *
****************************************************************/

/* Message structure between the IP and ARP/LANCE layers */

struct arp_msg
{
	Ushort   ret_code;       /* return value of called primitive */
	Ulong   destination;    /* internet address of target */
	struct  packet *packet; /* address of an ethernet buffer */
}; /* struct arp_msg */



/****************************************************************
*       Ethernet - Message - Formats                            *
****************************************************************/

/* Message structure used on ARP and LANCE layers */

#define ETH_PACKLEN     1504    /* size of ethernet data */


struct ethernet_msg
{
	Uchar   destination[6]; /* ethernet destination address */
	Uchar   source[6];      /* this sites ethernet address */
	Ushort    protocol_type;  /* only IP or ARP protocols allowed */
	Uchar   data[ETH_PACKLEN];
}; /* struct ethernet_msg */


struct packet
{
	Ushort  offset;         /* start of real data in this buffer */
	Ushort  length;         /* effective data length */
	struct ethernet_msg data;       /* placeholder for 1500+ bytes */
	Uchar   crc[4];         /* lance-chip writes crc back */
}; /* struct packet */

