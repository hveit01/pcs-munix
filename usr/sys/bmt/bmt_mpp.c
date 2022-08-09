/* PCS specific */

/* for monochrome BMT device */

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

static char *_Version = "@(#) RELEASE: 6.5 86/12/10 /usr/sys/bmt/bmt_mpp.c";

BMT_DES *bmt_des[MAXBIP];
static BIP_DEVICE *bmt_dev[MAXBIP];
extern char *bip_addr[];
extern short Bmt_default[];
extern short bmt_newmode[];
extern short bmt_dwnld_flag[];
extern short bip_intr[];
extern int bip_cnt;

send_bmt(dev, msg)
int dev;
register BMT_MESSAGE *msg;
{
    register BMT_FIFO *fp = &bmt_des[dev]->bmt_msg;
    register FIFO_ITEM *fip;
    
    msg->flag &= ~REPLIED;

    for (;;) {
        fip = fp->prod;
        if (++fip == &fp->fifo[F_MAX])
            fip = fp->fifo;
        if (fip->sig == 0) break;
        sleep((caddr_t)&fp->sem); /*bug: requires 2nd arg PRIO */
    }
    
    fip->val = (long)msg;
    fip->sig = F_MSG_NOT_FULL;
    
    if (fp->prod->sig == 0)
        Signal_bmt(&bmt_des[dev]->bmt_intr, F_MSG_NOT_EMPTY);
    fp->prod = fip;
}

BMT_MESSAGE *receive_bmt(dev, timeout)
int dev, timeout;
{
    register BMT_FIFO *fp = &bmt_des[dev]->unix_msg;
    register BMT_MESSAGE *msg;
    register FIFO_ITEM *fip;
    
    for (;;) {
        fip = fp->cons;
        if (++fip == &fp->fifo[F_MAX])
            fip = fp->fifo;
        if (fip->sig != 0) break;
        
        if (timeout == -1) {
            sleep((caddr_t)&fp->sem, NINTER-1);
        } else if (timeout > 0) {
            /* BUG: T_sleep called with obscure arg - see t_sleep.c */ 
            if (T_sleep((caddr_t)&fp->sem, timeout) != 0)
                return 0;
        } else
            return 0;
    }
    
    msg = (BMT_MESSAGE*)fip->val;
    fip->sig = 0;
    if (fp->cons->sig == 0)
        Signal_bmt(dev, F_MSG_NOT_FULL); /*bug: argument 1 not BMT_FIFO */
    fp->cons = fip;
    return msg;
}

reply_bmt(dev, msg)
int dev;
BMT_MESSAGE* msg;
{
    msg->flag &= ~REPLIED;
    Signal_bmt(&bmt_des[dev]->bmt_intr, F_REPLY_MSG, msg);
}

wait_bmt(msg, timeout)
BMT_MESSAGE *msg;
int timeout;
{
    while ((msg->flag & BUFFER_FULL) == 0) {
        if (timeout == -1)
            sleep((caddr_t)msg, NINTER-1);
        else if (timeout > 0) {
            if (T_sleep((caddr_t)msg, (ushort)timeout) != 0)
                return 1;
        } else
            return 1;
    }   
    msg->flag &= ~BUFFER_FULL;
    return 0;
}

rpc_bmt(dev, msg)
int dev;
BMT_MESSAGE *msg;
{
    send_bmt(dev, msg);
    wait_bmt(msg, -1);
}

static read_fifo(fp, frp)
register BMT_FIFO *fp;
FIFO_ITEM *frp;
{
    register FIFO_ITEM *fip = fp->cons;
    register FIFO_ITEM *fcons;
    
    if (++fip == &fp->fifo[F_MAX])
        fip = fp->fifo;
    
    *frp = *fip;
    if (frp->sig == 0)
        return;
    
    fip->sig = 0;
    fcons = fp->cons;
    if (fcons->sig != 0)
        Signal_bmt(&bmt_des[fp->bmt_id]->bmt_intr, F_INTR_NOT_FULL);

    fp->cons = fip;
}

Bmt_intr(dev)
int dev;
{
    register BIP_DEVICE *bip = (BIP_DEVICE*)&bip_addr[dev][RAM_MAP*0x10000];
    register BMT_DES *bdp;
    FIFO_ITEM fit;

    spltty();
    bip->map1 = RAM_MAP;
    
    if (bmt_newmode[dev] == 0)
        bipint(dev);
    else if ((bdp = bmt_des[dev]) != 0) {
        read_fifo(&bdp->unix_intr, &fit);
        while (fit.sig != 0) {
            switch (fit.sig) {
            case F_MSG_NOT_EMPTY:
                wakeup((caddr_t)&bdp->bmt_msg.sem);
                break;

            case F_MSG_NOT_FULL:
                wakeup((caddr_t)&bdp->unix_msg.sem);
                break;

            case F_POOL_NOT_EMPTY:
                wakeup((caddr_t)&bdp->unix_pool.sem);
                break;

            case F_REPLY_MSG:
                {
                    BMT_MESSAGE* msg = (BMT_MESSAGE*)fit.val;
                    msg->flag |= BUFFER_FULL;
                    wakeup((caddr_t)msg);
                    break;
                }

            case F_INTR_NOT_FULL:
                wakeup((caddr_t)&bdp->bmt_intr.sem);
                break;

            case F_EPORT_NOT_EMPTY:
                wakeup((caddr_t)&bdp->queue.pool_sem);
                Lock((caddr_t)&bdp->queue);
                if (bdp->queue.qflag & EVENT_SELECT) {
                    bdp->queue.qflag &= ~EVENT_SELECT;
                    wakeup((caddr_t)&nselect);
                }
                Unlock((caddr_t)&bdp->queue);
                break;

            case F_TTY_TREADY:
                {
                    Kbd_msg *msg = (Kbd_msg*)fit.val;
                    bip_xint(dev, msg);
                    bip->map1 = RAM_MAP;
                    break;
                }

            case F_TTY_RREADY:
                {
                    int ch = fit.val;
                    bmt_rint(dev, ch);
                    bip->map1 = RAM_MAP;
                    if (bdp->wps_flag)
                        wakeup((caddr_t)&bdp->wps_flag);
                    break;
                }
                
            case F_LONG_SEND:
                bdp->long_msg.src = (caddr_t)fit.val;
                wakeup((caddr_t)&bdp->long_msg.cnt);
                break;

            case F_LONG_RECEIVE:
                bdp->long_msg.dst = (caddr_t)fit.val;
                wakeup((caddr_t)&bdp->long_msg.cnt);
                break;

            case F_LONG_RELEASE:
                if ((bdp->long_msg.flag & 0x10) == 0)
                    release_buf(dev);
                break;

            case F_WPS_WAKEUP:
                wakeup((caddr_t)&bdp->wps_flag);
                break;

            default:
                printf("MPP Fatal Error 2 bmt=%d\n", dev);
                break;
            }
            read_fifo(&bdp->unix_intr, &fit);
        }
    }

    bip->map1 = Bmt_default[dev];
}

BMT_MESSAGE *request_bmt(dev)
int dev;
{
    register BMT_MESSAGE *msg;
    register BMT_FIFO *fp = &bmt_des[dev]->unix_pool;
    register FIFO_ITEM *fip;
    
    for (;;) {
        fip = fp->cons;
        if (++fip == &fp->fifo[F_MAX])
            fip = fp->fifo;
        
        if (fip->sig != 0) break;
        sleep((caddr_t)&fp->sem, NINTER-1);
    }
    
    msg = (BMT_MESSAGE*)(fip->val);
    fip->sig = 0;
    fp->cons = fip;
    return msg;
}

release_bmt(dev, msg)
int dev;
register BMT_MESSAGE *msg;
{
    if (msg->flag & EVENT_SELECT)
        return;

    msg->id[1] = 14;
    send_bmt(dev, msg);
}

extern char* xdes[];



static bmt_init(bip, id)
register BIP_DEVICE *bip;
int id;
{
    register BMT_DES *bdp;
    register char *dummy1;  /* unused but essential */
    short dummy2 = 0;       /* unused */

    set_csreg(bip);
    bip->host_base = (long)bip;
    bip->bmt_id = id;
    bip->host_intr = (short)bip_intr[id];
    bdp = bmt_des[id] = (BMT_DES*)&(((char*)bip->bmt_desp)[(int)bip]);
    bmt_dev[id] = bip;
    bdp->bmt_msg.prod = bdp->bmt_msg.fifo;
    bdp->unix_msg.cons = bdp->unix_msg.fifo;
    bdp->unix_intr.bmt_id = id;
    bdp->unix_intr.cons = bdp->unix_intr.fifo;
    bdp->bmt_intr.bmt_id = id;
    bdp->bmt_intr.prod = bdp->bmt_intr.fifo;
    bdp->unix_pool.cons = bdp->unix_pool.fifo;
    bdp->bmt_pool.prod = bdp->bmt_pool.fifo;

    InitEventQueue(id);
    release_buf(id);
    bmt_newmode[id] = 1;
    bmt_dwnld_flag[id] = 0;
    bip->intr_local68 = rintr;
}

Bmt_init()
{
    register BIP_DEVICE *bip;
    register int i, k;
    int dummy;  /*unused*/

    spltty();

    for (i=0; i < bip_cnt; i++) {
        bip = (BIP_DEVICE*)&bip_addr[i][RAM_MAP*0x10000];
        if (ssword((caddr_t)bip, 0) != 0) continue;
        if (bip->host_base == BMT_MAGIC) {
            bip->host_base = 0;
            for (k = 1000; bip->host_base ==0; ) {
                if (--k == 0) break;
            }
            if (bip->host_base != 0)
                bmt_init(bip, i);
        }
    }
}

Signal_bmt(fp, sig, val)
register BMT_FIFO *fp;
register int sig;
register long val;  /* is usually a pointer */
{
    register FIFO_ITEM *fip;
    
    for (;;) {
        fip = fp->prod;
        if (++fip == &fp->fifo[F_MAX])
            fip = fp->fifo;
        if (fip->sig == 0) break;
        sleep((caddr_t)&fp->sem, NINTER-1);
    }

    fip->val = val;
    fip->sig = sig;
    if (fp->prod->sig == 0)
        bmt_dev[fp->bmt_id]->intr_local68 = wintr;
    
    fp->prod = fip;
}

Interrupt_bmt(dev)
int dev;
{
    bmt_dev[dev]->intr_local68 = wintr;
}

Int_signal_bmt(fp, sig, val)
register BMT_FIFO *fp;
register int sig;
register long val;  /* is usually a pointer */
{
    register FIFO_ITEM *fip;
    
    fip = fp->prod;
    if (++fip == &fp->fifo[F_MAX])
        fip = fp->fifo;
    if (fip->sig != 0)
        return 0;

    fip->val = val;
    fip->sig = sig;
    if (fp->prod->sig == 0)
        bmt_dev[fp->bmt_id]->intr_local68 = wintr;
    
    fp->prod = fip;
    return 1;
}