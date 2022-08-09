/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.2  Apr 06 1987 /usr/sys/io/mfp.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "mfp.h"

struct tty mfp_tty;
int mfp_flags;
int mfp_cnt = 1;
extern struct mfpregs _mfp[];
extern mfpproc(), ttrstrt();

mfpopen(dev)
dev_t dev;
{
    register struct mfpregs *mp;
    register struct tty *tp;
    int dummy1,dummy2; /* not used, but declared in original code */

    if (dev >= mfp_cnt) {
        u.u_error = ENXIO;
        return;
    }

    mp = _mfp;
    tp = &mfp_tty;

    if ((tp->t_state & ISOPEN)==0) {
        ttinit(tp);
        mfp_flags = 0;
        tp->t_proc = mfpproc;
        mfpparam(dev);
        mp->vr = MFPIV_GPI0 | 0x70; /* vector 0x70 */
    }
    tp->t_state |= CARR_ON;
    (*linesw[tp->t_line].l_open)(tp);
}

mfpclose(dev)
{
    register struct mfpregs *mp; /*not used*/
    register struct tty *tp;

    tp = &mfp_tty;
    mp = _mfp;

    (*linesw[tp->t_line].l_close)(tp);
}

mfpread()
{
    register struct tty *tp = &mfp_tty;
    (*linesw[tp->t_line].l_read)(tp);
}

mfpwrite()
{
    register struct tty *tp = &mfp_tty;
    (*linesw[tp->t_line].l_write)(tp);
}

mfpxint()
{
    register struct tty *tp;
    register struct mfpregs *mp;
    
    sysinfo.xmtint++;
    
    tp = &mfp_tty;
    mp = _mfp;

    if (mp->tsr & MFPTS_BE) {
        if (tp->t_state & TTXON) {
            mp->udr = 0x11; /*xon*/
            tp->t_state &= ~TTXON;
        } else if (tp->t_state & TTXOFF) {
            mp->udr = 0x13; /*xoff*/
            tp->t_state &= ~TTXOFF;
        } else {
            tp->t_state &= ~BUSY;
            mfpproc(tp, T_OUTPUT);
        }
    }
}

mfprint()
{
    register struct mfpregs *mp;
    register struct tty *tp;
    register uint ch;
    register uint ch2;

    sysinfo.rcvint++;

    tp = &mfp_tty;
    mp = _mfp;

    if ((mp->rsr & MFPRS_BF)==0)
        return;

    if ((tp->t_state & ISOPEN)==0)
        return;

    if (tp->t_rbuf.c_ptr == 0)
        return;

    do {
        ch = mp->udr;
        ch2 = ch & 0x7f;

        if (tp->t_iflag & IXON) {
            if (tp->t_state & TTSTOP) {
                if (ch2 == 0x11 /*xon*/ || (tp->t_iflag & IXANY))
                    (*tp->t_proc)(tp, T_RESUME);
            } else {
                if (ch2 == 0x13) /*xoff*/
                    (*tp->t_proc)(tp, T_SUSPEND);
            }
            if (ch2 == 0x11 || ch2 == 0x13)
                continue;
        }
        if (tp->t_iflag & ISTRIP)
            ch &= 0x7f;

        *tp->t_rbuf.c_ptr++ = ch;
        if (--tp->t_rbuf.c_count == 0) {
            tp->t_rbuf.c_ptr -= (tp->t_rbuf.c_size  - tp->t_rbuf.c_count);
            (*linesw[tp->t_line].l_input)(tp, T_OUTPUT);
        }
    } while (mp->rsr & MFPRS_BF);
 
    if (tp->t_rbuf.c_ptr) {
        tp->t_rbuf.c_ptr -= (tp->t_rbuf.c_size - tp->t_rbuf.c_count);
        (*linesw[tp->t_line].l_input)(tp, T_OUTPUT);
    }
}

mfperrint()
{
    register struct mfpregs *mp;
    register struct tty *tp;
    register uint ch;
    register int iflag;
    unsigned char out[3];
    short cnt;
    uint rsr;

    tp = &mfp_tty;
    mp = _mfp;

    if (((rsr = mp->rsr) & MFPRS_FSOB) &&  /*break?*/
        (tp->t_iflag & IGNBRK) == 0 &&
        (tp->t_iflag & BRKINT) != 0) {
        (*linesw[tp->t_line].l_input)(tp,T_RESUME);
        return;
    }
    
    ch = mp->udr;
    if ((tp->t_state & ISOPEN) == 0)
        return;
    
    if (tp->t_rbuf.c_ptr == 0)
        return;

    cnt = 1;
    iflag = tp->t_iflag;
    if ((rsr & MFPRS_PE) && (iflag & INPCK)==0)
        rsr &= ~MFPRS_PE;
    
    if ((rsr & (MFPRS_OE|MFPRS_PE|MFPRS_FE))) {
        if ((rsr & MFPRS_PE) && (iflag & IGNPAR))
            return;

        if ((rsr & (MFPRS_PE|MFPRS_FE)) && (iflag & PARMRK)) {
            out[2] = 0xff;
            out[1] = 0;
            cnt = 3;
            sysinfo.rawch += 2;
        } else
            ch = 0;
        out[0] = ch;

        while (cnt) {
            cnt--;
            *(tp->t_rbuf.c_ptr) = out[cnt];
            tp->t_rbuf.c_count--;
            (*linesw[tp->t_line].l_input)(tp, T_OUTPUT);
        }
    }
}

mfpioctl(dev, cmd, arg, mode)
short dev;
{
    if (ttiocom(&mfp_tty, cmd, arg, mode))
        mfpparam(dev);
}

mfpparam(dev)
{
    register struct mfpregs *mp;
    register struct tty *tp;
    register int cflag;
    int i;
    char buf[13];
    
    tp = &mfp_tty;
    tp->t_cflag |= (CS8|CLOCAL);
    tp->t_cflag &= ~(CSTOPB|PARENB);
    
    mp = _mfp;
    cflag = tp->t_cflag;
    if (cflag != mfp_flags) {
        mfp_flags = cflag;
        for (i = cflag & CBAUD; mp->rsr & MFPRS_BF; )
            buf[0] = mp->udr;
        mp->iera |= (MFPIA_RBF|MFPIA_RERR|MFPIA_TBE);
        mp->imra |= (MFPMA_RBF|MFPMA_RERR|MFPMA_TBE);
    }
}

mfpproc(tp, cmd)
register struct tty *tp;
{
    register struct mfpregs *mp = _mfp;
    register struct ccblock *tbp;
    register dummy; /*unused*/
    register dev; /*uninitialized*/

    switch(cmd) {
    case T_TIME:
        tp->t_state &= ~TIMEOUT;
        mp->tsr &= ~MFPTS_B;
        goto out;

    case T_RESUME:
    case T_WFLUSH:
        tp->t_state &= ~TTSTOP;

    case T_OUTPUT:
out:
        tbp = &tp->t_tbuf;
        if (tp->t_state & (TTSTOP|BUSY))
            return;
        if (tbp->c_ptr==0 || tbp->c_count <= 0) {
            if (tbp->c_ptr)
                tbp->c_ptr -= (tbp->c_size - tbp->c_count);
            if (((*linesw[tp->t_line].l_output)(tp) & 0x8000) == 0)
                return;
        }
        tp->t_state |= BUSY;
        mp->udr = *tbp->c_ptr++;
        tbp->c_count--;
        break;

    case T_SUSPEND:
        tp->t_state |= TTSTOP;
        break;

    case T_BLOCK:
        tp->t_state &= ~TTXON;
        tp->t_state |= TBLOCK;
        if (tp->t_state & BUSY)
            tp->t_state |= TTXOFF;
        else {
            tp->t_state |= BUSY;
            mp->udr = 0x13; /*xoff*/
        }
        break;

    case T_RFLUSH:
        if ((tp->t_state & TBLOCK)==0)
            return;
        /*FALLTHRU*/
    case T_UNBLOCK:
        tp->t_state &= ~(TTXOFF|TBLOCK);
        if (tp->t_state & BUSY)
            tp->t_state |= TTXON;
        else {
            tp->t_state |= BUSY;
            mp->udr = 0x11; /*xon*/
        }
        break;

    case T_BREAK:
        mp->tsr |= MFPTS_B;
        tp->t_state |= TIMEOUT;
        timeout(ttrstrt, tp, hz / 4);
        break;

    case T_PARM:
        mfpparam(dev); /* dev is not initialized! */
        break;
    }
}
