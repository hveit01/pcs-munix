/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.2  Nov 13 1986 /usr/sys/munet/mnnode.c ";

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

init_nodes()
{
    register int i;
    for (i=0; i < UIMAXNODES; i++)
        uiclosenode(i);                 /* close and deallocate all nodes */
}

/* open network node for remote address destip */
int uiopennode(dev, destip)
dev_t dev;
ipnetaddr destip;
{
    register struct uinode *np;
    
    for (np = uinode; np < &uinode[UIMAXNODES]; np++) {
        if (np->nd_dev == NODEV) {
            np->nd_dev = dev;           /* set device */
            np->nd_etaddr = noname;     /* set remote mac address = invalid */
            np->nd_ipaddr = destip;     /* set remote ip */
            np->nd_time = time;         /* set timestamp */
            np->nd_flag = 0;            /* internal flag */
            np->nd_vers = 0;            /* version of node */
            np->nd_timediff = 0;        /* time delta */
            uarp_resolve(np, destip, 0); /* resolve IP addr */
            return np - uinode;         /* return index */
        }
    }
    return -1;                          /* no free node found */
}

/* find node by device# 
 * return index, -1 if not found 
 */
int uifindnode1(dev)
register dev_t dev;
{
    register int i;
    for (i=0; i < UIMAXNODES; i++) {
        if (uinode[i].nd_dev >= 0 && uinode[i].nd_dev == dev)
            return i;
    }
    return -1;                          /* not found */
}

/* find node by ip address
 * return index -1 if not found 
 */
int uifindnode3(ip)
register ipnetaddr ip;
{
    register int i;
    for (i=0; i < UIMAXNODES; i++) {
        if (uinode[i].nd_dev >= 0 && uinode[i].nd_ipaddr == ip)
            return i;
    }
    return -1;                          /* not found */
}
