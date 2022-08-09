/* PCS specific */
static char *_Version = "@(#) RELEASE:  2.3  Nov 02 1987 /usr/sys/io/icc.c";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/page.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/icc/unix_icc.h"

ushort *icc_base[] = {
    (ushort*)ICC0_BASE,
    (ushort*)ICC1_BASE,
    (ushort*)ICC2_BASE,
    (ushort*)ICC3_BASE
};

ICC_DES icc_des[4];

extern int nodev(), iw_intr(), if_intr(), is_intr(), 
           lance_intr(), scc_intr(), print_intr(), 
           socket_intr(), prot_intr(), mgmt_intr();

int (*sig_proc[11])() = { nodev, iw_intr, if_intr, is_intr,
                          lance_intr, scc_intr, scc_intr, print_intr,
                          socket_intr, prot_intr, mgmt_intr };

int icc_initflg[] = {
    0, 0, 0, 0
};

extern short *scc_addr;
extern short cr2image;
extern int icc_noprint;
extern Port_waker();


send_to_icc(ip)
register ICC_DES *ip;
{
    register UNIX_MESSAGE *up;

    if (ip->first == (UNIX_MESSAGE*)-1)
        return;

    up = ip->first;
    ip->first = ip->first->link;
    up->link = 0;

    ip->tr.sender = (up->flag & SEND) ? (long)up : up->sender;
    ip->tr.flag = up->flag;
    ip->tr.id[0] = up->id[0];
    ip->tr.id[1] = up->id[1];
    ip->tr.data[0] = up->data[0];
    ip->tr.data[1] = up->data[1];
    ip->tr.data[2] = up->data[2];
    ip->tr.data[3] = up->data[3];
    ip->tr.data[4] = up->data[4];
    ip->tr.data[5] = up->data[5];
    ip->tr_empty = 0;
    ip->tr_val = ~ ip->tr_val;
    *ip->icc_cr0 = ip->tr_val;
    *ip->icc_setint = ICC_INTR;
}

icc_xint(dev)
{
    register ICC_DES *ip;

    clrcache();
    ip = &icc_des[dev];
    ip->tr_empty = 1;
    send_to_icc(ip);
}

icc_rint(dev)
{
    register ICC_DES *ip;
    register UNIX_MESSAGE *up;
    register int id;
    
    clrcache();
    
    ip = &icc_des[dev];
    if (ip->rr.flag & ICC_SIG) {
        id = ip->rr.id[0];
        if (id <= 0 || id >= (uint)11)
            panic("ICC: invalid id[0]");
        
        (*sig_proc[id])(dev, ip);
        
        if (ip->owner_flag)
            ip->owner_flag |= ICC_OWNER_FLAG;
        else {
            ip->rr_val = ~ip->rr_val;
            *ip->icc_cr1 = ip->rr_val;
            *ip->icc_setint = ICC_INTR;
        }
    } else {
        up = (UNIX_MESSAGE*)ip->rr.sender;
        up->flag |= REPLIED;
        up->link = 0;
        up->id[0] = ip->rr.id[0];
        up->id[1] = ip->rr.id[1];
        up->data[0] = ip->rr.data[0];
        up->data[1] = ip->rr.data[1];
        up->data[2] = ip->rr.data[2];
        up->data[3] = ip->rr.data[3];
        up->data[4] = ip->rr.data[4];
        up->data[5] = ip->rr.data[5];
        if (ip->owner_flag)
            ip->owner_flag |= ICC_OWNER_FLAG;
        else {
            ip->rr_val = ~ip->rr_val;
            *ip->icc_cr1 = ip->rr_val;
            *ip->icc_setint = ICC_INTR;
        }
        
        wakeup(up);
    }
}

icc_init_done(dev)
{
    register ABORT_MESSAGE *ap;
    
    ap = &icc_des[dev].abo_msg;
    if (ap->magic == (short)ICC_ABORT) {
        printf("\nICC%d aborted: code = %x, pc = %X\n",
            dev, ap->code, ap->pc);
        ap->magic = 0;
    } else {
        icc_initflg[dev]++;
    }
    
    if (icc_des[dev].owner_flag == OWNER_FLAG)
        icc_des[dev].owner_flag |= ICC_OWNER_FLAG;
}

icc_init(dev)
{
    register ICC_DES *ip;
    ulong paddr;
    unsigned short *base;
    int i;

    if (icc_initflg[dev] > 0)
        return;

    if (icc_there(dev)==0)
        return;
        
    base = icc_base[dev];
    
    ip = &icc_des[dev];
    ip->first = ip->last = (UNIX_MESSAGE*)-1;
    ip->icc_cr0 =    (short*)base;
    ip->icc_cr1 =    (short*)base+1;
    ip->icc_cr2 =    (short*)base+2;
    ip->icc_setint = (short*)base+3;
    ip->rr.flag = 0;
    ip->tr.flag = 0;
    paddr = logtophys(ip);
    ip->tr_val = (paddr >> 16) & 0xff;
    ip->rr_val = (paddr >> 8)  & 0xff;
    
    if (dev==0) {
        scc_addr = (short*)&ip->con_reg;
        cr2image = paddr & 0xff;
    }
    
    *ip->icc_cr2 = paddr & 0xff;
    *ip->icc_cr0 = ip->tr_val;
    *ip->icc_cr1 = ip->rr_val;
    ip->tr_empty = 0;
    ip->tr.flag = RECEIVE;
    
    icc_des[dev].owner_flag = 0;
    *ip->icc_setint = ICC_INIT_VERS;
    icc_noprint++;
    printf("Waiting for icc...");
    for (i = 40; --i >= 0 && icc_initflg[dev]==0; )
        delay(hz >> 1);
    if (i >= 0)
        icc_noprint = 0;
    if (i < 0)
        printf("\nICC%d NOT initialized\n", dev);
    else
        printf("ICC%d initialized: %s\n", dev, ip->version);
}

com_send(dev, up)
register UNIX_MESSAGE *up;
{
    register ICC_DES *ip;
    int s;
    
    if (up->link)
        return 1;

    s = spldisk();
    up->link = (UNIX_MESSAGE*)-1;
    
    ip = &icc_des[dev];
    if (ip->first == (UNIX_MESSAGE*)-1)
        ip->first = ip->last = up;
    else {
        ip->last->link = up;
        ip->last = up;
    }
    
    if (ip->tr_empty)
        send_to_icc(ip);

    splx(s);
    return 0;
}

Send_icc(dev, up)
RTK_HEADER *up;
{
    up->flag = SEND;
    com_send(dev, up);
}

Reply_icc(dev, up)
RTK_HEADER *up;
{
    up->flag = REPLY;
    com_send(dev, up);
}

Wait_for_reply(up, tmout)
RTK_HEADER *up;
{
    int ret;
    int s = spldisk();
    
    ret = 0;
    if ((up->flag & REPLIED)==0) {
        if (tmout > 0)
            ret = Time_sleep(up, tmout);
        else
            sleep(up, PZERO);
    }
    splx(s);
    return ret;
}

Rpc(dev, id0, id1, up, tmout)
/*char id0, id1;*/
RTK_HEADER *up;
{
    int ret;

    up->id[0] = id0;
    up->id[1] = id1;
    ret = Send_icc(dev, up);
    if (tmout)
        ret = Wait_for_reply(up, tmout);
    return ret;
}

Time_sleep(up, tmout)
RTK_HEADER *up;
{
    if (tmout==0)
        return 1;

    up->flag &= ~TIME_OUT;
    cancelto(Port_waker, up);
    timeout(Port_waker, up, tmout);
    sleep(up, PZERO);
    return (up->flag & TIME_OUT) ? 1 : 0;
}

Port_waker(up)
UNIX_MESSAGE *up;
{
    up->flag |= TIME_OUT;
    wakeup(up);
}

icc_there(devno)
{
    if (icc_initflg[devno] > 0)
        return 1;
    
    if (ssword(icc_base[devno], 0) == -1)
        return 0;
    
    return 1;
}

/*
 * This is to be called with the address of the "abo_msg" item within
 * the ICC_DES struct. This will skip over the ABORT_MESSAGE and
 * print the "version" element. CAVEAT!
 */
print_intr(dev, p)
char *p;
{
        printf("error in ICC%d: %s\n", dev, 
            p+sizeof(ABORT_MESSAGE)+sizeof(UNIX_MESSAGE*));
}
