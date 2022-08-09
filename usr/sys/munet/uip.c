/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.3  Feb 05 1987 /usr/sys/munet/uip.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include <sys/utsname.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/diskless.h>
#include <sys/munet/uarp.h>
#include "rbq.h"

extern short ipid;  /* defined in munet.c */

/* ARP request ethernet packet */
struct etharp {
    enetaddr dest;
    enetaddr source;
    unsigned short type;
    struct uarp_pkt arp;
};

/* IP packet header */
struct ethip {
/* Ethernet header */
    enetaddr e_dest;        /* destination Ethernet address */
    enetaddr e_source;      /* source Ethernet address */
    ushort e_type;          /* Ethernet packet type */
/* IP header */
    ushort ip_word1;        /* IP version, IHL, TOS values */
    ushort ip_len;          /* IP datagram total length value */
    ushort ip_id;           /* IP id from fragmentation (unused) */
    ushort ip_word4;        /* IP flags, and fragment offset */
    ushort ip_word5;        /* IP TTL and protocol */
    ushort ip_cksum;        /* IP simple checksum on header */
    ulong ip_srcaddr;       /* contains IP source address */
    ulong ip_destaddr;      /* contains IP destination address */
};

/* return 0 or errno */
int uarp_resolve(node, ipaddr, waitflg)
register struct uinode *node;   /* my node */
ipnetaddr ipaddr;               /* the requested IP */
int waitflg;                    /* =0 return immediately */
{
    struct etharp *p;
    unsigned short bufid;
    int ret;
    extern wakeup();

    if ((p = (struct etharp*)buf_get(node->nd_dev, UIMINPACKET,
                                     RT_HEADER_SIZE, &bufid))==0)
        return u.u_error;

    p->dest = bcname;
    p->type = ETHERPUP_ARPTYPE;
    p->arp.arp_hrd = ARP_HRD_E;
    p->arp.arp_pro = ARP_PRO_IP;
    p->arp.arp_hln = ARP_LEN_E;
    p->arp.arp_pln = ARP_LEN_IP;
    p->arp.arp_op  = ARP_OP_REQUEST;
    p->arp.arp_sha = utsname.uiname;
    p->arp.arp_spa = utsname.ipname;
    p->arp.arp_tpa = ipaddr;
    if ((ret = uiwrites(bufid, node->nd_ipaddr, ETHERPUP_ARPTYPE)) != 0)
        return ret;
    
    if (waitflg) {
        spldisk();
        timeout(wakeup, (caddr_t)&node->nd_ipaddr, hz*5);
        sleep((caddr_t)&node->nd_ipaddr, PZERO-1);
        if (node->nd_etaddr.lo==noname.lo &&
                node->nd_etaddr.mi==noname.mi &&
                node->nd_etaddr.hi==noname.hi) {
            spl0();
            return ENOUARP;
        }
        spl0();
        cancelto(wakeup, (caddr_t)&node->nd_ipaddr);
    }
    return 0;
}

/* uidx = index into uinode table */
int getether(uidx)
int uidx;
{
    if (uinode[uidx].nd_etaddr.lo==noname.lo &&
        uinode[uidx].nd_etaddr.mi == noname.mi &&
        uinode[uidx].nd_etaddr.hi == noname.hi)
            return uarp_resolve(&uinode[uidx], uinode[uidx].nd_ipaddr, 1);
    else
        return 0;
}

/* set IP parameters into a packet
*/
ip_fill(ip, destip, len, frag)
struct ethip *ip;
ipnetaddr destip;
int len;
ushort frag;
{
    ip->e_type = ETHERPUP_IPTYPE;
    ip->ip_word1 = 0x4558;              /* version 4, IHL=5, TOS=0x58 01011000
                                         * TOS: prec=000, delay=low, thru=high
                                         *      reliability=normal
                                         *      bit7-6 = 01 */
    ip->ip_len = len;                   /* IP length */

    INC_SSEQN(ipid);
    ip->ip_id = ipid;                   /* fragmentation ID */

    ip->ip_word4 = frag & 0xf000;       /* fragmentation flags */
    ip->ip_cksum = 0;                   /* clear checksum */
    ip->ip_word5 = 0x844;               /* TTL=8, PROTO=0x44 (distributed file system) */
    ip->ip_srcaddr = utsname.ipname;    /* my IP address */
    ip->ip_destaddr = destip;           /* target address */
    ip->ip_cksum =                      /* make header checksum */
        calc_check(&ip->ip_word1, (ip->ip_word1 & 0xf00) >> 6); /* IHL*4 bytes */
}

/* setup fragment packet (used after ip_fill) */
ip_fmore(ip, len, frag)
struct ethip *ip;
int len;
ushort frag;
{
    ip->ip_len = len;                   /* length */
    ip->ip_word4++;                     /* increment fragment offset */
    ip->ip_word4 = (frag & 0xf000) + (ip->ip_word4 & 0xfff);
    ip->ip_cksum = 0;
    ip->ip_cksum =                      /* make header checksum */
        calc_check(&ip->ip_word1, (ip->ip_word1 & 0xf00) >> 6); /* IHL*4 bytes */
}
