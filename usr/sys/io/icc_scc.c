/* PCS specific */
static char *_Version = "@(#) RELEASE:  2.2  Feb 23 1987 /usr/sys/io/icc_scc.c";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/page.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "fcntl.h"

#include "sys/icc/types.h"
#include "sys/icc/pikitor.h"
#include "sys/icc/icc.h"
#include "sys/icc/scc.h"

/* note: "unit" is 0 or 1, whereas "dev" is the dev# made by mknod */
int scc_cnt = 2;

struct tty scc_tty[2];

/* this is a block of data returned from ICC for SCC
 * this is the size of a UNIX_MESSAGE */
struct sccmsg {
    struct unix_message *link;
    long sender;
    short flag;
    char id[2];
    char datasz;
    char data[6*sizeof(int)-1]; /*sizeof data block in UNIX_MESSAGE*/
};

#define SCC_POOL_SIZE 16
static struct scctab {
    short woff0;
    int idx;
    struct tty *tp;
    UNIX_MESSAGE um[SCC_POOL_SIZE];
} scc_tab[2];


extern int icc_initflg[];
extern sccproc();
UNIX_MESSAGE *scc_get();
extern ttrstrt();

sccopen(unit, mode)
{
    register struct tty *tp;
    register UNIX_MESSAGE *up;
    uint res;

    if (unit >= 2 || icc_initflg[0] == 0) {
        u.u_error = ENXIO;
        return;
    }

    up = (UNIX_MESSAGE*)scc_get(&scc_tab[unit]);
    tp = &scc_tty[unit];

    if ((tp->t_state & (WOPEN|ISOPEN))==0) {
        scc_tab_init(unit);
        scc_tab[unit].tp = &scc_tty[unit];
        ttinit(tp);
        tp->t_proc = sccproc;
        scc_tab[unit].woff0 = 0;
        scc_tab[unit].idx = 0;
    }

    spltty();
    up->data[0] = mode;
    up->data[1] = tp->t_iflag;
    up->data[2] = tp->t_oflag;
    up->data[3] = tp->t_cflag;
    up->data[4] = tp->t_lflag;
    up->data[5] = tp->t_line;
    up->link = 0;
    Rpc(unit>>1, (unit & 1)+SCCA, 1, up, WAIT);
    res = (ushort)up->data[0];

    if ((tp->t_cflag & CLOCAL) || (res & 8)) /* result bit 3 is DCD status */
        tp->t_state |= CARR_ON;
    else
        tp->t_state &= ~CARR_ON;
    
    if ((mode & O_NDELAY)==0) {
        while ((tp->t_state & CARR_ON)==0) {
            tp->t_state |= WOPEN;
            sleep(&tp->t_canq, 28);
        }
    }
    (*linesw[tp->t_line].l_open)(tp);
    spl0();
}

sccclose(unit)
{
    register struct tty *tp = &scc_tty[unit];
    register UNIX_MESSAGE *up;

    (*linesw[tp->t_line].l_close)(tp);

    if ((tp->t_cflag & HUPCL) && (tp->t_state & ISOPEN)==0) {
        up = scc_get(&scc_tab[unit]);
        up->link = 0;
        Rpc(unit >> 1, (unit & 1)+SCCA, 2, up, NOWAIT);
    }
}

sccread(unit)
{
    register struct tty *tp = &scc_tty[unit];
    (*linesw[tp->t_line].l_read)(tp);
}

sccwrite(unit)
{
    register struct tty *tp = &scc_tty[unit];
    (*linesw[tp->t_line].l_write)(tp);
}

sccrint(tp, ch)
register struct tty *tp;
register ch;
{
    sysinfo.rcvint++;

    if ((tp->t_state & ISOPEN)==0)
        return;
    
    if (tp->t_rbuf.c_ptr == 0)
        return;

    if (tp->t_iflag & ISTRIP)
        ch &= 0x7f;
    else
        ch &= 0xff;

    *tp->t_rbuf.c_ptr = ch;
    tp->t_rbuf.c_count--;
    (*linesw[tp->t_line].l_input)(tp, L_BUF);
}

scceint(tp, ch)
register struct tty *tp;
register ch;
{
    sysinfo.mdmint++;

    if (ch & 0x80) {
        if ((tp->t_iflag & IGNBRK)==0 && (tp->t_iflag & BRKINT))
            (*linesw[tp->t_line].l_input)(tp, L_BREAK);
    }

    if (tp->t_cflag & CLOCAL)
        return;
    
    if (ch & 8) {
        if ((tp->t_state & CARR_ON)==0) { /* DCD detected */
            wakeup(&tp->t_canq);
            tp->t_state |= CARR_ON;
        }
    }
    
    if ((ch & 8)==0) {
        if (tp->t_state & CARR_ON) {
            if (tp->t_state & ISOPEN) {
                signal(tp->t_pgrp, SIGHUP);
                ttyflush(tp, 3);
            }
            tp->t_state &= ~CARR_ON;
        }
    }
}

sccsint(tp, ch, res)
register struct tty *tp;
register res, ch;
{
    register iflag;
    char out[3];
    short cnt;

    sysinfo.rcvint++;

    if ((tp->t_state & ISOPEN)==0)
        return;

    if (tp->t_rbuf.c_ptr==0)
        return;

    cnt = 1;
    iflag = tp->t_iflag;
    if ((res & 0x40) && (iflag & INPCK)==0) /* break bit set but no check */
        res &= ~0x40;                       /* ignore break */

    if ((res & 0x60)) {                     /* another error as well? */
        if (res & 0x40) {                   /* break bit set */
            if (iflag & IGNBRK)
                return;
            if (iflag & BRKINT) {
                (*linesw[tp->t_line].l_input)(tp, L_BREAK);
                return;
            }
        } else if (iflag & IGNPAR)
            return;

        /* res has still 0x40 set, but is not ignored or processed yet */

        /* mark parity errors? */
        if ((res & 0x40) && (iflag & PARMRK)) {
            out[2] = 0xff;
            out[1] = 0;
            cnt = 3;
            sysinfo.rawch += 2;
        } else
            ch = 0;
        out[0] = ch;
        while (cnt) {
            cnt--;
            *tp->t_rbuf.c_ptr = out[cnt];
            tp->t_rbuf.c_count--;
            (*linesw[tp->t_line].l_input)(tp, L_BUF);
        }
    }
}

/* devmajor should be 0 */
scc_intr(devmajor, up)
UNIX_MESSAGE *up;
{
    register char *cp;
    register struct tty *tp;
    register ch;
    int unit;
    int sz;
    
    unit = up->id[0] - SCCA + devmajor;
    tp = &scc_tty[unit];
    sz = ((struct sccmsg*)up)->datasz;
    for (cp = ((struct sccmsg*)up)->data; sz > 0; sz -= 2) {
        ch = *cp++;
        if (ch == R_INTR)
            sccrint(tp, *cp++);
        else if (ch==S_INTR) {
            sccsint(tp, *cp++, *cp++);
            sz--;
        } else if (ch == E_INTR)
            scceint(tp, *cp++);
        else if (ch == SCC_ACK) {
            cp++;
            tp->t_state &= ~BUSY;
            sccproc(tp, T_OUTPUT);
        }
    }
}

sccioctl(unit, func, arg, mode)
{
    register struct tty *tp = &scc_tty[unit];
    if (func == TCCLR && suser())
        scc_clr(tp, unit);
    else if (ttiocom(tp, func, arg, mode) != 0)
        scc_param(unit, func);
}

scc_param(unit, func)
{
    register struct tty *tp = &scc_tty[unit];
    register UNIX_MESSAGE *up = scc_get(&scc_tab[unit]);

    up->data[0] = func;
    up->data[1] = tp->t_iflag;
    up->data[2] = tp->t_oflag;
    up->data[3] = tp->t_cflag;
    up->data[4] = tp->t_lflag;
    up->data[5] = tp->t_line;
    up->link = 0;
    Rpc(unit >> 1, (unit & 1)+SCCA, 3, up, WAIT); 
}

sccproc(tp, cmd)
register struct tty *tp;
{
    register caddr_t wp;
    register caddr_t cp;
    register cnt;

    UNIX_MESSAGE *bp;
    int i;
    struct scctab *stp;
    int unit;
    struct sccmsg *up;
    
    unit = tp - scc_tty;
    stp = &scc_tab[unit];

    switch (cmd) {
    case T_TIME:
        tp->t_state &= ~TIMEOUT;
        goto out;

    case T_RESUME:
    case T_WFLUSH:
        tp->t_state &= ~TTSTOP;
        /*FALLTHRU*/

    case T_OUTPUT:
out:
        if (tp->t_state & (TTSTOP|BUSY))
            break;
        if (((*linesw[tp->t_line].l_output)(tp) & CPRES)==0)
            break;
        cnt = tp->t_tbuf.c_count;
        tp->t_tbuf.c_count = 0;
        cp = tp->t_tbuf.c_ptr;
        tp->t_state |= BUSY;
        up = (struct sccmsg*)scc_get(stp);
        up->datasz = 1;
        for (;;) {
            up->link = 0;
            i = 20; /* max# of chars to put in buffer without reserve of 3 */
            wp = &up->data[1];
            if (cnt < i)
                i = cnt;
            
            cnt = cnt - i;
            up->data[0] = i;
            while (i-- != 0)
                *wp++ = *cp++;
            up->id[0] = (unit & 1)+SCCA;
            up->id[1] = SCC_DATA;
            Send_icc(unit >> 1, up);
            if (cnt == 0)
                return;
            up = (struct sccmsg*)scc_get(stp);
            up->datasz = 0;
        }
        /*NOTREACHED*/
    case T_SUSPEND:
        tp->t_state |= TTSTOP;
        break;

    case T_BREAK:
        tp->t_state |= TIMEOUT;
        timeout(ttrstrt, tp, hz/4);
        break;

    case T_PARM:
        scc_param(unit, TCSETA);
        break;

    case T_BLOCK:
        tp->t_state &= ~TTXON;
        tp->t_state |= TBLOCK;
        bp = scc_get(stp);
        bp->link = 0;
        bp->id[0] = (unit & 1)+SCCA;
        bp->id[1] = SCC_PROC;
        bp->data[0] = cmd;
        Send_icc(unit>>1, bp);
        break;

    case T_UNBLOCK:
        tp->t_state &= ~(TTXOFF|TBLOCK);
        bp = scc_get(stp);
        bp->link = 0;
        bp->id[0] = (unit & 1)+SCCA;
        bp->id[1] = SCC_PROC;
        bp->data[0] = cmd;
        Send_icc(unit>>1, bp);
        break;
    }
}

scc_clr(tp, unit)
register struct tty *tp;
{
    UNIX_MESSAGE *up;

    if ((tp->t_state & (WOPEN|ISOPEN))==0)
        return;

    tp->t_state = ~BUSY;
    up = scc_get(&scc_tab[unit]);
    up->link = 0;
    Rpc(unit>>1, (unit & 1)+SCCA, SCC_CLR, up, NOWAIT); 
}

static UNIX_MESSAGE *scc_get(stp)
register struct scctab *stp;
{
    int s = currpl();
    spltty();

    /* spin until a slot is free */
    for(;;) {
        stp->idx++;
        if (stp->idx >= SCC_POOL_SIZE)
            stp->idx = 0;
        if (stp->um[stp->idx].link==0) {
            splx(s);
            return &stp->um[stp->idx];
        }
    }
}

static scc_tab_init(unit)
{
    register UNIX_MESSAGE *up;
    register struct scctab *stp = &scc_tab[unit];
    register i;
    
    stp->idx = 0;

    for (i=0, up = &stp->um[0]; i < SCC_POOL_SIZE; up++, i++) {
        up->link = 0;
        up->flag = 0;
        up->id[1] = SCC_DATA;
        up->id[0] = SCCA;
    }
}
