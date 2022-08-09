/* PCS specific */

/* Event handling for CBIP devices */

#include <sys/types.h>
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

static char *_Version = "@(#) RELEASE: 6.1 86/11/28 /usr/sys/bmt/kkeyboard.c";

static char ledon[]  = "\033[xq";
static char ledoff[] = "\033[xo";
static KEYBOARD kbdrec[NCBMT];

extern scc_writes(), scc_write();
extern int scc_read();
extern short colserver[];

kbd_ioctl(dev, func, arg)
int dev;
int func;
long arg;
{
    register int sccdev = dev << 1;
    switch (func) {
    case KC_SET_LED:
        ledon[2] = arg;
        scc_writes(sccdev, ledon);
        break;
        
    case KC_RESET_LED:
        ledoff[2] = arg;
        scc_writes(sccdev, ledoff);
        break;

    case KC_EVENT:
        kbdrec[dev].flag |= KF_EVENT;
        kbdrec[dev].flag &= ~KF_KEYDOWN;
        scc_writes(sccdev, "\033[2qK");
        break;

    case KC_NORMAL:
        scc_writes(sccdev, "\033[2oN");
        kbdrec[dev].flag &= ~KF_EVENT;
        break;

    case KC_LOCAL:
        scc_writes(sccdev, "\033[3qN");
        break;

    case KC_ON_LINE:
        scc_writes(sccdev, "\033[3oN");
        break;

    case KC_DEVICE:
        kbd_device(sccdev);
        break;

    case KC_AUTOREP_ON:
        kbdrec[dev].flag |= KF_AUTOREPEAT;
        kbdrec[dev].flag &= ~KF_KEYDOWN;
        kbdrec[dev].time = 36;
        break;

    case KC_AUTOREP_OFF:
        kbdrec[dev].flag &= ~KF_AUTOREPEAT;
        break;

    case KC_BELL:
        scc_write(sccdev, 7);
        break;
    }
}

kbd_rint(dev)
int dev;
{
    register char ch;
    register int sccdev = dev << 1;
    register KEYBOARD *kp = &kbdrec[dev];

    while (scc_ioctl(sccdev, SCC_EMPTY) == 0) {
        ch = scc_read(sccdev);
        kp->lastkey = ch;
        if (ch & 0x80) {
            kp->flag &= ~KF_KEYDOWN;
            kp->time = 0x7fffffff;  /* long time */
        } else {
            kp->flag |= KF_KEYDOWN;
            kp->time = 36;          /* repeat timer */
        }
        if (colserver[dev])
            cbip_rint(dev, ch);
        else
            cbip_inp(dev, ch);
    }
}

kbd_open(dev)
int dev;
{
    register int sccdev = dev << 1;
    spltty();
    scc_open(sccdev);
    kbd_ioctl(dev, KC_NORMAL);
    spl0();
}

static int kbd_device(sccdev)
register int sccdev;
{
    int ch = 0;
    int cnt = 2000;
    
    while (scc_ioctl(sccdev, SCC_EMPTY) == 0)
        scc_read(sccdev);
    
    scc_write(sccdev, 'W');
    do {
        if (scc_ioctl(sccdev, SCC_EMPTY) == 0) break;
    } while (--cnt > 0);
    
    if (scc_ioctl(sccdev, SCC_EMPTY) == 0)
        ch = scc_read(sccdev);
    return ch;
}

kbd_watch_dog(dev)
int dev;
{
    register KEYBOARD *kp = &kbdrec[dev];
    if ((kp->flag & KF_AUTOREPEAT) && (kp->flag & KF_EVENT) && --kp->time <= 0) {
        if ((kp->flag & KF_KEYDOWN) && colserver[dev]) {
            cbip_rint(dev, kp->lastkey);
            kp->time = 6;
            return;
        } else 
            kp->time = 0x7fffffff;
    }
}
