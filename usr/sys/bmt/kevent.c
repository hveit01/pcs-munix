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
#include <sys/sel.h>
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

static char *_Version = "@(#) RELEASE: 6.6 87/04/10 /usr/sys/bmt/kevent.c";

extern BMT_DES *cbmt_des[NCBMT];
extern short colserver[];

static EVENT_MESSAGE *Request_event();


static RemoveEvents(dev, mask, when)
int dev;
register EventMask mask;
ulong when;
{
    register EVENT_MESSAGE *ep;
    register EVENT_MESSAGE *nextp;
    register EVENT_QUEUE *qp = &cbmt_des[dev]->queue;

    ep = &qp->first;
    while (ep->link != &qp->last) {
        nextp = ep->link;
        if (nextp->event.when > when) return;
        if ((mask & nextp->mask)) {
            ep->link = nextp->link;
            if (nextp == qp->last.link)
                qp->last.link = ep;
            nextp->link = qp->free;
            qp->free = nextp;
            continue;
        }
        ep = nextp;
    }
}

static EVENT_MESSAGE *ReceiveEvent(dev, mask, when)
int dev;
register EventMask mask;
ulong when;
{
    register EVENT_MESSAGE *ep;
    register EVENT_MESSAGE *nextp;
    register EVENT_QUEUE *qp = &cbmt_des[dev]->queue;
    ulong t;
    
    if (mask == 0)
        return &qp->last;

    for (t = lbolt + when; ;) {
        ep = &qp->first;
        nextp = ep->link;
        qp->last.mask = mask;
        while ((mask & nextp->mask) == 0) {
            ep = nextp;
            nextp = ep->link;
        }
        if (nextp->event.what) {
            ep->link = nextp->link;
            if (nextp == qp->last.link)
                qp->last.link = ep;
            return nextp;
        }
        qp->flag |= CLIENT_WAITING;
        if (when == 0) break;
        if (when == -1)
            sleep((caddr_t)&qp->pool_sem, NINTER-1);
        else if (t > lbolt) {
            /* BUG: T_sleep called with obscure arg - see t_sleep.c */ 
            if (T_sleep((caddr_t)&qp->pool_sem, (short)(t - lbolt)) != 0)
                break;
        } else 
            break;
    }
    return &qp->last;
}

static EVENT_MESSAGE *Look_for_Event(dev, mask, when)
int dev;
register EventMask mask;
ulong when;
{
    register EVENT_MESSAGE *ep;
    register EVENT_MESSAGE *nextp;
    register EVENT_QUEUE *qp = &cbmt_des[dev]->queue;
    ulong t;
    
    if (mask == 0)
        return &qp->last;

    for (t = lbolt + when; ;) {
        ep = &qp->first;
        nextp = ep->link;
        qp->last.mask = mask;
        while ((mask & nextp->mask) == 0) {
            ep = nextp;
            nextp = ep->link;
        }
        if (nextp->event.what)
            return nextp;
        
        qp->flag |= CLIENT_WAITING;
        if (when == 0) break;
        if (when == -1)
            sleep((caddr_t)&qp->pool_sem, NINTER-1);
        else if (t > lbolt) {
            /* BUG: T_sleep called with obscure arg - see t_sleep.c */ 
            if (T_sleep((caddr_t)&qp->pool_sem, (short)(t - lbolt)) != 0)
                break;
        } else 
            break;
    }
    return &qp->last;
}

GetEvent(dev, arg)
int dev;
EventRecord *arg; 
{
    register EVENT_MESSAGE *ep;
    EventMask mask;
    ulong when;

    mask = fulong(&arg->message);
    when = fulong(&arg->when);
    ep = ReceiveEvent(dev, mask, when);
    
    if (copyout(&ep->event, arg, sizeof(EventRecord)) != 0)
        u.u_error = EFAULT;
    if (ep->event.what)
        Release_event(dev, ep);
}

FlushGetEvent(dev, arg)
int dev;
EventRecord *arg;
{
    register EVENT_MESSAGE *ep;
    EventRecord tmp;

    if (copyin(arg, &tmp, sizeof(EventRecord)) != 0) {
        u.u_error = EFAULT;
        return;
    }

    RemoveEvents(dev, tmp.message, (ulong)-1);
    ep = ReceiveEvent(dev, *(int*)&tmp.where, tmp.when);

    if (copyout(&ep->event, arg, sizeof(EventRecord)) != 0)
        u.u_error = EFAULT;
    if (ep->event.what)
        Release_event(dev, ep);
}

EventAvail(dev, arg)
int dev;
EventRecord *arg;
{
    register EVENT_MESSAGE *ep;
    EventMask mask;
    ulong when;

    mask = fulong(&arg->message);
    when = fulong(&arg->when);
    ep = Look_for_Event(dev, mask, when);
    
    if (copyout(&ep->event, arg, sizeof(EventRecord)) != 0)
        u.u_error = EFAULT;
}

FlushEvents(dev, arg)
int dev;
struct { long mask, when; } *arg;
{
    register EVENT_QUEUE *qp = &cbmt_des[dev]->queue;
    
    if (qp->first.link == &qp->last)
        return;
    
    RemoveEvents(dev, fulong(&arg->mask), fulong(&arg->when));
}

PostEvent(dev, arg)
int dev;
EventRecord *arg;
{
    EVENT_QUEUE *qp = &cbmt_des[dev]->queue;
    EVENT_MESSAGE *ep;

    ep = Request_event(dev);
    if (copyin(arg, &ep->event, sizeof(EventRecord)) != 0) {
        u.u_error = EFAULT;
        Release_event(dev, ep);
        return;
    }
    ep->event = *arg;
    ep->mask = 1 << ep->event.what;
    qp->last.link->link = ep;
    ep->link = &qp->last;
    qp->last.link = ep;
    if (qp->flag & CLIENT_WAITING) {
        qp->flag &= ~CLIENT_WAITING;
        wakeup((caddr_t)&qp->pool_sem);
    }
    if (qp->flag & EVENT_SELECT) {
        qp->flag &= ~EVENT_SELECT;
        wakeup((caddr_t)&nselect);
    }
}

static Release_event(dev, ep)
int dev;
EVENT_MESSAGE *ep;
{
    EVENT_QUEUE *qp = &cbmt_des[dev]->queue;

    ep->link = qp->free;
    qp->free = ep;
}

static EVENT_MESSAGE *Request_event(dev)
dev_t dev;
{
    register EVENT_MESSAGE *ep;
    register EVENT_QUEUE *qp = &cbmt_des[dev]->queue;
    register EVENT_MESSAGE *ep2;
    
    if (qp->free == 0) {
        ep = qp->first.link;
        ep2 = &qp->first;
        do {
            if (ep->event.what == 12) {
                ep2->link = ep->link;
                if (ep->link == &qp->last)
                    qp->last.link = ep2;
                return ep;
            }
            ep2 = ep;
            ep = ep->link;
        } while (ep != &qp->last);
        ep = qp->first.link;
        qp->first.link = ep->link;
    } else {
        ep = qp->free;
        qp->free = qp->free->link;
    }
    return ep;
}

event_init(dev)
int dev;
{
    register EVENT_QUEUE *qp = &cbmt_des[dev]->queue;
    short i;
    
    qp->first.link = &qp->last;
    qp->last.link = &qp->first;
    qp->last.event.what = 0;
    qp->flag = 0;

    for (i=0; i < EPOOL_SIZE; i++)
        qp->pool[i].link = &qp->pool[i+1];
    qp->pool[EPOOL_SIZE-1].link = 0;
    qp->free = &qp->pool[0];
}

int cbip_sel(dev, rsel, wsel)
dev_t dev;
int rsel, wsel;
{
    register EVENT_QUEUE *qp;
    register int flag = 0;

    if (colserver[dev] == 0)
        return 0;

    qp = &cbmt_des[dev]->queue;
    if (rsel == -1)
        qp->flag &= ~EVENT_SELECT;
    else if (wsel) {
        if (qp->first.link != &qp->last)
            flag |= CLIENT_WAITING;
        else if (rsel)
            qp->flag |= CLIENT_WAITING|EVENT_SELECT;
    }
    return flag;
}
