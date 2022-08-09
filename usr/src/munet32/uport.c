/* PCS */

#include <sys/types.h>
#include <sys/param.h>
#include <macros.h> /* only required because of (unused) references */
#include <sys/munet/munet.h>
#include <sys/munet/uisw.h>
#include <sys/munet/uport.h>
#include <sys/munet/uisw.h>


static char *_Version = "@(#) 2.1  Apr 08 1987 /usr/src/munet32/libmunet/uport.c";

short open_uport(portno, acc_port, acc_ipaddr, flag)
short portno;
short acc_port;
ipnetaddr acc_ipaddr;
short flag;
{
    struct upt_info upt;
    
    if (acc_port <= 0 && acc_port != ANYPORT)
        return -1;
    
    upt.portno = portno;
    upt.acc_port = acc_port;
    upt.acc_ipaddr = acc_ipaddr;
    upt.flag = flag;
    
    if (uidoit(&upt, UIGETPORT) == 0)
        return upt.portno;
    else
        return -1;
}

close_uport()
{
    uidoit(0, UICLOSEPORT);
}

int read_uport(data, rwcnt, node, fromport)
caddr_t data;
int rwcnt;
ipnetaddr *node;
short *fromport;
{
    register int cnt;
    struct uupacket pkt;
    
    if (rwcnt < 0)
        return -1;
    
    do {
        uidoit(&pkt, UIREAD);
    } while (pkt.uu_callno != UIIIPC);
    
    if (pkt.uu_rwcnt == 0)
        return 0;

    if (pkt.uu_rwcnt < 0 || pkt.uu_rwcnt > UIMAXDATA_OLD)
        return -1;
    
    cnt = rwcnt > pkt.uu_rwcnt ? pkt.uu_rwcnt : rwcnt;
    
    if (cnt > 0)
        bcopy(pkt.uu_data, data, cnt);

    *node = pkt.uu_node;
    *fromport = pkt.uu_fromport;
    return cnt;
}

int write_uport(data, rwcnt, node, toport, fromport)
caddr_t data;
int rwcnt;
int node;
short toport;
short fromport;
{
    struct uupacket pkt;

    if (rwcnt < 0)
        return -1;
    
    if (rwcnt > UIMAXDATA_OLD)
        rwcnt = UIMAXDATA_OLD;

    if (rwcnt > 0)
        bcopy(data, pkt.uu_data, rwcnt);
    
    pkt.uu_rwcnt = rwcnt;
    pkt.uu_node = node;
    pkt.uu_toport = toport;
    pkt.uu_fromport = fromport;
    pkt.uu_callno = UIIIPC;
    if (uidoit(&pkt, UIWRITE) < 0)
        return -1;
    else
        return rwcnt;
}
