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

static char *_Version = "@(#) RELEASE: 6.6 86/12/10 /usr/sys/bmt/cbip.c";

static BMT_DES cbip_des[NCBMT];

BMT_DES *cbmt_des[NCBMT] = { &cbip_des[0], &cbip_des[1] };
short colserver[NCBMT];

extern int cbip_cnt;                    /* c.c */
extern unsigned long cbip_io_base[];    /* c.c */
extern struct tty cbip_tty[];           /* c.c */
extern char *cbip_addr[];               /* c.c */
extern int clt_base[];

extern cbip_proc();

cbip_open(dev)
int dev;
{
    register struct tty *tp;
    
    if (dev >= cbip_cnt || fsword(cbip_io_base[dev]) == -1) {
        u.u_error = ENXIO;
        return;
    }
    
    tp = &cbip_tty[dev];
    
    if ((tp->t_state & (WOPEN|ISOPEN))==0) {
        ttinit(tp);
        tp->t_proc = cbip_proc;
        col_init(dev);
    } else {
        (*linesw[tp->t_line].l_open)(tp);
        return;
    }
    
    tp->t_state |= CARR_ON;
    (*linesw[tp->t_line].l_open)(tp);
    init_server(dev);
}

static init_server(dev)
{
    register BMT_DES *bdp = cbmt_des[dev];
    
    bdp->kbd_msg.head.flag = NORELEASE;
    bdp->kbd_msg.head.id[0] = 0;
    bdp->kbd_msg.head.id[1] = KBD_MSG;
    bdp->tty_msg.flag = LONG_ERROR;
    bdp->tty_msg.id[1] = TTY_MSG;
    mice_open(dev);
    cbip_scc_init(dev);
    tab_open(dev);
    kbd_open(dev);
    mpp_init(dev);
    event_init(dev);
    cbip_xint(dev);
}

cbip_close(dev)
dev_t dev;
{
    register struct tty *tp = &cbip_tty[dev];
    (*linesw[tp->t_line].l_close)(tp);
}

cbip_write(dev)
int dev;
{
    register struct tty *tp = &cbip_tty[dev];
    (*linesw[tp->t_line].l_write)(tp);
}

cbip_read(dev)
int dev;
{
    register struct tty *tp = &cbip_tty[dev];
    (*linesw[tp->t_line].l_read)(tp);
}

cbip_xint(dev)
int dev;
{
    register struct tty *tp;
    
    sysinfo.xmtint++;
    tp = &cbip_tty[dev];
    tp->t_state &= ~BUSY;
    cbip_proc(tp, T_OUTPUT);
}

cbip_rint(dev, ch)
int dev;
int ch;
{
    register Kbd_msg *msg;
    
    msg = &cbip_des[dev].kbd_msg;
    if ((msg->head.flag & IN_QUEUE)==0)
        msg->cnt = 0;
    if (msg->cnt >= MAX_KBD_BUF) return;

    msg->buf[msg->cnt] = ch;
    msg->cnt++;
    msg->time = cbmt_des[dev]->TickCount;
    if ((msg->head.flag & IN_QUEUE) == 0)
        send_cbmt(dev, msg);
}

cbip_inp(dev, ch)
int dev;
register int ch;
{
    register struct tty *tp;
    
    sysinfo.rcvint++;
    tp = &cbip_tty[dev];
    
    if ((tp->t_state & ISOPEN) == 0) return;
    if (tp->t_rbuf.c_ptr == 0) return;

    if (tp->t_iflag & IXON) {
        if (tp->t_state & TTSTOP) {
            if (ch == 0x11 || (tp->t_iflag & IXANY)) /* XON */
                (*tp->t_proc)(tp, T_RESUME);
        } else if (ch == 0x13) /* XOFF */
            (*tp->t_proc)(tp, T_SUSPEND);
        if (ch == 0x11 || ch == 0x13)
            return;
    }
    if (tp->t_iflag & ISTRIP)
        ch &= 0x7f;
    else
        ch &= 0xff;

    *tp->t_rbuf.c_ptr = ch;
    tp->t_rbuf.c_count--;
    (*linesw[tp->t_line].l_input)(tp, 0);
}

cbip_ioctl(dev, func, arg, sel)
dev_t dev;
int func;
caddr_t arg;
int sel;
{
    register struct tty *tp = &cbip_tty[dev];
    register BMT_DES *bdp = &cbip_des[dev];
    int s;
    int i;
    
    if (func > MIOCSTART && func < MIOCEND && colserver[dev] == 0) {
        u.u_error = ENXIO;
        return;
    }
    
    s = currpl();
    spltty();

    switch (func) {
    case SERVER_ON:
        bdp->col_proc = u.u_procp;
        init_server(dev);
        colserver[dev] = 1;
        break;
        
    case ACTIVE_WINDOW:
        cbmt_des[dev]->activeWinInd = (short)arg;
        break;
        
    case MIOCSEND:
        CSend(dev, arg);
        break;
        
    case MIOCLSEND:
        CLSend(dev, arg);
        break;
        
    case MIOCRECEIVE:
        CReceive(dev, arg);
        break;
    
    case MIOCLRECEIVE:
        CLReceive(dev, arg);
        break;
    
    case MIOCREPLY:
        CReply(dev, arg);
        break;
        
    case MIOCRPC:
        CBmt_rpc(dev, arg);
        break;
    
    case MIOCWAIT:
        CWait_for(dev, arg);
        break;
        
    case GET_EVENT:
        GetEvent(dev, arg);
        break;
        
    case FLUSH_EVENT:
        FlushEvents(dev, arg);
        break;
    
    case EVENT_AVAIL:
        EventAvail(dev, arg);
        break;
        
    case POST_EVENT:
        PostEvent(dev, arg);
        break;
        
    case FLUSH_GET:
        FlushGetEvent(dev, arg);
        break;
    
    case SERVER_BASE:
        {
            struct ap {
                unsigned long addr, iobase, size;
            };
            sulong(&((struct ap*)arg)->addr, cbip_addr[dev]);
            sulong(&((struct ap*)arg)->iobase, cbip_io_base[dev]);
            sulong(&((struct ap*)arg)->size, 0xfffff);
            break;
        }

    case KBD_CMD:
        {
            unsigned long kfunc, kval;
            struct ap {
                long *kfuncp, *kvalp;
            } *ap = (struct ap*)arg;
            kfunc = fulong(&ap->kfuncp);
            kval = fulong(&ap->kvalp);
            kfunc = kbd_ioctl(dev, kfunc, kval);
            sulong(&ap->kvalp, kfunc);
            break;
        }
    
    case TAB_CMD:
        {
            struct { short func, arg; } val;
            if (copyin(arg, &val, sizeof(long)) != 0)
                u.u_error = EFAULT;
            else
                tab_ioctl(dev, val.func, val.arg);
            break;
        }
        
    case TTY_CMD:
        if (copyin(arg, bdp->rbuf, sizeof(bdp->rbuf)) != 0) {
            u.u_error = EFAULT;
            splx(s);
            return;
        }
        for (i=1; i <= bdp->rbuf[0]; i++)
            cbip_inp(dev, bdp->rbuf[i]);
        break;
        
    case COLTAB_CMD:
        clt_base[dev] = fulong(arg) & 0x0f;
        break;

    default:
        ttiocom(tp, func, arg, sel);
        break;
    }
    splx(s);
}

cbip_proc(tp, func)
register struct tty *tp;
int func;
{
    register BMT_DES *bdp;
    int dev;
    int s = spltty();
    
    

    switch (func) {
    case T_RESUME:
    case T_WFLUSH:
        tp->t_state &= ~TTSTOP;
        /*FALLTHRU*/

    case T_OUTPUT:
        if (tp->t_state & (TTSTOP|BUSY))
            break;
        dev = tp - cbip_tty;
        bdp = &cbip_des[dev];
        if (bdp->pcnt > 0) {
            bdp->pcnt = send_to_server(tp, dev, bdp->pcnt);
            tp->t_state |= BUSY;
        } else if ((*linesw[tp->t_line].l_output)(tp) & CPRES) {
            bdp->pbuf = tp->t_tbuf.c_ptr;
            bdp->pcnt = tp->t_tbuf.c_count;
            tp->t_tbuf.c_count = 0;
            bdp->pcnt = send_to_server(tp, dev, bdp->pcnt);
            tp->t_state |= BUSY;
        }
        if (colserver[dev] == 0)
            tp->t_state &= ~BUSY;
        splx(s);        /* redundant */
        break;

    case T_SUSPEND:
        tp->t_state |= TTSTOP;
        break;
    }
    splx(s);
}

static send_to_server(tp, dev, cnt)
register struct tty *tp;
int dev;
int cnt;
{
    register BMT_MESSAGE *msg = &cbip_des[dev].tty_msg;
    BMT_DES *bdp = cbmt_des[dev];
    register char *cp;
    int n;
    
    if (colserver[dev] == 0) {
        cursoff(dev);
        while (cnt-- != 0)
            colput__(dev, *bdp->pbuf++);
        curson(dev);
        return 0;
    }

    n = MSG_SIZE*sizeof(long)-1;
    if (cnt < n)
        n = cnt;
    cnt -= n;

    cp = (char*)msg->data;
    *cp++ = n;
    while (n-- != 0)
        *cp++ = *bdp->pbuf++;
    msg->id[0] = 0;
    send_cbmt(dev, msg);
    return cnt;
}

cbip_getchar(dev)
{
}

cbip_putchar(dev)
register int dev;
{
}

proc_bmt_request(bdp)
register BMT_DES *bdp;
{
    static short toggle = 4;

    for (;;) {
        switch (toggle) {
        case 2:
            toggle = 4;
            if (bdp->pool_flag & MOUSE_WAITING) {
                bdp->pool_flag &= ~MOUSE_WAITING;
                send_mouse_rec(bdp->bmtid);
                return;
            }
            /*FALLTHRU*/
        case 4:
            toggle = 2;
            if (bdp->pool_flag & TAB_WAITING) {
                bdp->pool_flag &= ~TAB_WAITING;
                tab_request(bdp->bmtid);
                return;
            }
        }
    }
}

cbip_ackn(dev)
int dev;
{
    register struct tty *tp = &cbip_tty[dev];
    
    tp->t_state &= ~BUSY;
    cbip_proc(tp, T_OUTPUT);
}
