/************************************************************************
*                               I P                                     *
*                                                                       *
* This include file describes the structure of an IP (Internet Protocol)*
* packet header.                                                        *
* It also contains the necessary defines for the various (bit-)fields.  *
*                                                                       *
* change history:                                                       *
*       21/03/86 j&j    original version                                *
*       05/05/86 jh     added IP_VERSION,IP_MIN_LENGTH                  *
*       06/05/86 jh     added routing table entries                     *
************************************************************************/


#define IP_PACKET 0x0800        /* packet type for ethernet frames */

#define IP_ICMP_PROTOCOL   1    /* protocol field contents means ICMP */
#define IP_TCP_PROTOCOL    6    /* at least we hope so */
#define IP_UDP_PROTOCOL    17   /* it is so */
#define IP_MUNET_PROTOCOL  0x44 /* Munet Packets */

#define IP_VERSION      4       /* version of current ip header */
#define IP_MIN_LENGTH   5       /* minimun length of an ip header */


/* structure of an IP message */

struct ippacket {
	Uchar   ip_version_IHL; /* versionnumber and Header-Length */
	Uchar   ip_type_of_svc; /* Type of Service (various bits) */
	Ushort  ip_total_len;   /* nr of longwords in IP header */
	Ushort  ip_ident;       /* assembly/disassembly help */
	Ushort  ip_flags_offset;/* fragmentation flags and location */
	Uchar   ip_time_to_live;/* downcounter until selfdestruct */
	Uchar   ip_protocol;    /* next level's protocol */
	Ushort  ip_hdr_checksum;/* Checksum of ip Header */
	Ulong   ip_src_adr;     /* originator's internet address */
	Ulong   ip_dest_adr;    /* destination's internet address */
	char    ip_option_begin;/* here may the options start */
	};


/* structure of the possible options */

struct ipoptions {
	Uchar           ip_option;      /* opcode for option */
	Uchar           ip_opt_len;     /* if necessary: option length */
	union {
		struct ip_security {    /* if securityoptions */
			Ushort  ip_secur;       /* security level */
			Ushort  ip_compart;     /* compartment fields */
			Ushort  ip_restrict;    /* handling restrictions */
			Uchar   ip_TCC[3];      /* transmission Control code */
		} ip_security;

		struct ip_route {       /* if routing-info */
			Uchar   ip_route_ptr;   /* offset into the next field*/
			Ulong   ip_route_adr[1]; /* 4 bytes per inet address*/
		} ip_route;

		Ushort  ip_stream;      /* if sattellite SATNET identifier */

		struct ip_timestamp {   /* if time recording is enabled */
			Uchar   ip_time_ptr;    /* offset into data field */
			Uchar   ip_oflow_flag;  /* overflow and misc. flags */
			Ulong   ip_inet_adr;    /* the stamped inet-Address */
			Ulong   ip_timestamp;   /* local time, not systemwide*/
				/* the last two fields are repeated at taste */
		} ip_timestamp;
	} ip_u; /* endunion */

}; /* endstruct ipoptions */

struct pseudoheader
{
	Ulong src;              /* ip source address */
	Ulong dest;             /* ip destination address */
	char zero;              /* reserved,0 */
	char ptcl;              /* protocol value */
	Ushort length;          /* real data length */
}; /* struct pseudoheader */


#ifdef TRAIL_HDR

#define IP_TRAIL512     0x1001  /* 512 bytes of trailing header */
#define IP_TRAIL1024    0x1002  /* 1024 bytes of trailing header */

struct trail_hdr {
	Ushort  ptcl;           /* protocol field of ip message */
	Ushort  length;         /* length of header msg that follows */
	char    headers;        /* bytes for the headers */
}; /* endstruct trail_hdr */

#endif



struct ip_entry
{
	char flag;              /* used ? */
	Ulong ip_addr;          /* the desired ip */
	Ulong path;             /* the gateway to this ip */
}; /* end struct ip_entry */

/* some convenient shorthands to access bitfields */

#define ip_ip_version      ((ip->ip_version_IHL >> 4) & 0xF)
#define ip_ip_IHL          (ip->ip_version_IHL & 0xF)
#define ip_ip_precedence   ((ip->ip_type_of_svc >> 5) & 0x7)
#define ip_ip_delay        (ip->ip_type_of_svc & 0x10)
#define ip_ip_throughput   (ip->ip_type_of_svc & 0x08)
#define ip_ip_reliability  (ip->ip_type_of_svc & 0x04)
#define ip_ip_dont_fragm   (ip->ip_flags_offset & 0x4000)
#define ip_ip_more_fragm   (ip->ip_flags_offset & 0x2000)
#define ip_ip_fragm_offset (ip->ip_flags_offset & 0x1FFF)
#define ip_ip_time_oflow   ((ip->ip_oflow_flag >> 4) & 0xF)
#define ip_ip_time_flags   (ip->ip_oflow_flag & 0xF)


/* opcode for the various options */

#define IP_ENDOFLIST    0
#define IP_NOOP         1
#define IP_RECORD_ROUTE 7
#define IP_TIMESTAMP    68
#define IP_SECURITY     130
#define IP_LOOSE_ROUTE  131
#define IP_STREAM_ID    136
#define IP_STRICT_ROUTE 137

