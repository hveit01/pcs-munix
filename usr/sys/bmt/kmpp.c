/* PCS specific */

/* Event handling for CBIP devices */

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

#define CWS1
#include <sys/bmt/gdi.h>
#include <sys/bmt/gdisys.h>
#include <sys/bmt/rtk.h>
#include <sys/bmt/cbip.h>
#include <sys/bmt/device.h>
#include <sys/bmt/list.h>
#include <sys/bmt/EventMgr.h>
#include <sys/bmt/Layer.h>
#include <sys/bmt/font.h>
#include <sys/bmt/Window.h>

static char *_Version = "@(#) RELEASE: 6.3 87/04/06 /usr/sys/bmt/kmpp.c";

extern BMT_DES *cbmt_des[];


send_cbmt(dev, msg)
int dev;
register BMT_MESSAGE *msg;
{
    register BMT_PORT *port;
    register BMT_DES *bdp = cbmt_des[dev];
    
    port = &bdp->msgport;
    msg->flag &= ~REPLIED;
    msg->flag |= IN_QUEUE;
    msg->link = ENDLINK;
    if (port->first == ENDLINK) {
        port->first = port->last = msg;
        if (port->flag & SERVER_WAITING) {
            port->flag &= ~SERVER_WAITING;
            wakeup((caddr_t)port);
        }
    } else {
        port->last->link = msg;
        port->last = msg;
    }
}

send_driver_msg(dev, msg)
int dev;
register BMT_MESSAGE *msg;
{
    register BMT_PORT *port;
    register BMT_DES *bdp = cbmt_des[dev];
    
    port = &bdp->msgport;
    msg->flag &= ~REPLIED;
    msg->flag |= IN_QUEUE;

    if (port->first == ENDLINK) {
        msg->link = ENDLINK;
        port->first = port->last = msg;
        if (port->flag & SERVER_WAITING) {
            port->flag &= ~SERVER_WAITING;
            wakeup((caddr_t)port);
        }
    } else {
        msg->link = port->first;
        port->first = msg;
    }
}

BMT_MESSAGE *receive_cbmt(dev, timeout)
int dev;
int timeout;
{
    register BMT_PORT *port;
    register BMT_DES *bdp = cbmt_des[dev];
    register BMT_MESSAGE *mp;
    
    port = &bdp->msgport;
    while (port->first == ENDLINK) {
        port->flag |= SERVER_WAITING;
        if (timeout == -1)
            sleep((caddr_t)port, PBMT);
        else if (timeout > 0) {
            if (T_sleep((caddr_t)port, timeout))    /* BUG: 1st arg should be BMT_MESSAGE! */
                return 0;
        } else
            return 0;
    }

    mp = port->first;
    if (mp == 0) {
        printf("MPP Fatal Error 1 (link = 0 \n"); /* closing parenthesis missing */
        sleep((caddr_t)port, PBMT);
    }
    port->first = port->first->link;
    mp->link = 0;
    mp->flag &= ~IN_QUEUE;
    return mp;
}

reply_cbmt(dev, msg)
int dev; /*unused*/
BMT_MESSAGE *msg;
{
    msg->flag |= REPLIED;
    wakeup((caddr_t)msg);
}

wait_cbmt(msg, timeout)
BMT_MESSAGE *msg;
int timeout;
{
    while ((msg->flag & REPLIED) == 0) {
        if (timeout > 0) {
            if (T_sleep((caddr_t)msg, (ushort)timeout) != 0)
                return 1;
        } else if (timeout == -1)
            sleep((caddr_t)msg, PBMT);
        else
            return 1;
    }
    
    msg->flag &= ~REPLIED;
    return 0;
}

rpc_cbmt(dev, msg, timeout)
int dev;
BMT_MESSAGE *msg;
int timeout;
{
    send_cbmt(dev, msg);
    wait_cbmt(msg, timeout);
}

BMT_MESSAGE *request_cbmt(dev)
int dev;
{
    register BMT_MESSAGE *msg;
    register BMT_DES *bdp = cbmt_des[dev];

    while (bdp->free == 0) {
        bdp->pool_flag |= CLIENT_WAITING;
        bdp->col_proc->p_pri = PBMT;
        sleep((caddr_t)&bdp->pool_sem, PBMT);
    }
    msg = bdp->free;
    if (bdp->free == ENDLINK) {
        printf("MPP Fatal  Error 3 (link = - 1)\n");
        sleep((caddr_t)&bdp->pool_flag, PBMT);
    }
    bdp->free = bdp->free->link;
    return msg;
}

BMT_MESSAGE *get_msg(bdp)
register BMT_DES *bdp;
{
    register BMT_MESSAGE *msg = bdp->free;

    if (bdp->free)
        bdp->free = bdp->free->link;
    return msg;
}

release_cbmt(dev, msg)
int dev;
register BMT_MESSAGE *msg;
{
    register BMT_DES *bdp;
    
    if (msg->flag & LONG_ERROR)
        cbip_ackn(dev);
    else if (msg->flag & NORELEASE)
        return;
    else {
        bdp = cbmt_des[dev];
        msg->link = bdp->free;
        msg->flag = 0;
        bdp->free = msg;
        if (bdp->pool_flag & (TTY_WAITING|TAB_WAITING|KBD_WAITING|MOUSE_WAITING))
            proc_bmt_request(bdp);
    
        if (bdp->pool_flag & CLIENT_WAITING) {
            bdp->pool_flag &= ~CLIENT_WAITING;
            wakeup((caddr_t)&bdp->pool_sem);
        }
    }
}

LONG_BMT_MESSAGE *request_cbuf(dev)
int dev;
{
    register LONG_BMT_MESSAGE *msg = &cbmt_des[dev]->long_msg;

    while ((msg->flag & BUFFER_FREE) == 0) {
        msg->flag |= BUFFER_WANTED;
        sleep((caddr_t)msg, PBMT);
    }
    msg->flag = 0;
    return msg;
}

release_cbuf(dev)
int dev;
{
    register LONG_BMT_MESSAGE *msg = &cbmt_des[dev]->long_msg;
    
    msg->flag |= BUFFER_FREE;
    if (msg->flag & BUFFER_WANTED)
        wakeup((caddr_t)msg);
}

mpp_init(dev)
int dev;
{
    register BMT_DES *bdp = cbmt_des[dev];
    register BMT_MESSAGE *mp, *mp2;
    
    int s = currpl();
    spltty();
    
    bdp->bmtid = dev;
    bdp->msgport.first = bdp->msgport.last = ENDLINK;
    bdp->pool_flag = 0;
    mp = &bdp->msg_pool[0];
    bdp->free = mp;
    
    mp2 = &bdp->msg_pool[POOL_SIZE-1]; 
    do {
        mp->link  = mp+1;
        mp++;
    } while (mp < mp2);
    mp2->link = 0;
    
    release_cbuf(dev);
    splx(s);
}

CSend(dev, arg)
int dev;
register BMT_MESSAGE *arg;
{
    register BMT_MESSAGE *msg = request_cbmt(dev);
    register LONG_BMT_MESSAGE *lmsg;

    if (copyin(&arg->sender, &msg->sender, 
            sizeof(BMT_MESSAGE)-sizeof(BMT_MESSAGE*)-sizeof(Semaphore)) != 0) {
        u.u_error = EFAULT;
        release_cbmt(dev, msg);
        return;
    }
    
    sulong(&arg->sender, msg);
    if (msg->flag & (BUFFER_FREE|BUFFER_WANTED)) {
        lmsg = request_cbuf(dev);
        lmsg->cnt = msg->data[2];
        lmsg->size = 0;
        lmsg->time = 0x7fff;
    }
    send_cbmt(dev, msg);
}

CLSend(dev, data)
int dev;
register caddr_t *data;
{
    register LONG_BMT_MESSAGE *lmsg = &cbmt_des[dev]->long_msg;

    if (lmsg->flag & BUFFER_FULL) {
        lmsg->flag |= PROD_WAITING;
        lmsg->time = 1000;
        sleep((caddr_t)&lmsg->cnt, PPORT);
    }
    lmsg->flag &= ~PROD_WAITING;
    lmsg->size = BUFF_SIZE;
    if (lmsg->size > lmsg->cnt)
        lmsg->size = lmsg->cnt;

    if (copyin(data, lmsg->data, lmsg->size) != 0) {
        lmsg->cnt = 0;
        lmsg->flag |= LONG_ERROR;
        lmsg->size = 0;
    }
    lmsg->flag |= BUFFER_FULL;
    if (lmsg->flag & CONS_WAITING)
        wakeup((caddr_t)lmsg->data);
    if (lmsg->flag & LONG_ERROR) {
        u.u_error = EFAULT;
        lmsg->cnt = 0;
    }
}

CLReceive(dev, arg)
int dev;
register caddr_t arg;
{
    register LONG_BMT_MESSAGE *lmsg = &cbmt_des[dev]->long_msg;
    
    if ((lmsg->flag & BUFFER_FULL) == 0) {
        lmsg->flag |= CONS_WAITING;
        lmsg->time = 1000;
        sleep((caddr_t)lmsg->data, PPORT);
    }
    lmsg->flag &= ~CONS_WAITING;
    if (copyout(lmsg->data, arg, lmsg->size) != 0) {
        printf("\n Copyout Error in CLReceive ");
        lmsg->flag |= LONG_ERROR;
    }

    lmsg->cnt -= lmsg->size;
    lmsg->flag &= ~BUFFER_FULL;
    if (lmsg->flag & PROD_WAITING)
        wakeup((caddr_t)&lmsg->cnt);
    
    if (lmsg->cnt <= 0)
        release_cbuf(dev);
    
    if (lmsg->flag & LONG_ERROR)
        u.u_error = EFAULT;
}

CReceive(dev, arg)
int dev;
caddr_t arg;
{
    register BMT_MESSAGE *msg = receive_cbmt(dev, fulong(arg)); /* in = timeout */
    
    if (msg == 0) {
        u.u_error = EIO;
        return;
    }
    
    msg->sender = msg;
    
    /* out = message data */
    if (copyout(&msg->sender, &(((BMT_MESSAGE*)arg)->sender),
            sizeof(BMT_MESSAGE) - sizeof(BMT_MESSAGE*) - sizeof(Semaphore)) != 0) {
        u.u_error = EFAULT;
        u.u_rval1 = 0;
    }
    release_cbmt(dev, msg);
}

CReply(dev, arg)
int dev;
register BMT_MESSAGE *arg;
{
    register BMT_MESSAGE *msg = (BMT_MESSAGE*)fulong(&arg->sender);
    
    if (copyin(&arg->flag, &msg->flag,
            sizeof(BMT_MESSAGE)-sizeof(MSG_HEADER)+sizeof(short)+2) != 0) {
        u.u_error = EFAULT;
        u.u_rval1 = 0;
    } else
        reply_cbmt(dev, arg->sender);
}

CWait_for(dev, arg)
dev_t dev;
register BMT_MESSAGE *arg;
{
    register BMT_MESSAGE *msg = (BMT_MESSAGE*)fulong(&arg->sender);

    if (wait_cbmt(msg, fulong(arg)) != 0) { /* arg->link is actually timeout */
        u.u_error = EIO;
        return;
    }
    
    if (copyout(msg->data, arg->data, sizeof(msg->data)) != 0) {
        u.u_error = EFAULT;
        u.u_rval1 = 0;
    }
    release_cbmt(dev, msg);
}

CBmt_rpc(dev, msg)
dev_t dev;
register BMT_MESSAGE *msg;
{
    BMT_MESSAGE *sender;
    
    suword(&msg->flag, CONS_WAITING);
    CSend(dev, msg);
    CWait_for(dev, msg);
    
    sender = (BMT_MESSAGE*)fulong(&msg->sender);
    
    sender->flag = 0;
    release_cbmt(dev, sender);
}

server_watch_dog(dev)
int dev;
{
    register LONG_BMT_MESSAGE *lmsg = &cbmt_des[dev]->long_msg;
    
    if (--lmsg->time == 0) {
        if (lmsg->flag & PROD_WAITING) {
            wakeup((caddr_t)&lmsg->cnt);
            printf("Time_out for Receiver \n");
            lmsg->flag |= LONG_ERROR;
            lmsg->flag &= ~PROD_WAITING;
            lmsg->cnt = 0;
            lmsg->time = 100;
        } else if (lmsg->flag & CONS_WAITING) {
            wakeup((caddr_t)lmsg->data);
            lmsg->size = 0;
            lmsg->flag |= LONG_ERROR;
            lmsg->flag &= ~CONS_WAITING;
            printf("Time_out for Sender \n");
            lmsg->time = 100;
        } else if ((lmsg->flag & BUFFER_FREE)==0) {
            lmsg->flag &= BUFFER_WANTED;
            release_cbuf(dev);
        } else
            lmsg->time = 0x7fff;
    }
}
