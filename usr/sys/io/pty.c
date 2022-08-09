/* PCS specific */
static char* _Version = "@(#) RELEASE:  2.8  Nov 09 1987 /usr/sys/io/pty.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/tty.h"
#include "sys/ttold.h"
#include "sys/proc.h"
#include "sys/file.h"
#include "sys/conf.h"
#include "sys/termio.h"
#include "sys/sysinfo.h"
#include "sys/sel.h"
#include "sys/pty.h"
#include "fcntl.h"

extern int pt_cnt; /*c.c*/
extern int sxtline; /*linesw.c*/
extern struct tty pt_tty[];

extern ptsstart(), ttrstrt();

ptsopen(dev)
{
    register struct tty *tp;
    
    if (dev >= pt_cnt) {
        u.u_error = ENXIO;
        return;
    }
    tp = &pt_tty[dev];
    tp->t_proc = ptsstart;

    if ((tp->t_state & (WOPEN|ISOPEN))==0) {
        tp->t_state &= ~BUSY;
        ttinit(tp);
    }
    while ((tp->t_state & CARR_ON)==0) {
        tp->t_state |= WOPEN;
        sleep(&tp->t_canq, 28);
    }
    (*linesw[tp->t_line].l_open)(tp);
}

ptsclose(dev)
{
    register struct tty *tp = &pt_tty[dev];
    if ((tp->t_state & CARR_ON)==0)
        ttyflush(tp, FWRITE);

    spltty();
    (*tp->t_proc)(tp, T_RESUME);
    spl0();
    ttywait(tp);
    ttyflush(tp, (FREAD|FWRITE));
    (*linesw[tp->t_line].l_close)(tp);
    wakeup(&tp->t_outq.c_cf);
    if (tp->t_tmflag & TERM_CTLECHO) {
        tp->t_tmflag &= ~TERM_CTLECHO;
        wakeup(&nselect);
    }
    tp->t_tmflag |= TMPSCLOSE;
}

ptsread(dev)
{
    register struct tty *tp = &pt_tty[dev];
    (*linesw[tp->t_line].l_read)(tp);
    if (tp->t_state & CARR_ON) {
        wakeup(&tp->t_rawq.c_cf);
        if (tp->t_tmflag & LCF) {
            tp->t_tmflag &= ~LCF;
            wakeup(&nselect);
        }
    }
}

ptswrite(dev)
{
    register struct tty * tp = &pt_tty[dev];
    (*linesw[tp->t_line].l_write)(tp);
}

ptsstart(tp, cmd)
register struct tty *tp;
{
    register i;

    int flag = 0;
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
        if ((tp->t_state & (TTSTOP|BUSY|TIMEOUT)) == 0)
            flag++;
        break;

    case T_SUSPEND:
        tp->t_state |= TTSTOP;
        break;

    case T_BLOCK:
        tp->t_state &= ~TTXON;
        tp->t_state |= (TTXOFF|TBLOCK);
        flag++;
        break;

    case T_RFLUSH:
        if ((tp->t_state & TBLOCK)==0)
            break;
        /*FALLTHRU*/
    case T_UNBLOCK:
        tp->t_state &= ~(TTXOFF|TBLOCK);
        tp->t_state |= TTXON;
        flag++;
        break;

    case T_BREAK:
        tp->t_state |= TIMEOUT;
        timeout(ttrstrt, tp, hz / 4);
        break;

    case T_PARM:
        flag = 1;
        tp->t_tmflag |= TMPTYIOCTLSL;
        break;
    }
    
    if (flag==0)
        return;
    
    if ((tp->t_state & CARR_ON)==0)
        return;

    if (tp->t_proc != ptsstart) {
        for (i=0; i<pt_cnt; i++) {
            if (pt_tty[i].t_state == CARR_ON) {/*really? not & CARR_ON? */

                if (pt_tty[i].t_outq.c_cc || pt_tty[i].t_tbuf.c_count > 0) {
                    wakeup(&pt_tty[i].t_outq.c_cf);
                    if (pt_tty[i].t_tmflag & TERM_CTLECHO) {
                        pt_tty[i].t_tmflag &= ~TERM_CTLECHO;
                        wakeup(&nselect);
                    }
                }
            }
        }
    }
    
    wakeup(&tp->t_outq.c_cf);
    if (tp->t_tmflag & TERM_CTLECHO) {
        tp->t_tmflag &= ~TERM_CTLECHO;
        wakeup(&nselect);
    }
}

ptcopen(dev)
{
    register struct tty *tp;

    if (dev >= pt_cnt) {
        u.u_error = ENXIO;
        return;
    }

    tp = &pt_tty[dev];
    if (tp->t_state & CARR_ON) {
        u.u_error = EIO;
        return;
    }

    tp->t_proc = ptsstart;
    tp->t_state |= (CARR_ON|BUSY);
    if (tp->t_state & WOPEN) {
        tp->t_state &= ~WOPEN;
        wakeup(&tp->t_canq);
    }
    tp->t_tmflag = 0;
}

ptcclose(dev)
{
    register struct tty *tp = &pt_tty[dev];
    
    if (tp->t_state & ISOPEN) {
        signal(tp->t_pgrp, SIGHUP);
        ttyflush(tp, (FREAD|FWRITE));
    }
    tp->t_state &= ~(EXTPROC|BUSY|CARR_ON);
    tp->t_tmflag = 0;
}

ptcread(dev)
{
    register struct tty *tp;
    register ch;
    int flag = 1;
    
    tp = &pt_tty[dev];
    if ((tp->t_tmflag & (TMPTYNEW|TMPTYIOCTLSL)) == (TMPTYNEW|TMPTYIOCTLSL)) {
        tp->t_tmflag &= ~TMPTYIOCTLSL;
        u.u_error = EAGAIN;
        return;
    }

    do {
        if (tp->t_state & TTXON) {
            tp->t_state &= ~TTXON;
            if (passc(0x11) < 0) /*XON*/
                return;
            flag = 0;
        } else if (tp->t_state & TTXOFF) {
                tp->t_state &= ~TTXOFF;
                if (passc(0x13) < 0) /*XOFF*/
                    return;
                flag = 0;
        } else if ((tp->t_state & TTIOW) && tp->t_outq.c_cc==0) {
            tp->t_state &= ~TTIOW;
            wakeup(&tp->t_oflag);
        } else {
redo:
            if ((tp->t_proc != ptsstart || 
                tp->t_line == sxtline) && tp->t_outq.c_cc == 0) {
                  
                if (tp->t_tbuf.c_ptr==0 || tp->t_tbuf.c_count <= 0) {
                    if (tp->t_tbuf.c_ptr)
                    tp->t_tbuf.c_ptr -= (tp->t_tbuf.c_size - tp->t_tbuf.c_count);
                        (*linesw[tp->t_line].l_output)(tp);
                }
        
                while (tp->t_outq.c_cc < tthiwat[tp->t_cflag & CBAUD] &&
                    tp->t_tbuf.c_count != 0) {
                        putc(*tp->t_tbuf.c_ptr++, &tp->t_outq);
                        tp->t_tbuf.c_count--;
                }
            }

            while ((tp->t_state & ISOPEN) && 
                   (tp->t_outq.c_cc==0 || tp->t_state & (TTSTOP|TIMEOUT))) {
                tp->t_state &= ~BUSY;
                if (u.u_fmode & O_NDELAY) return;
                sleep(&tp->t_outq.c_cf, 28);
                if ((tp->t_tmflag & (TMPTYNEW|TMPTYIOCTLSL)) == (TMPTYNEW|TMPTYIOCTLSL)) {
                    tp->t_tmflag &= ~TMPTYIOCTLSL;
                    u.u_error = EAGAIN;
                    return;
                }
                goto redo;
            }
        }
        
        if ((tp->t_state & ISOPEN)==0)
            return;
        
        tp->t_state |= BUSY;
        while ((ch=getc(&tp->t_outq)) >= 0) {
            if ((tp->t_oflag & OPOST) && ch == 0x80) {
                if ((ch=getc(&tp->t_outq)) < 0)
                    break;
                if (ch > 0x80) {
                    tp->t_state |= TIMEOUT;
                    timeout(ttrstrt, tp, (ch & 0x7f) + 6);
                    break;
                }
            }
            flag = 0;
            if (passc(ch) < 0)
                break;
        }

        if (tp->t_state & OASLP) {
            if (tp->t_outq.c_cc <= ttlowat[tp->t_cflag & CBAUD]) {
                tp->t_state &= ~OASLP;
                wakeup(&tp->t_outq);
            }
        }
        if ((tp->t_state & TTIOW) && tp->t_outq.c_cc==0) {
            tp->t_state &= ~(TTIOW|BUSY);
            wakeup(&tp->t_oflag);
        }
    } while (flag);
}

ptcwrite(dev)
{
    register struct tty *tp = &pt_tty[dev];
    register char *bp;
    register char *bpend;
    register cnt;
    register char ch;
    
    char buf[100];
    short iflag;
    short lflag;

    if ((tp->t_state & ISOPEN)==0)
        return;

    spltty();
    iflag = tp->t_iflag;
    lflag = tp->t_lflag;
    
    if (tp->t_tmflag & TMPTYNEW) {
        tp->t_iflag = iflag & (ushort)(IXON|ISTRIP);
        tp->t_lflag = lflag & (ushort)~(ECHONL|ECHOK|ECHOE|ECHO|XCASE);
    }

    while (u.u_count) {
        cnt = (u.u_count < 100) ? u.u_count : 100;
        bp = buf;
        iomove(bp, cnt, 0);
        if (u.u_error) break;

        for (bpend = bp+cnt; bp < bpend; ) {
            if (tp->t_iflag & IXON) {
                while (tp->t_delct && tp->t_rawq.c_cc >= 254) {
                    tp->t_iflag = iflag;
                    tp->t_lflag = lflag;
                    wakeup(&tp->t_rawq);
                    sleep(&tp->t_rawq.c_cf, 29);
                    iflag = tp->t_iflag;
                    lflag = tp->t_lflag;
                    if (tp->t_tmflag & TMPTYNEW) {
                        tp->t_iflag = iflag & (ushort)(IXON|ISTRIP);
                        tp->t_lflag = lflag & (ushort)~(ECHONL|ECHOK|ECHOE|ECHO|XCASE);
                    }
                }
            }
            if (tp->t_iflag & IXON) {
                ch = *bp & 0x7f;
                if (tp->t_state & TTSTOP) {
                    if (ch == 0x11 || (tp->t_iflag & IXANY)) /*XON*/
                        (*tp->t_proc)(tp, T_RESUME);
                } else if (ch == 0x13) /*XOFF*/
                    (*tp->t_proc)(tp, T_SUSPEND);
                
                if (ch== 0x11 || ch == 0x13) {
                    bp++;
                    continue;           
                }
            } 
            *tp->t_rbuf.c_ptr = *bp++;
            tp->t_rbuf.c_count--;
            (*linesw[tp->t_line].l_input)(tp, L_BUF);
        }
    }

    spl0();
    tp->t_iflag = iflag;
    tp->t_lflag = lflag;
}

ptsioctl(dev, func, arg, mode)
{
    switch (func) {
    case TCSETA:
    case TCSETAW:
    case TCSETAF:
    case TIOCSETP:
        ttiocom(&pt_tty[dev], func, arg, mode);
        wakeup(&pt_tty[dev].t_outq.c_cf);
        pt_tty[dev].t_tmflag |= TMPTYIOCTLSL;
        break;
    default:
        ttiocom(&pt_tty[dev], func, arg, mode);
        break;
    }
}

ptcioctl(dev, func, arg, mode)
{
    switch(func) {
    case TCSETAW:
    case TCSETAF:
    case TIOCSETP:
        ttyflush(&pt_tty[dev], FWRITE);
        /*FALLTHRU*/
    default:
        ttiocom(&pt_tty[dev], func, arg, mode);
        break;
    case TCPTYDUMPSL:
        copyout(&pt_tty[dev], arg, sizeof(struct tty));
        break;
    case TCPTYISSLOPEN:
        delay(hz >> 1);
        suword(arg, (short)((pt_tty[dev].t_state & ISOPEN) != 0));
        break;
    case TCPTYSETNEWMODE:
        pt_tty[dev].t_tmflag |= TMPTYNEW;
        pt_tty[dev].t_state |= EXTPROC;
        break;
    case TCPTYRESETNEWMODE:
        pt_tty[dev].t_tmflag &= ~TMPTYNEW;
        pt_tty[dev].t_state &= ~EXTPROC;
        break;
    }
}

ptc_sel(dev, nfds, readfd, writefd, exceptfd)
dev_t dev;
{
    register ret;
    struct tty *tp = &pt_tty[dev];

    ret = 0;
    
    if (nfds == -1)
        tp->t_tmflag &= ~(TMPTYRSEL|TMPTYWSEL);
    else {
        if (readfd) {
            if (tp->t_outq.c_cc==0) {
                if (tp->t_tbuf.c_ptr==0 || tp->t_tbuf.c_count <=0) {
                    if (tp->t_tbuf.c_ptr)
                        tp->t_tbuf.c_ptr -= (tp->t_tbuf.c_size - tp->t_tbuf.c_count);
                    (*linesw[tp->t_line].l_output)(tp);
                }
            }
            if (tp->t_outq.c_cc && (tp->t_state & (TTSTOP|TIMEOUT))==0)
                ret |= 1;
            else {
                tp->t_state &= ~BUSY;
                if (nfds)
                    tp->t_tmflag |= TMPTYRSEL;
            }
        }
        if (writefd) {
            if ((tp->t_iflag & IXON) && tp->t_delct && tp->t_rawq.c_cc >= 254) {
                if (nfds)
                    pt_tty[dev].t_tmflag |= TMPTYWSEL;
            } else
                ret |= 2;
        }
        if (exceptfd) {
            if ((tp->t_state & (CARR_ON|ISOPEN)) != (CARR_ON|ISOPEN))
                ret |= 4;
        }
    }
    return ret;
}
