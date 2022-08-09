/* PCS specific */

/* Event handling for BMT devices */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/var.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/tty.h>
#include <sys/ttold.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/termio.h>
#include <sys/sysinfo.h>
#include <sys/cadmaus.h>

#include <sys/bmt/gdi.h>
#include <sys/bmt/gdisys.h>
#include <sys/bmt/rtk.h>
#include <sys/bmt/bmt.h>
#include <sys/bmt/device.h>
#include <sys/bmt/list.h>
#include <sys/bmt/EventMgr.h>
#include <sys/bmt/Layer.h>
#include <sys/bmt/font.h>
#include <sys/bmt/Window.h>

static char *_Version = "@(#) RELEASE: 6.6 87/04/06 /usr/sys/bmt/host_mpp.c";

extern BMT_DES *bmt_des[];
extern BMT_MESSAGE *request_bmt(), *receive_bmt();
extern Bip_device *bip_addr[];

#define RAMOFF (RAM_MAP*0x10000)

LONG_BMT_MESSAGE *request_buf(dev)
int dev;
{
    register LONG_BMT_MESSAGE *lmsg = &bmt_des[dev]->long_msg;
    int dummy;
    
    while ((lmsg->flag & BUFFER_FREE)==0) {
        lmsg->flag |= BUFFER_WANTED;
        sleep((caddr_t)lmsg, PPORT+3);
    }
    lmsg->flag = 0;
    return lmsg;
}

release_buf(dev)
int dev;
{
    register LONG_BMT_MESSAGE *lmsg = &bmt_des[dev]->long_msg;
    
    lmsg->flag |= BUFFER_FREE;
    if (lmsg->flag & BUFFER_WANTED)
        wakeup((caddr_t)lmsg);
}

Send(dev, arg)
int dev;
register BMT_MESSAGE *arg;
{
    register BMT_MESSAGE *msg = request_bmt(dev);
    register LONG_BMT_MESSAGE *lmsg;
    
    sulong(&arg->sender, msg);
    if (copyin(&arg->flag, &msg->flag, 
            sizeof(BMT_MESSAGE)-2*sizeof(BMT_MESSAGE*)-sizeof(Semaphore)) != 0) {
        u.u_error = EFAULT;
        release_bmt(dev, msg);
        printf("S_Error ");
        return;
    }
    
    if (msg->flag & (LONG_SEND|LONG_REQUEST)) {
        lmsg = &bmt_des[dev]->long_msg;
        lmsg = request_buf(dev);
        lmsg->cnt = msg->data[2];
        lmsg->src = (char*)msg->data[0];
        lmsg->dst = (char*)msg->data[1];
        lmsg->size = 0;
        send_bmt(dev, msg);
        sleep((caddr_t)&lmsg->cnt, PPORT+3);
    } else
        send_bmt(dev, msg);
}

LSend(dev, arg)
int dev;
register char *arg;
{
    register LONG_BMT_MESSAGE *lmsg = &bmt_des[dev]->long_msg;
    
    lmsg->size = BUFF_SIZE;
    if (lmsg->size > lmsg->cnt)
        lmsg->size = lmsg->cnt;

    if (bmtcopy_in(dev, arg, lmsg->dst, lmsg->size) != 0) {
        lmsg->cnt = 0;
        lmsg->flag |= LONG_ERROR;
        lmsg->size = 0;
        printf("LS_Error ");
    }
    
    lmsg->cnt -= lmsg->size;
    lmsg->dst += lmsg->size;
    if (lmsg->cnt <= 0) {
        Signal_bmt(&bmt_des[dev]->bmt_intr, F_RECEIVE_READY);
        release_buf(dev);
    }
}

LReceive(dev, arg)
int dev;
register caddr_t arg;
{

    register LONG_BMT_MESSAGE *lmsg = &bmt_des[dev]->long_msg;
    
    lmsg->size = BUFF_SIZE;
    if (lmsg->size > lmsg->cnt)
        lmsg->size = lmsg->cnt;

    if (bmtcopy_out(dev, lmsg->src, arg, lmsg->size) != 0)
        lmsg->flag |= LONG_ERROR;

    lmsg->cnt -= lmsg->size;
    lmsg->src += lmsg->size;
    if (lmsg->cnt <= 0) {
        Signal_bmt(&bmt_des[dev]->bmt_intr, F_SEND_READY);
        release_buf(dev);
    }
    
    if (lmsg->flag & LONG_ERROR)
        u.u_error = EFAULT;
}

Receive(dev, arg)
int dev;
BMT_MESSAGE *arg;
{
    register BMT_MESSAGE *msg;

    msg = receive_bmt(dev, fulong(&arg->link)); /* link field is actually timeout */
    if (!msg) {
        u.u_error = EIO;
        return;
    }

    if (copyout(&msg->flag, &arg->flag,
            sizeof(BMT_MESSAGE)-2*sizeof(BMT_MESSAGE*)-sizeof(Semaphore)) != 0) {
        u.u_error = EFAULT;
        u.u_rval1 = 0;
        release_bmt(dev, msg);
    } else {
        arg->sender = msg;
        if ((msg->flag & NORELEASE) == 0)
            release_bmt(dev, msg);
    }
}

Reply(dev, arg)
int dev;
register BMT_MESSAGE *arg;
{
    BMT_MESSAGE *msg = (BMT_MESSAGE*)fulong(&arg->sender);

    if (msg->link) {
        u.u_error = EIO;
        return;
    }
    
    if (copyin(arg->data, arg->sender->data, sizeof(arg->data)) != 0) {
        u.u_error = EFAULT;
        u.u_rval1 = 0;
    } else
        reply_bmt(dev, msg);
}

Wait_for(dev, msg)
dev_t dev;
register BMT_MESSAGE *msg;
{
    register BMT_MESSAGE *sender = (BMT_MESSAGE*)fulong(&msg->sender);
    if (wait_bmt(sender, fulong(&msg->link)) != 0) { /* link is timeout */
        u.u_error = EIO;
        return;
    }

    if (copyout(sender->data, msg->data, sizeof(sender->data)) != 0) {
        u.u_error = EFAULT;
        u.u_rval1 = 0;
    }
    
    release_bmt(dev, sender);
}

Bmt_rpc(dev, msg)
dev_t dev;
register BMT_MESSAGE *msg;
{
    Send(dev, msg);
    Wait_for(dev, msg);
}

/* same as copy_in from bmt_cpmem.c, but with ulong instead of char* */
bmtcopy_in(dev, from, to, cnt)
int dev;
register ulong from, to;
int cnt;
{
    register ulong p;
    ulong bip = (ulong)bip_addr[dev];
    int end, sz;

    for (end = to + cnt; to < end; from += sz, to += sz) {
        ((Bip_device*)bip)->map0 = to >> 17;
        p = (to & -RAMOFF) + RAMOFF;
        if (p < end)
            sz = p - to;
        else
            sz = end - to;

        if (copyin(from, bip + (to & (RAMOFF-1)), sz) != 0) {
            printf(" Copy_in Error \n");
            return -1;
        }
    }
    return 0;
}

/* same as copy_in from bmt_cpmem.c, but with ulong instead of char* */
bmtcopy_out(dev, from, to, cnt)
int dev;
ulong from;
ulong to;
int cnt;
{
    register ulong p;
    ulong bip = (ulong)bip_addr[dev];
    int sz;
    register ulong end = from;
    
    for (end += cnt; from < end; from += sz, to += sz) {
        ((Bip_device*)bip)->map0 = from >> 17;
        p = (from & -RAMOFF) + RAMOFF;
        if (end > p)
            sz = p - from;
        else
            sz = end - from;

        if (copyout(bip + (from & (RAMOFF-1)), to, sz) != 0)
            return -1;
    }
    return 0;
}
