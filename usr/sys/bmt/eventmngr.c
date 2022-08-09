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
#include <sys/sel.h>
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

/* surprisingly, no _Version entry exists */

static int set_cur_pos;
static EVENT_MESSAGE *Request_event();

extern BMT_DES *bmt_des[];
extern short bmt_newmode[];

static RemoveEvents(dev, mask, when)
dev_t dev;
register EventMask mask;
ulong when;
{
    register EVENT_MESSAGE *ep, *ep2;
    register EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    
    ep = &qp->first;
    
    Lock(qp);
    while (ep->link != &qp->last) {
        ep2 = ep->link;
        if (ep2->event.when > when) break;
        if (mask & ep2->mask) {
            ep->link = ep2->link;
            if (ep2 == qp->last.link)
                qp->last.link = ep;
            ep2->link = qp->free;
            qp->free = ep2;
        } else
            ep = ep2;
    }
    Unlock(qp);
}

static EVENT_MESSAGE *ReceiveEvent(dev, mask, tm)
dev_t dev;
register EventMask mask;
long tm;
{
    register EVENT_MESSAGE *ep, *ep2;
    register EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    ulong when = lbolt + tm;
    short s;
    
    if (mask) {
        s = currpl();
        for (;;) {
            Lock(qp);
            ep = &qp->first;
            ep2 = ep->link;
            qp->last.mask = mask;
            while ((mask & ep2->mask) == 0) {
                ep = ep2;
                ep2 = ep2->link;
            }
            if (ep2->event.what) {
                ep->link = ep2->link;
                if (ep2 == qp->last.link)
                    qp->last.link = ep;
                Unlock(qp);
                splx(s);
                return ep2;
            }
            qp->qflag |= 1;
            Unlock(qp);
            if (tm == 0) 
                break;
            else if (tm == -1)
                sleep((caddr_t)&qp->pool_sem, PPORT+3);
            else if (when > lbolt) {
                if (T_sleep(&qp->pool_sem, (short)(when - lbolt)) != 0) 
                    break;
            } else
                break;
            /*continue*/
        }
        splx(s);
    }
    return &qp->last;
}

static EVENT_MESSAGE *Look_for_Event(dev, mask, tm)
dev_t dev;
register EventMask mask;
long tm;
{
    register EVENT_MESSAGE *ep, *ep2;
    ulong when;
    short s;
    
    register EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    when = lbolt + tm;
    
    if (mask == 0)
        return &qp->last;
        
    s = currpl();
    for (;;) {
        Lock(qp);
    
        ep = &qp->first;
        ep2 = ep->link;
        qp->last.mask = mask;
        while ((ep2->mask & mask) == 0) {
            ep = ep2;
            ep2 = ep2->link;
        }
        if (ep2->event.what) {
            Unlock(qp);
            splx(s);
            return ep2;
        }

        qp->qflag |= 1;
        Unlock(qp);
        if (tm == -1)
            sleep((caddr_t)&qp->pool_sem, PPORT+3);
        else if (tm == 0) 
            break;
        else if (when > lbolt) {
            if (T_sleep(&qp->pool_sem, (short)(when - lbolt)) != 0) 
                break;
        } else
            break;
        /*continue*/
    }
    splx(s);
    return &qp->last;
}

static GetEvent(dev, arg)
int dev;
EventRecord *arg;
{
    EventMask mask = fulong(&arg->message);
    long tm = fulong(&arg->when);
    register EVENT_MESSAGE *msg = ReceiveEvent(dev, mask, tm);

    if (copyout(&msg->event, arg, sizeof(EventRecord)) != 0)
        u.u_error = EFAULT;

    if (msg->event.what)
        Release_event(dev, msg);
}

static FlushGetEvent(dev, arg)
EventRecord *arg;
{
    register EVENT_MESSAGE *msg;
    EventRecord er;

    if (copyin(arg, &er, sizeof(EVENT_MESSAGE)) != 0) { /*BUG? should be sizeof(EventRecord)*/
        u.u_error = EFAULT;
        return;
    }

    RemoveEvents(dev, er.message, -1);
    msg = ReceiveEvent(dev, er.where, er.when);
    
    if (copyout(&msg->event, arg, sizeof(EventRecord)) != 0)
        u.u_error = EFAULT;
    
    if (msg->event.what)
        Release_event(dev, msg);
}

static EventAvail(dev, arg)
int dev;
EventRecord *arg;
{
    EventMask mask = fulong(&arg->message);
    long tm = fulong(&arg->when);
    register EVENT_MESSAGE *msg = Look_for_Event(dev, mask, tm);
    
    if (copyout(&msg->event, arg, sizeof(EventRecord)) != 0)
        u.u_error = EFAULT;
}

static FlushEvents(dev, arg)
int dev;
struct {
    EventMask mask;
    long when;
} *arg;
{
    register EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    
    if (qp->first.link == &qp->last)
        return;

    RemoveEvents(dev, fulong(&arg->mask), fulong(&arg->when));
}

static PostEvent(dev, arg)
int dev;
EventRecord *arg;
{
    EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    EVENT_MESSAGE *msg = Request_event(dev);
    
    if (copyin(arg, &msg->event, sizeof(EventRecord)) != 0) {
        u.u_error = EFAULT;
        Release_event(dev, msg);
        return;
    }

    msg->mask = 1 << msg->event.what;
    Lock(qp);
    qp->last.link->link = msg;
    msg->link = &qp->last;
    qp->last.link = msg;
    if (qp->qflag & 4) {
        qp->qflag &= ~4;
        wakeup((caddr_t)&nselect);
    }
    if (qp->qflag & 1) {
        qp->qflag &= ~1;
        Unlock(qp);
        wakeup((caddr_t)&qp->pool_sem);
        return;
    }
    Unlock(qp);
}

static Release_event(dev, msg)
int dev;
EVENT_MESSAGE *msg;
{
    EVENT_QUEUE *qp = &bmt_des[dev]->queue;

    Lock(qp);
    msg->link = qp->free;
    qp->free = msg;
    Unlock(qp);
}

static EVENT_MESSAGE *Request_event(dev)
dev_t dev;
{
    EVENT_MESSAGE *msg;
    EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    
    Lock(qp);
    if (qp->free == 0) {
        msg = qp->first.link;
        qp->first.link = msg->link;
    } else {
        msg = qp->free;
        qp->free = qp->free->link;
    }
    Unlock(qp);
    return msg;
}

InitEventQueue(dev)
int dev;
{
    register EVENT_QUEUE *qp = &bmt_des[dev]->queue;
    short i;
    
    qp->first.link = &qp->last;
    qp->last.link = &qp->first;

    qp->last.event.what = 0;
    qp->flag = qp->qflag = 0;
    
    for (i=0; i < EPOOL_SIZE; i++)
        qp->pool[i].link = &qp->pool[i+1];
    qp->pool[EPOOL_SIZE-1].link = 0;

    qp->pofifo = &bmt_des[dev]->bmt_intr;
    qp->free = &qp->pool[0];
}

EventMngr(dev, func, arg)
dev_t dev;
int func;
caddr_t arg;
{
    int unused;
    
    switch (func) {
    case GET_EVENT:
        GetEvent(dev, arg);
        break;

    case FLUSH_GET:
        FlushGetEvent(dev, arg);
        break;
    
    case EVENT_AVAIL:
        EventAvail(dev, arg);
        break;

    case POST_EVENT:
        PostEvent(dev, arg);
        break;

    case FLUSH_EVENT:
        FlushEvents(dev, arg);
        break;
    }
}

int bip_sel(dev, rsel, wsel)
dev_t dev;
int rsel, wsel;
{
    register EVENT_QUEUE *qp;
    register int sel = 0;

    if (bmt_newmode[dev] == 0) 
        return 0;

    qp = &bmt_des[dev]->queue;
    Lock(qp);

    if (rsel == -1)
        qp->qflag &= ~4;
    else if (wsel) {
        if (qp->first.link != &qp->last)
            sel |= 0x01;
        else if (rsel)
            qp->qflag |= 0x05;
    }
    Unlock(qp);
    return sel;
}
