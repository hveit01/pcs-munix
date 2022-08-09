/* PCS specific */

/* Event handling for CBIP devices */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>

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

static char *_Version = "@(#) RELEASE: 6.1 86/11/28 /usr/sys/bmt/kmouse.c";

long cbipdev[NCBMT] = { 0, 0 };
long clt_base[NCBMT] = { 0, 0 };
static struct mdescr {
    short open;
    short event;
    short buttons;
    short change;
    Point pos;
    LocatorMsg *curmsg;
} mouse[NCBMT];
static struct dev_mouse *mice_reg[NCBMT];
static struct dev_mon *mont_reg[NCBMT];
static struct dev_intr *intr_reg[NCBMT];

extern BMT_DES *cbmt_des[];
extern unsigned long cbip_io_base[];
extern unsigned long cbip_intr_vec[];
extern short colserver[];
extern EVENT_MESSAGE *get_msg();

vsync_int()
{
    register struct dev_mouse *mdev;
    register struct mdescr *mousep;
    register int i = 0;
    int s = currpl();

    spltty();

    while (i < 2) {
        if (colserver[i] == 0) {
            i++;
            continue;
        }
        
        mdev = mice_reg[i];
        mousep =  &mouse[i];
        mont_reg[i]->ereg = clt_base[i];
        cbmt_des[i]->TickCount += 17;
        mousep->buttons = mdev->mscr & 0xff;
        mousep->change = (mousep->buttons & BUTTON_DONE) != BUTTON_DONE;
        mousep->pos.x += (char)mdev->mcntx;
        mousep->pos.y += (char)mdev->mcnty;
        if (mousep->pos.x || mousep->pos.y || mousep->change) {
            mouse_keys(mousep);
            send_mouse_rec(i);
        }

        tab_watch_dog(i);
        server_watch_dog(i);
        kbd_watch_dog(i);
        i++;
    }
    
    timeout((caddr_t)vsync_int, 0, 1);
    splx(s);
}

mice_open(dev)
int dev;
{
    struct mdescr *mousep = &mouse[dev];
    if (mousep->open)
        return;
    
    mice_reg[dev] = (struct dev_mouse*)(cbip_io_base[dev] | MICE_OFFSET);
    mont_reg[dev] = (struct dev_mon*)(cbip_io_base[dev] | MON_OFFSET);
    intr_reg[dev] = (struct dev_intr*)(cbip_io_base[dev] | INTR_OFFSET);

    mousep->curmsg = 0;
    mousep->open++;
    mousep->pos.x = 0;
    mousep->pos.y = 0;
    mousep->buttons = 0;

    intr_reg[dev]->ievec = (cbip_intr_vec[dev] + 4) >> 2;
    intr_reg[dev]->iereg = MOUSE_IEN;

    cbmt_des[dev]->TickCount = 0;
    cbipdev[dev] = mice_reg[dev]->mscr & 0x40;
    timeout((caddr_t)vsync_int, 0, 1);
}

button_int(dev)
int dev;
{
    register struct dev_mouse *mdev = mice_reg[dev];
    register struct mdescr *mousep = &mouse[dev];

    mont_reg[dev]->ereg = clt_base[dev];
    mousep->buttons = mdev->mscr & 0xff;
    mousep->change = mousep->buttons ^ BUTTON_DONE;
    mousep->pos.x += (char)mdev->mcntx;
    mousep->pos.y += (char)mdev->mcnty;
    mouse_keys(mousep);
    send_mouse_rec(dev);
}

send_mouse_rec(dev)
int dev;
{
    register LocatorMsg *msg;
    register struct mdescr *mousep = &mouse[dev];
    register LocatorMsg *curmsg = mouse[dev].curmsg;
    BMT_DES *bdp = cbmt_des[dev];

    msg = (LocatorMsg*)get_msg(bdp);
    if (msg == 0) {
        cbmt_des[dev]->pool_flag |= MOUSE_WAITING;
        if (curmsg && (curmsg->head.flag & IN_QUEUE) && curmsg->head.id[1] == MOUSE_MSG &&
                !mousep->change) {
            curmsg->pos.x += mousep->pos.x;
            curmsg->pos.y += mousep->pos.y;
            mousep->pos.x = mousep->pos.y = 0;
        }
    } else {
        msg->head.id[0] = 0;
        msg->head.id[1] = MOUSE_MSG;
        msg->event = mousep->event;
        msg->pos = mousep->pos;
        msg->buttons = mousep->buttons;
        msg->time = bdp->TickCount;
        mousep->curmsg = msg;
        mousep->pos.x = mousep->pos.y = 0;
        send_cbmt(dev, msg);
    }
}

static mouse_keys(mousep)
register struct mdescr *mousep;
{
    register short buttons = mousep->buttons;
    register short chg = mousep->buttons ^ BUTTON_DONE;

    mousep->event = 0;
    mousep->buttons = 0;
    if (chg & L_DONE) {
        if (buttons & L_UP)
            mousep->event = L_BUTTON;
        else {
            mousep->event = R_BUTTON;
            mousep->buttons = MouseLeft;
        }
    }
    if (chg & M_DONE) {
        if (buttons & M_UP)
            mousep->event = L_BUTTON|R_BUTTON;
        else {
            mousep->event = M_BUTTON;
            mousep->buttons |= MouseMid;
        }
    }
    if (chg & R_DONE) {
        if (buttons & R_UP)
            mousep->event = L_BUTTON|M_BUTTON;
        else {
            mousep->event = M_BUTTON|R_BUTTON;
            mousep->buttons |=  MouseRight;
        }
    }
}
