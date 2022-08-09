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

static char *_Version = "@(#) RELEASE: 6.10 87/01/07 /usr/sys/bmt/bmt.c";
int Bip_test = 0;

#define BIPVEC  (0x16c+4)       /* see l.s */

short Bmt_default[MAXBIP];
short bmt_newmode[MAXBIP];
short bmt_dwnld_flag[MAXBIP];
Kbd_msg *out_queue[MAXBIP];
extern struct tty bip_tty[];

extern char *bip_addr[];
extern int bip_cnt;
extern BMT_DES *bmt_des[];
extern short fuword();

/*forward*/int bmtproc();

bipopen(dev)
int dev;
{
    register struct tty *tp;
    register BIP_DEVICE *bip;
    Bip_device *mem;
    
    bip = (BIP_DEVICE*)&bip_addr[dev][RAM_MAP*0x10000]; /* real bitmap memory */
    mem = (Bip_device*)&bip_addr[dev][ROM_MAP*0x10000]; /* total mem range */
    
    if (dev >= bip_cnt || fsword(&bip->go) == -1) {
        u.u_error = ENXIO;
        return;
    }
    
    tp = &bip_tty[dev];
    bip->map1 = RAM_MAP;
    
    if ((tp->t_state & (WOPEN|ISOPEN)) == 0) {
        ttinit(tp);
        tp->t_proc = bmtproc;
        Bmt_default[dev] = RAM_MAP;
        if (bmt_newmode[dev] == 0) {
            bip->go = INT_ENABLE|RUN;
            delay(hz);
            mem->rintno = mem->wintno = (dev & 3) + (BIPVEC/4); /* int vector base */
            mem->intr_state = 0;
        } else {
            bip->go = 0xc000|INT_ENABLE|RUN;
        }
    }
    tp->t_state |= CARR_ON;
    (*linesw[tp->t_line].l_open)(tp);
    bip->map0 = 4;
    bip->map1 = Bmt_default[dev];
}

bipclose(dev)
dev_t dev;
{
    register struct tty *tp = &bip_tty[dev];
    (*linesw[tp->t_line].l_close)(tp);
}

bipwrite(dev)
int dev;
{
    register struct tty *tp = &bip_tty[dev];
    (*linesw[tp->t_line].l_write)(tp);
}

bipread(dev)
int dev;
{
    register struct tty *tp = &bip_tty[dev];
    (*linesw[tp->t_line].l_read)(tp);
}

bip_mouse_read(dev, cmd)
int dev, cmd;
{
    register struct tty *tp;
    register char *addr = &bip_addr[dev][0];    /* must be named addr, see cadmaus.h */
    struct { short x,y,keys; } xy;
    char *xyp = (char*)&xy;
    char *p;

    /* ignore if tty buf full */
    tp = &bip_tty[dev];
    if (tp->t_rawq.c_cc >= TTXOHI &&
        (cmd == M_timeout || cmd == M_motion || tp->t_rawq.c_cc >= (TTYHOG-7)))
        return;

    if (M->mflags & 0x40) {
        xy.x = M->mxw;
        xy.y = M->myw;
    } else {
        xy.x = M->mx;
        xy.y = M->my;
    }
    
    /* upload data */
    bmt_rint(dev, cmd);
    if (cmd == M_timeout)
        return;
    for (p = xyp; p < (xyp+4); p++)
        bmt_rint(dev, *p);
}

bipint(dev)
int dev;
{
    register char *keyp;
    register struct tty *tp;
    register Bip_device *bip;
    register int i;
    int dummy;
    
    bip = (Bip_device*)&bip_addr[dev][0];
    if (bmt_dwnld_flag[dev])
        return;

    tp = &bip_tty[dev];
    while ( (i=bip->intr_state) != 0) {
        bip->intr_state = 0;
        switch (i) {
        case wintr:
            tp->t_state &= ~BUSY;
            bmtproc(tp, T_OUTPUT);
            break;
        case rintr:
            for (keyp = bip->keys; (i = *(unsigned char*)keyp) != 0; keyp++) {
                if (i < M_lowchar || i > M_hichar)
                    bmt_rint(dev, i);
                else
                    bip_mouse_read(dev, i);
                bip->map1 = RAM_MAP;
            }
            break;
        default:
            printf("bipint error flag=%d", i);
            break;
        }
        bip->map1 = RAM_MAP;
    }
}

bip_xint(dev, val)
int dev;
Kbd_msg *msg;
{
    register struct tty *tp;

    sysinfo.xmtint++;
    out_queue[dev] = msg;
    tp = &bip_tty[dev];
    tp->t_state &= ~BUSY;
    bmtproc(tp, T_OUTPUT);
}

bmt_rint(dev, val)
int dev;
register int val;
{
    register struct tty *tp;

    sysinfo.rcvint++;
    if (bmt_dwnld_flag[dev])
        return;
    
    tp = &bip_tty[dev];
    if ((tp->t_state & ISOPEN) == 0)
        return;
    if (tp->t_rbuf.c_ptr == 0)
        return;

    if (tp->t_iflag & IXON) {
        if (tp->t_state & TTSTOP) {
            if (val == 0x11 || (tp->t_iflag & IXANY)) /* XON */
                (*tp->t_proc)(tp, T_RESUME);
        } else if (val == 0x13) {   /* XOFF */
            (*tp->t_proc)(tp, T_SUSPEND);
        }
        if (val == 0x11 || val == 0x13)
            return;
    }
    tp->t_rbuf.c_ptr[0] = val;
    tp->t_rbuf.c_count--;
    (*linesw[tp->t_line].l_input)(tp, L_BUF);
}

old_wpsmouse(dev, arg)
int dev;
caddr_t arg;
{
    char *addr = &bip_addr[dev][0]; /* must be addr, see cadmaus.h */
    struct { short x,y,keys; } xy;
    
    xy.x = M->mxw;
    xy.y = M->myw;
    xy.keys = M->mkeys;
    if (copyout(&xy, arg, sizeof(xy)) != 0)
        u.u_error = EFAULT;
}

bipioctl(dev, func, arg, sel)
dev_t dev;
int func;
union {
    caddr_t ap;
    ushort ua[2];
    int ia;
} arg;
int sel;
{
    register struct tty *tp = &bip_tty[dev];
    register BIP_DEVICE *bip = (BIP_DEVICE*)(bip_addr[dev]+RAM_MAP*0x10000);
    BIP_DEVICE *bip2;
    int s;
    uint map1;
    

    if (func == TCBIPADR) {
        sulong((caddr_t)arg, bip_addr[dev]);
        return;
    }
    
    s = currpl();
    spltty();
    bip->map1 = RAM_MAP;
    switch (func) {
    case GET_EVENT:
    case FLUSH_GET:
    case EVENT_AVAIL:
    case POST_EVENT:
    case FLUSH_EVENT:
        if (bmt_newmode[dev])
            EventMngr(dev, func, arg.ap);
        else
            u.u_error = ENXIO;
        break;

    case MIOCRESET: 
        {
            Bip_device *p;
            splhi();
            if (arg.ap)
                bmt_dwnld_flag[dev] = 1;
            out_queue[dev] = 0;
        
            bip2 = (BIP_DEVICE*)bip_addr[dev];
            bip->go = 0;

            for (p = (Bip_device*)bip; p < (Bip_device*)&bip->go; )
                *((short*)p)++ = 0;
            bip->go = INT_ENABLE|READY;
            bmt_newmode[dev] = 0;
            delay(hz);
            ((Bip_device*)bip2)->rintno = 
            ((Bip_device*)bip2)->wintno = (dev & 3) + (BIPVEC/4);   /* int vector base */
            break;
        }

    case MIOC_MAP1:
        map1 = arg.ua[1];
        Bmt_default[dev] = map1;
        break;

    case MIOCCNTREG:
        bip->go = fuword(arg.ap) | (INT_ENABLE|RUN);
        break;

    case MIOCRMWON:
        bip->go = 0xc000 | INT_ENABLE|RUN;
        break;

    case MIOCRMWOUT:
        bip->go = INT_ENABLE|RUN;
        break;

    case MIOCGETPOSW:
        if (bmt_newmode[dev] == 0)
            old_wpsmouse(dev, arg.ap);
        else {
            BMT_DES *bdp = bmt_des[dev];
            if (bdp->wps_new.pos.x == bdp->wps_old.pos.x &&
                bdp->wps_new.pos.y == bdp->wps_old.pos.y && 
                bdp->wps_new.keys == bdp->wps_old.keys) {
                bdp->wps_flag = 1;
                bip->map1 = Bmt_default[dev];
                sleep((caddr_t)&bdp->wps_flag, NINTER-1);
                bip->map1 = RAM_MAP;
                bdp->wps_flag = 0;
            }
            bdp->wps_old = bdp->wps_new;
            if (copyout((caddr_t)&bdp->wps_new, arg.ap, sizeof(WPS_BLOCK)) != 0)
                u.u_error = EFAULT;
        }
        break;

    case MIOCSEND:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            Send(dev, arg);
        break;

    case MIOCLSEND:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            LSend(dev, arg);
        break;

    case MIOCRECEIVE:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            Receive(dev, arg);
        break;

    case MIOCLRECEIVE:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            LReceive(dev, arg);
        break;

    case MIOCREPLY:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            Reply(dev, arg);
        break;

    case MIOCRPC:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            Bmt_rpc(dev, arg);
        break;

    case MIOCWAIT:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            Wait_for(dev, arg);
        break;

    case MIOCOPYIN:
    case MIOCOPYOUT:
        if (bmt_newmode[dev] == 0)
            u.u_error = ENXIO;
        else
            bmt_swap(dev, func, arg);
        break;
    default:
        ttiocom(tp, func, arg, sel);
        break;
    }
    
    bip->map1 = Bmt_default[dev];
    splx(s);
}

bmtproc(tp, func)
register struct tty *tp;
int func;
{
    register char *qp;
    register char *cp;
    register uint cnt;
    int dev;
    BIP_DEVICE *bip;
    Bip_device *bip2;
    Kbd_msg *outq;
    int s = currpl();
    spltty();

    dev = tp - bip_tty;
    if (bmt_dwnld_flag[dev]) {
        splx(s);
        return;
    }

    switch (func) {
    case T_RESUME:
    case T_WFLUSH:
        tp->t_state &= ~TTSTOP;
        /*FALLTHRU*/

    case T_OUTPUT:
        if (tp->t_state & (TTSTOP|BUSY))
            break;
        if (bmt_newmode[dev]) {
            if ((outq = out_queue[dev])==0)
                break;
            if ((((*linesw[tp->t_line].l_output)(tp)) & CPRES) == 0)
                break;
            cnt = tp->t_tbuf.c_count;
            tp->t_state |= BUSY;
            bip = (BIP_DEVICE*)&bip_addr[dev][RAM_MAP*0x10000];
            bip->map1 = RAM_MAP;
            qp = (char*)&outq->cnt;
            cp = tp->t_tbuf.c_ptr;
            *qp++ = cnt;
            *qp++ = 0;
            while (cnt-- != 0)
                *qp++ = *cp++;
            *qp = '\0';
            tp->t_tbuf.c_count = 0;
            tp->t_state &= ~BUSY;
            out_queue[dev] = 0;
            Signal_bmt(&bmt_des[dev]->bmt_intr, F_REPLY_MSG, outq);
            bip->map1 = Bmt_default[dev];
        } else {
            if (((*linesw[tp->t_line].l_output)(tp) & CPRES) == 0)
                break;
            cnt = tp->t_tbuf.c_count;
            bip2 = (Bip_device*)bip_addr[dev];
            bip2->map1 = RAM_MAP;
            while (bip2->func);
            bcopy(tp->t_tbuf.c_ptr, bip2->wbuf, cnt);
            bip2->args[0] = cnt;
            bip2->func = UNIX_WAITING;
            tp->t_tbuf.c_count = 0;
            bip2->map1 = Bmt_default[dev];
            tp->t_state |= BUSY;
        }
        break;

    case T_SUSPEND:
        tp->t_state |= TTSTOP;
        break;
    }
    splx(s);
}

/* only use UART of BMT 0 */
bmt_getchar()
{
    register CON_DEVICE *devp = &bmt_des[0]->con_reg;

    devp->rcsr |= 1;
    while (devp->rcsr & 1);
    return devp->rbuf;
}

/* only use UART of BMT 0 */
bmt_putchar(ch)
register int ch;
{
    register CON_DEVICE *devp = &bmt_des[0]->con_reg;
    register int wait = 50000;

    while ((devp->tcsr & 1) && wait-- != 0);
    devp->tbuf = ch;
    devp->tcsr |= 1;
    Int_signal_bmt(&bmt_des[0]->bmt_intr, F_CONS_REQ, ch);
}

set_csreg(bip)
BIP_DEVICE *bip;
{
    bip->go = 0xc000 | INT_ENABLE | RUN;
}
