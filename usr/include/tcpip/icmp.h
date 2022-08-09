/************************************************************************
*                             I C M P                                   *
*                                                                       *
* This include file describes the structure of an ICMP (Internet        *
* Control Message Protocol) packet header.                              *
* It also contains the necessary defines for the various (bit-)fields.  *
*                                                                       *
* change history:                                                       *
*       24/03/86 j&j    original version                                *
************************************************************************/


/* structure of an ICMP-Message */

struct icmppacket {
	Uchar   icmp_type;      /* message type, or opcode, if you prefer */
	Uchar   icmp_code;      /* explanation of above */
	Ushort  icmp_checksum;  /* checksum of complete icmp-packet */

	union {
		struct icmp_xxx {  /* if nothing else is appropriate */
		    long    icmp_unused;
		    char    icmp_original_msg;
		} icmp_xxx;

		struct icmp_problem { /* if problem report message */
		    Uchar   icmp_pointer;  /* pointer to questionable byte */
		    char    icmp_rsvd[3];
		    char    icmp_original_msg;
		} icmp_problem;

		struct icmp_redirect { /* if message could be redirected */
		    Ulong   icmp_internet_adr;  /* this host should be used */
		    char    icmp_original_msg;
		} icmp_redirect;

		struct icmp_echo {  /* if connection is tested */
		    Ushort    icmp_echo_id;    /* only for correllation */
		    Ushort    icmp_echo_seq;   /* ditto */
		    Uchar   icmp_echo_data[1];
		} icmp_echo;

		struct icmp_timestamp { /* if some host has lost its clock */
		    Ushort    icmp_time_id;
		    Ushort    icmp_time_seq;
		    Ulong   icmp_ori_time;
		    Ulong   icmp_rcv_time;
		    Ulong   icmp_xmt_time;
		} icmp_timestamp;

		struct icmp_info { /* find out the network-no the host is on */
		    Ushort    icmp_info_id;
		    Ushort    icmp_info_seq;
		} icmp_info;

	} icmp_u; /* endunion */

}; /* endstruct */


/* opcode for the various options/message types */

#define ICMP_ECHO_REPLY         0
#define ICMP_UNREACHABLE        3
#define ICMP_SRC_QUENCH         4
#define ICMP_REDIRECT           5
#define ICMP_ECHO               8
#define ICMP_TIME_EXCEED        11
#define ICMP_PARAM_PRBLM        12
#define ICMP_TIMESTAMP          13
#define ICMP_TIMESTAMP_REP      14
#define ICMP_INFO_RQ            15
#define ICMP_INFO_REPLY         16


/* type codes */

#define ICMP_NET_UNREACH        0
#define ICMP_HOST_UNREACH       1
#define ICMP_PROT_UNREACH       2
#define ICMP_PORT_UNREACH       3
#define ICMP_FRAG_UNREACH       4
#define ICMP_ROUT_UNREACH       5

#define ICMP_RED_NET            0
#define ICMP_RED_HOST           1
#define ICMP_RED_TOSN           2
#define ICMP_RED_TOSH           3

#define ICMP_TIME_OUT           0
#define ICMP_FRAG_OUT           1
