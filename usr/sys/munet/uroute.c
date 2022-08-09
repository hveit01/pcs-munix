/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/munet/uroute.c ";

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

#define NROUTES 10

/* single routing entry */
struct rtentry {
    int off0;       /* 0-3*/
    int gwip;       /* gateway ip */
    int off8;       /* 8-b */
    enetaddr mac;   /* gateway mac */
    short flag;     /* =2 used */
};

/* routing table */
struct rtable {
    struct rtentry *rt_item;    /* entry element */
    int rt_id;                  /* id */
};

struct rtable rtable[NROUTES];

/* id == 0 -> default, id != 0 -> specific route */
/* get MAC address of routing entry */
uip_nroute(id, mac)
int id;
enetaddr *mac;
{
    register int i;
    
    
    for (i=0; i<NROUTES; i++) { /* get by id */
        if (rtable[i].rt_id == id && rtable[i].rt_item->flag==2) {
            *mac = rtable[i].rt_item->mac;
            return 1;
        }
    }
    for (i=0; i<NROUTES; i++) { /* get default route */
        if (rtable[i].rt_item->gwip == 0 && rtable[i].rt_item->flag==2) {
            *mac = rtable[i].rt_item->mac;
            return 1;
        }
    }
    return -1;
}

/* this seems to be broken as it accesses undefined memory (rt) 
 * OTOH - is not linked anywhere */
ugrtalloc(gwip, mac, id)
ipnetaddr gwip;
enetaddr mac;
ushort id;
{
    register int i;
    struct rtentry *rt;                 /* BUG: point to anywhere */
    
    if (id == 0) {                      /* will only set default route */
        for (i=0; i < 10; i++) {
            if (rtable[i].rt_id==0) {   /* find default entry */
                rt->gwip = gwip;        /* ??? */
                rt->gwip = 0;           /* ??? */
                rt->mac = mac;          /* ??? */
                rt->flag = 2;           /* ??? */
                rtable[i].rt_item = rt;
                rtable[i].rt_id = 1;
            }
        }
    }
    return -1;
}
