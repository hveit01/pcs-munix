/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/munet/uip_icmp.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/var.h"
#include <sys/utsname.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/diskless.h>
#include <sys/munet/uarp.h>
#include "rbq.h"

struct {
    int unknown[20];
    int err;                            /* 80 protocol error? */
    int unknown2[2];
    int szerr;                          /* 92 packet size error? */
} icmpstat;

struct ethicmp {
/* Ethernet header */
    enetaddr e_dest;                    /* destination Ethernet address */
    enetaddr e_source;                  /* source Ethernet address */
    ushort e_type;                      /* Ethernet packet type */
/* IP header */
    ushort ip_word1;                    /* IP version, IHL, TOS values */
    ushort ip_len;                      /* IP datagram total length value */
    ushort ip_id;                       /* IP id from fragmentation (unused) */
    ushort ip_word4;                    /* IP flags, and fragment offset */
    ushort ip_word5;                    /* IP TTL and protocol */
    ushort ip_cksum;                    /* IP simple checksum on header */
    ulong ip_srcaddr;                   /* contains IP source address */
    ulong ip_destaddr;                  /* contains IP destination address */
/* ICMP header */
    struct icmp {
        unsigned char type;             /* type */
        unsigned char code;             /* code */
        ushort checksum;                /* checksum */
        union {
            struct iredir {
                ulong ipaddr;               /* new ipaddr */
                ushort ipwords[6];          /* original ip hdr */
                ulong ipsrc, ipdest;        /* original src, dest */
            } iredir;
            struct itime {
                ushort id, seq;             /* unused */
                uint otime, rtime, xtime;   /* time stamps */
            } itime;
        } u;
    } i;
    
};

/* process incoming ICMP packet */
uicmp_input(ipkt)
struct ethicmp *ipkt;
{
    struct icmp *ip;
    int code, len;

    len = ipkt->ip_len;
    if (len < 8) goto fail;

    ip = &ipkt->i;
    if (ip->type > 16) goto fail;
        
    code = ip->code;  
    switch (ip->type) {
    case 3:                         /* unreachable */
        if (code > 5)               /* >5: 13 = packet blocked */
            goto err;
        code += 8;                  /* translate to 8..13 */
        goto done;
    case 11:                        /* TTL exceeded */
        if (code > 1)               /* timeout while defragment */
            goto err;
        code += 18;                 /* translate to 18..19 */
        goto done;
    case 12:                        /* datagram param error? */
        if (code != 0)
            goto err;
        code = 20;                  /* translate to 20 */
        goto done;
    case 4:                         /* throttle send (obsolete) */
        if (code == 0) {
            code = 4;               /* translate to 4 */
            /* maybe debug code was here - disabled in production code */
done:       
            goto fail;
        }
err:    icmpstat.err++;             /* some error occurred */
        goto fail;
    case 8:                         /* echo request */
        ip->type = 0;               /* set to 0 */
        break;
    case 13:                        /* timestamp */
        if (len < 20) {
            icmpstat.szerr++;       /* packet too short */
            goto fail;
        }
        ip->type = 14;              /* timestamp reply */
        ip->u.itime.rtime = -1;     /* set rcv time */
        ip->u.itime.xtime = -1;     /* set xmt time */
        break;
    case 15:                        /* info request (obsolete, replaced by DHCP) */
        goto fail;
    case 0:                         /* echo reply */
    case 5:                         /* redirect */
    case 14:                        /* timestamp reply */
    case 16:                        /* info reply (obsolete) */
        if (len < 36) {
            icmpstat.szerr++;       /* packet too short */
            goto fail;
        } else 
        if (ip->type == 5)          /* redirect? */
            rtredirect(ip->u.iredir.ipdest, ip->u.iredir.ipaddr);
        goto fail;
    default:
        goto fail;
    }
    return 1;

fail:
    return 0;
}

/* handle ICMP redirect */
int rtredirect(iporg, ipnew)
ipnetaddr iporg, ipnew;
{
    enetaddr etaddr;
    int i;
    
    for (i=0; i < UIMAXNODES; i++) {
        if (uinode[i].nd_dev== -1) {    /* search for empty slot */
            uinode[i].nd_dev = -2;      /* mark as temporary used */
            uinode[i].nd_ipaddr = ipnew; /* set new ip addr */
            if (uarp_resolve(&uinode[i], ipnew, 1) != 0) { /* ARP resolve */
                uinode[i].nd_dev = -1;  /* not found, free tmp node */
                uinode[i].nd_ipaddr = 0;
                return -2;              /* redirect failed */
            } else {
                etaddr = uinode[i].nd_etaddr; /* save new mac address */
                uinode[i].nd_dev = -1;       /* free tmp node */
                uinode[i].nd_ipaddr = 0;
                for (i=0; i < UIMAXNODES; i++) { /* search original node */
                    if (uinode[i].nd_ipaddr == iporg)
                        uinode[i].nd_etaddr = etaddr; /* if found set new mac addr */
                }
                return 1;               /* successful redirect */
            }
        }
    }
    return -1;
}
