/************************************************************************
*                               A R P                                   *
*                                                                       *
* This include file describes the structure of an arp packet for the    *
* ethernet. It contains also the necessary defines for the ARP packet   *
* types.                                                                *
*                                                                       *
* change history:                                                       *
*       28/04/86 jh                                                     *
************************************************************************/

#define ARP_PACKET      0x0806

#define ARP_REQUEST     1
#define ARP_REPLY       2

#define ARP_HW_ETHER    1

#define ARP_TABLE_LEN   50      /* number of entries in table */

#define TEMP    1               /* entry may be killed */
#define PREV    0               /* if new entry as TEMP, else do not change */
#define PERM    -1              /* entry may not be killed */

#define LIFETIME 20             /* max time an entry is considered used */

#define SLEEP_VALUE     5000L   /* sleep about 5 seconds */

struct  arppacket {
	short   arp_hrd;     /* hardware type */
	short   arp_prot;    /* what protocol ? */
	char    arp_hln;     /* hardware address length */
	char    arp_pln;     /* protocol address length */
	short   arp_opc;     /* opcode of arp packet */
	char    hw_adr[6];   /* only ethernet supported */
	Ulong   pt_adr;      /* protocoladdr of sender */
	char    hw_adr_targ[6]; /* target if known */
	Ulong   pt_adr_targ;    /* target */
	char    pad;            /* for padding to minimun ethernet length */
}; /* struct arppacket */

struct arpentry
{
	short   flag;           /* 0 = free, 1= temp, FF = permanent */
	short   age;            /* the age of this entry */
	Ulong   ip_addr;        /* the ip address */
	char    ether_addr[6];  /* and the corresponding ethernet address */
}; /* struct arpentry */
