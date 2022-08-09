/*
 * include file for use of the protocol entry/exit features of
 * the TCP/IP software.
 *
 * Vers. 1.0   26 Mai 86        j&j
 */

#ifndef PROT_LANCE
#include "/usr/diplom/include/message.h"
#endif


#define TRAIL_HDR       /* we can handle trailing header ip messages */


/* type of EXIT to install */

#define LANCE_EXIT      PROT_LANCE
#define IP_EXIT         PROT_IP
#define IP_GATE_EXIT    PROT_IP_GATE


/* structure to install a protocol exit */

struct exit_struct {
	Ushort  ip_or_lance;
	Ushort  low_bound;              /* lower bound of exit-value range */
	Ushort  high_bound;             /* upper bound */
#ifdef ON_UNIX
	Ushort  ident;                  /* socket nr of installer */
	Ushort  link;                   /* link to next installed entry */
#endif
#ifdef ON_ICC
	Ushort  ident;                  /* id code from host */
#endif
}; /* struct exit_struct */
