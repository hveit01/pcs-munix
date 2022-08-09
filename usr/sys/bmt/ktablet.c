/* PCS specific */

/* Event handling for CBIP devices */

#include <sys/types.h>
/*#include <sys/systm.h>*/
#include <sys/param.h>

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

static char *_Version = "@(#) RELEASE: 6.2 87/01/15 /usr/sys/bmt/ktablet.c";

static struct tabrec {
    short state;
    short event;
    Point pos;
    Point lastpos;
    short lastevent;
    short timeout;
    short sccdev;
    LocatorMsg *curmsg;
    short rcnt;
    char data[4];
    short xxx;
} tab_rec[NCBMT];

extern BMT_DES *cbmt_des[];


tab_rint(dev)
int dev;
{
    register struct tabrec *tp = &tab_rec[dev];
    int event;
    int rc;

    tp->state &= ~1;
    tp->timeout = 20;

    while (scc_ioctl(tp->sccdev, SCC_EMPTY) == 0) {
        rc = scc_ioctl(tp->sccdev, SCC_STATE);
        event = tp->data[tp->rcnt++] = scc_read(tp->sccdev);
        if (rc) {
            tp->rcnt = 0;
            tp->event = 0;
            continue;
        } else if (event & T_KEY_UP) {
            tp->event = event;
            tp->rcnt = 0;
            continue;
        } else if (tp->rcnt == 4 && (tp->event & T_KEY_UP)) {
            tp->event |= T_IDLE;
            tp->pos.x = (tp->data[0] & 0x3f) + ((tp->data[1] & 0x3f) << 6);
            tp->pos.y = (tp->data[2] & 0x3f) + ((tp->data[3] & 0x3f) << 6);
            if (tp->pos.x != tp->lastpos.x || tp->pos.y != tp->lastpos.y ||
                    tp->event != tp->lastevent) {
                tp->lastevent = tp->event;
                tp->lastpos = tp->pos;
                tab_send_msg(dev);
            }
            tp->event = 0;
            tp->rcnt = 0;
        } 
        if (tp->rcnt > 4) {
            tp->rcnt = 0;
            tp->event = 0;
        }
    }
}

tab_sint(tp)
register struct tabrec *tp;
{
    tp->rcnt = 0;
}

tab_watch_dog(dev)
int dev;
{
    register struct tabrec *tp = &tab_rec[dev];
    if ((tp->state & 1)==0 && tp->timeout-- < 0) {
        tp->state |= 1;
        tp->lastevent &= ~T_IDLE;
        tab_send_msg(dev);
    }
}

tab_ioctl(dev, func, arg)
int dev, func, arg;
{
    register struct tabrec *tp = &tab_rec[dev];
    switch (func) {
    case TC_STOP:
        scc_write(tp->sccdev, 'S');
        break;

    case TC_POINT:
        scc_write(tp->sccdev, 'P');
        break;
    
    case TC_STREAM:
        scc_write(tp->sccdev, arg);
        scc_ioctl(tp->sccdev, SCC_FLUSH);
        break;
    
    case TC_SWITCH:
        scc_write(tp->sccdev, arg);
        break;

    case TC_SPEED:
        scc_ioctl(tp->sccdev, SCC_SPEED, arg);
        break;
    
    case TC_FLUSH:
        scc_ioctl(tp->sccdev, SCC_FLUSH);
        break;
    }
}

tab_open(dev)
int dev;
{
    register struct tabrec *tp = &tab_rec[dev];
    
    tp->curmsg = 0;
    tp->rcnt = 0;
    tp->pos.x = 0;
    tp->pos.y = 0;
    tp->lastpos = tp->pos;
    tp->lastevent = 0;
    tp->state = 1;
    tp->sccdev = (dev << 1) + 1;
    scc_open(tp->sccdev);
    scc_ioctl(tp->sccdev, SCC_SPROC, tp, tab_sint);
    scc_ioctl(tp->sccdev, SCC_SPEED, 12);
    tab_ioctl(dev, TC_STREAM, 'K');
}

static tab_send_msg(dev)
int dev;
{
    register LocatorMsg *msg;
    register BMT_DES *bdp = cbmt_des[dev];
    register LocatorMsg *curmsg = tab_rec[dev].curmsg;
    
    if (curmsg && (curmsg->head.flag & IN_QUEUE) && curmsg->head.id[1] == TAB_MSG) {
        if (curmsg->event == tab_rec[dev].lastevent) {
            curmsg->pos = tab_rec[dev].pos;
            curmsg->time = bdp->TickCount;
            return;
        }
    }
    
    msg = (LocatorMsg*)get_msg(bdp);
    if (msg == 0) {
        tab_rec[dev].curmsg = 0;
        bdp->pool_flag |= TAB_WAITING;
        return;
    }
    
    msg->head.id[0] = 0;
    msg->head.id[1] = TAB_MSG;
    msg->pos = tab_rec[dev].pos;
    msg->event = tab_rec[dev].lastevent;
    msg->time = bdp->TickCount;
    tab_rec[dev].curmsg = msg;
    send_cbmt(dev, msg);
}

tab_request(dev)
int dev;
{
    register BMT_DES *bdp = cbmt_des[dev];
    register LocatorMsg *msg;
    
    msg = (LocatorMsg*)get_msg(bdp);
    msg->head.id[0] = 0;
    msg->head.id[1] = TAB_MSG;
    msg->pos = tab_rec[dev].pos;
    msg->event = tab_rec[dev].event;
    msg->time = bdp->TickCount;
    send_cbmt(dev, msg);
}
