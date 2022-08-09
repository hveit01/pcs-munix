/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.3  Sep 14 1987 /usr/sys/io/sxt.c ";

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
#include "sys/ttold.h"
#include "sys/ioctl.h"
#include "sys/sel.h"
#include "sys/conf.h"
#include "sys/sxt.h"
#include "fcntl.h"

/* only defined in sgtty, but needed here */
#define TIOCEXCL (tIOC|13)
#define TIOCNXCL (tIOC|14)

Link_p linkTable[MAXLINKS];
char sxtbusy[MAXLINKS];

/* see space.h, should be actually declared in sxt.h */
#define LINKSIZE (uint)(sizeof(struct Link) + sizeof(struct Channel)*(MAXPCHAN-1))

caddr_t sxtbuf;
extern int sxt_cnt; /*space.h*/
extern char sxt_buf[]; /*space.h*/

extern sxtnullproc(), sxtvtproc(), sxtin();
extern Link_p sxtalloc();


sxtopen(dev, mode)
{
    register Link_p lp;
    register struct tty *tp;
    register chan = CHAN(dev);
    register bit;
    register s;

    lp = linkTable[LINK(dev)];
    if (lp == (Link_p)1) {
        u.u_error = EBUSY;
        return;
    }

    if (lp == 0) {
        if (chan != 0)
            u.u_error = EINVAL;
        else {
            linkTable[LINK(dev)] = (Link_p)1;
        }
        return;
    }
        
    if (chan == 0) {
        u.u_error = EBUSY;
        return;
    }

    if (chan >= lp->nchans) {
        u.u_error = ENXIO;
        return;
    }

    bit = 1 << chan;
    if (lp->open & bit) {
        if ((mode & O_EXCL) || (lp->xopen & bit)) {
            u.u_error = ENXIO;
            return;
        }
    } else {
        lp->open |= bit;
        if (lp->chans[chan].tty.t_proc == sxtnullproc ||
                lp->chans[chan].tty.t_proc == sxtvtproc)
            ttyflush(&lp->chans[chan].tty, FREAD|FWRITE);

        bzero(&lp->chans[chan], sizeof(struct Channel));
        sxtlinit(&lp->chans[chan].tty, chan, LINK(dev), lp->old, lp->line);
        if (mode & O_EXCL)
            lp->xopen |= bit;
    }

    tp = &lp->chans[chan].tty;

    s = spltty();
      if (u.u_procp->p_pid == u.u_procp->p_pgrp &&
              u.u_ttyp == 0 && tp->t_pgrp == 0) {
          u.u_ttyp = &tp->t_pgrp;
          tp->t_pgrp = u.u_procp->p_pgrp;
      }
      tp->t_state |= ISOPEN;
    splx(s);
}

sxtclose(dev)
{
    register Link_p lp;
    register struct tty *tp;
    register (*func)();
    register int chan;
    int s;
    int i;

    lp = linkTable[LINK(dev)];
    if (lp == (Link_p)1) {
        linkTable[LINK(dev)] = 0;
        return;
    }

    chan = CHAN(dev);
    tp = &lp->chans[chan].tty;
    tp->t_state &= ~(BUSY|TIMEOUT);
    linesw[tp->t_line].l_close(tp);
    tp->t_pgrp = 1; /* init process */
    tp->t_state &= ~CARR_ON;
    chan = (1 << chan);
    lp->xopen &= ~chan;
    lp->open &= ~chan;

    s = spltty();
    
    if (chan == 1) {
        lp->controllingtty = 0;
        lp->line->t_pgrp = u.u_procp->p_pgrp;
        lp->line->t_line = lp->old;
        lp->line = tp;
        func = sxtnullproc;
        tp->t_proc = func;
        for (i = 1; i < lp->nchans; i++) {
            tp = &lp->chans[i].tty;
            tp->t_pgrp = 1;
            if (tp->t_proc == sxtnullproc || tp->t_proc == sxtvtproc)
                ttyflush(tp, FREAD|FWRITE);
        }
    }
    
    if (lp->open == 0) {
        linkTable[LINK(dev)] = 0;
        sxtfree(lp);
    }
    splx(s);
}

sxtioctl(dev, func, arg, mode)
{
    register Link_p lp;
    register struct tty *tp;
    register struct tty *tp2;
    register i;
    struct termio tio;
    struct sgttyb sg;
    struct sxtblock sxtb;
    int sgflags;
    int uarg;
    int unused;
    int s;
    int k;
    char linesave;

    uarg = ((ushort*)&arg)[1]; /* interpret arg as ushort */
    lp = linkTable[LINK(dev)];

    if (lp == (Link_p)1 && func != SXTIOCLINK) {
        u.u_error = ENXIO;
        return;
    }

    switch (func) {
    case SXTIOCLINK:
        if (uarg > 8 || uarg < 1) {
            u.u_error = EINVAL;
            return;
        }

        tp = cdevsw[bmajor(u.u_ttyd)].d_ttys;
        if (tp == 0) {
            u.u_error = ENOTTY;
            return;
        }

        tp += minor(u.u_ttyd);
        for (i = 0; i < linecnt; i++) {
            if (linesw[i].l_input == sxtin)
                break;
        }
        if (i == linecnt) {
            u.u_error = ENXIO;
            return;
        }

        if (lp == 0) {
            u.u_error = EBADF;
            return;
        }
        if (lp != (Link_p)1) {
            u.u_error = EBUSY;
            return;
        }
        
        lp = sxtalloc(uarg);
        if (lp == 0) {
            u.u_error = ENOMEM;
            return;
        }

        ttyflush(tp, FREAD|FWRITE);
        lp->dev = u.u_ttyd;
        lp->controllingtty = 0;
        lp->lwchan = 0;
        lp->wpending = 0;
        lp->wrcnt = 0;
        lp->line = tp;
        lp->old = tp->t_line;
        lp->nchans = uarg;
        lp->chanmask = 0xff;

        for (k = lp->nchans; k < 8; k++)
            lp->chanmask >>= 1;

        lp->open = lp->xopen = 1;
        sxtlinit(&lp->chans[0].tty, 0, LINK(dev), lp->old, tp);

        s = spltty();
          linkTable[LINK(dev)] = lp;
          tp->t_line = i;
          tp->t_link = LINK(dev);

          tp2 = &lp->chans[0].tty;
          if (u.u_procp->p_pid == u.u_procp->p_pgrp &&
              u.u_ttyp == 0 && tp2->t_pgrp==0) {
              u.u_ttyp = &tp2->t_pgrp;
                  tp2->t_pgrp = u.u_procp->p_pgrp;
          }
          tp2->t_state |= ISOPEN;
        splx(s);
        break;

    case SXTIOCSWTCH:
        if (lp==(Link_p)1 || lp==0) {
            u.u_error = EINVAL;
            return;
        }
        if (((1 << uarg) & lp->open)==0) {
            u.u_error = EINVAL;
            return;
        }
        s = spltty();
          if (CHAN(dev) != 0) {
              u.u_error = EPERM;
              splx(s);
              return;
          }
        
          if (uarg != lp->controllingtty) {
              lp->controllingtty = uarg;
              if (uarg != 0) {
                  ttywait(&lp->chans[0].tty);
                  tp = lp->line;
                  tp2 = &lp->chans[uarg].tty;
                  tp->t_iflag = tp2->t_iflag;
                  tp->t_oflag = tp2->t_oflag;
                  tp->t_cflag = tp2->t_cflag |(tp->t_cflag & (TTSEL|TTYSEL));
                  tp->t_lflag = tp2->t_lflag;
                  bcopy(tp2->t_cc, tp->t_cc, NCC);
                  tp->t_pgrp = tp2->t_pgrp;
                  tp->t_term = tp2->t_term;
                  (*tp->t_proc)(tp, T_PARM);
                  if (tp2->t_cflag & TTYSEL) {
                      tp2->t_cflag &= ~TTYSEL;
                      wakeup(&nselect);
                  }
              }
          }
        splx(s);
        wakeup(&lp->chans[uarg].tty);
        break;

    case SXTIOCWF:
        if (lp==(Link_p)1 || lp==0) {
            u.u_error = EINVAL;
            return;
        }
        if (((1 << uarg) & lp->open)==0) {
            u.u_error = EINVAL;
            return;
        }
        
        if (uarg == lp->controllingtty)
            return;

        s = spltty();
          while (uarg != lp->controllingtty)
              sleep(&lp->chans[uarg].tty, NINTER);
          lp->line->t_pgrp = lp->chans[uarg].tty.t_pgrp;
          lp->line->t_term = lp->chans[uarg].tty.t_term;
        splx(s);
        break;
    
    case SXTIOCBLK:
        if (lp==(Link_p)1 || lp==0) {
            u.u_error = EINVAL;
            return;
        }
        if (((1 << uarg) & lp->open)==0) {
            u.u_error = EINVAL;
            return;
        }
        tp2 = &lp->chans[uarg].tty;
        tp2->t_cflag |= TOSTOP;
        break;

    case SXTIOCUBLK:
        if (lp==(Link_p)1 || lp==0) {
            u.u_error = EINVAL;
            return;
        }
        if (((1 << uarg) & lp->open)==0 || uarg==0) {
            u.u_error = EINVAL;
            return;
        }
        tp2 = &lp->chans[uarg].tty;
        tp2->t_cflag &= ~TOSTOP;
        wakeup(&lp->chans[uarg].tty);
        if (tp2->t_cflag & TTYSEL) {
            tp2->t_cflag &= ~TTYSEL;
            wakeup(&nselect);
        }
        break;

    case SXTIOCSTAT:
        if (lp==(Link_p)1 || lp==0) {
            u.u_error = EINVAL;
            return;
        }

        sxtb.input  = lp->iblocked;
        sxtb.output = lp->oblocked;
        if (copyout(&sxtb, arg, sizeof(struct sxtblock)) != 0) {
            u.u_error = EFAULT;
            return;
        }
        break;

    case TIOCEXCL:
        lp->xopen |= (1 << CHAN(dev));
        break;
    
    case TIOCNXCL:
        lp->xopen &= ~(1 << CHAN(dev));
        break;
    
    case TCGETA:
    case FIONREAD:
    case TIOCGETP:
        ttiocom(&lp->chans[CHAN(dev)], func, arg, mode);
        break;

    case TIOCSETP:
    case TIOCSETN:
        ttiocom(&lp->chans[CHAN(dev)].tty, func, arg, mode);
        if (CHAN(dev) == lp->controllingtty) {
            tp = lp->line;
            if (copyin(arg, &sg, sizeof(struct sgttyb)) != 0) {
                u.u_error = EFAULT;
                return;
            }
            tp->t_iflag = 0;
            tp->t_oflag = 0;
            tp->t_lflag = 0;
            tp->t_cflag = (sg.sg_ispeed & CBAUD) | CREAD | 
                (tp->t_cflag & (TTYSEL|TTSEL|CLOCAL));
            if ((sg.sg_ispeed & CBAUD) == B110)
                tp->t_cflag |= CSTOPB;
            tp->t_cc[2] = sg.sg_erase;
            tp->t_cc[3] = sg.sg_kill;
            if ((sgflags=sg.sg_flags) & O_TANDEM)
                tp->t_cflag |= TOSTOP;
            if (sgflags & O_XTABS)
                tp->t_oflag |= TAB3;
            else if (sgflags & (O_TB1|O_TB2))
                tp->t_oflag |= TAB1;
            if (sgflags & O_LCASE) {
                tp->t_iflag |= IUCLC;
                tp->t_oflag |= OLCUC;
                tp->t_lflag |= XCASE;
            }
            if (sgflags & O_ECHO)
                tp->t_lflag |= (ECHO|ECHOE|ECHOK);
            if (sgflags & O_CRMOD) {
                tp->t_iflag |= ICRNL;
                tp->t_oflag |= ONLCR;
                if (sgflags & O_CR1)
                    tp->t_oflag |= CR1;
                if (sgflags & O_CR2)
                    tp->t_oflag |= (CR2|ONOCR);
            } else {
                tp->t_oflag |= ONLRET;
                if (sgflags & O_NL1)
                    tp->t_oflag |= CR1;
                if (sgflags & O_NL2)
                    tp->t_oflag |= CR2;
            }
            if (sgflags & (O_RAW|O_CBREAK)) {
                tp->t_cc[5] = 0;
                tp->t_cc[4] = 1;
                tp->t_cflag |= CS8;
                if (sgflags & O_CBREAK) {
                    tp->t_iflag |= (IXON|ISTRIP|IGNPAR|BRKINT);
                    tp->t_lflag |= ISIG;
                    tp->t_oflag |= OPOST;
                } else
                    tp->t_iflag &= ~(IUCLC|ICRNL);
            } else {
                tp->t_cc[4] = 0x1a;
                tp->t_cc[5] = 0;
                tp->t_cc[6] = 0;
                tp->t_iflag |= (IXANY|IXON|ISTRIP|IGNPAR|BRKINT);
                tp->t_oflag |= OPOST;
                tp->t_cflag |= 
                    ((sgflags & O_ODDP) && (sgflags & O_EVENP)) ? 
                        CS8 : (PARENB|CS7);
                tp->t_lflag |= (ISIG|ICANON);
            }

            tp->t_iflag |= INPCK;
            if ((sgflags & O_ODDP)) {
                if (sgflags & O_EVENP)
                    tp->t_iflag &= ~INPCK;
                else
                    tp->t_cflag |= PARODD;
            }
            if (sgflags & O_VTDELAY)
                tp->t_oflag |= FFDLY;
            if (sgflags & O_BSDELAY)
                tp->t_oflag |= BSDLY;
            (*tp->t_proc)(tp, T_PARM);
        }
        break;

    case TCSETA:
    case TCSETAW:
    case TCSETAF:
        ttiocom(&lp->chans[CHAN(dev)].tty, func, arg, mode);
        if (CHAN(dev) == lp->controllingtty) {
            if (copyin(arg, &tio, sizeof(struct termio)) != 0) {
                u.u_error = EFAULT;
                return;
            }
            linesave = tio.c_line;
            tio.c_line = lp->line->t_line;
            if (copyout(&tio, arg, sizeof(struct termio)) != 0) {
                u.u_error = EFAULT;
                return;
            }
            (*cdevsw[bmajor(lp->dev)].d_ioctl)(minor(lp->dev), TCSETA, arg, mode);
            tio.c_line = linesave;
            if (copyout(&tio, arg, sizeof(struct termio)) != 0) {
                u.u_error = EFAULT;
                return;
            }
        }
        break;

    case TCSBRK:
        ttiocom(&lp->chans[CHAN(dev)].tty, func, arg, mode);
        if (CHAN(dev) == lp->controllingtty && uarg==0)
            (*lp->line->t_proc)(lp->line, T_BREAK);
        break;
    
    case TCXONC:
    case TCFLSH:
        ttiocom(&lp->chans[CHAN(dev)].tty, func, arg, mode);
        break;

    default:
        ttiocom(&lp->chans[CHAN(dev)].tty, func, arg, mode);
        if (CHAN(dev) == lp->controllingtty)
            (*cdevsw[bmajor(lp->dev)].d_ioctl)(minor(lp->dev), func, arg, mode);
    }
}

sxtlinit(tp, chan, link, old, linep)
register struct tty *tp;
char chan, link, old;
register struct tty *linep;
{
    ttinit(tp);
    tp->t_line = old;
    tp->t_proc = sxtvtproc;
    tp->t_link = link;
    tp->t_state |= linep->t_state & ~(TTIOW|IASLP|OASLP|BUSY|ISOPEN);
}

sxtread(dev)
{
    register Link_p lp;
    register struct tty *tp;
    register int chan;
    int s;

    chan = CHAN(dev);
    lp = linkTable[LINK(dev)];

    if (lp == (Link_p)1) {
        u.u_error = ENXIO;
        return;
    }
    if (lp == 0) {
        u.u_error = EBADF;
        return;
    }
    if ((lp->open & (1<<chan)) == 0) {
        u.u_error = EBADF;
        return;
    }

    tp = &lp->chans[chan].tty;
    s = spltty();
      while (lp->controllingtty != chan) {
          lp->iblocked |= (1 << chan);
          sleep(&lp->chans[chan].tty, NINTER);
      }
      lp->iblocked &= ~(1 << chan);
    splx(s);

    (*linesw[tp->t_line].l_read)(tp);
}

sxtwrite(dev)
{
    register Link_p lp;
    register struct tty *tp;
    register int chan;
    int s;

    chan = CHAN(dev);
    lp = linkTable[LINK(dev)];

    if (lp == (Link_p)1) {
        u.u_error = ENXIO;
        return;
    }
    if (lp == 0) {
        u.u_error = EBADF;
        return;
    }
    if ((lp->open & (1<<chan)) == 0) {
        u.u_error = EBADF;
        return;
    }

    chan = CHAN(dev);
    tp = &lp->chans[chan].tty;
    s = spltty();
      while ((tp->t_cflag & TOSTOP) && lp->controllingtty != chan) {
          lp->oblocked |= (1 << chan);
          sleep(&lp->chans[chan].tty, NINTER);
      }
      lp->oblocked &= ~(1 << chan);
    splx(s);

    (*linesw[tp->t_line].l_write)(tp);
}

sxtrwrite(tp)
register struct tty *tp;
{
    register struct tty *tp2;
    register Link_p lp;
    register link;

    for (link = 0; link < MAXLINKS; link++) {
        lp = linkTable[link];
        if (lp != 0 && lp != (Link_p)1 && tp == lp->line)
            break;
    }
            
    if (link != MAXLINKS) {
        tp2 = &lp->chans[lp->controllingtty].tty;
        (*linesw[tp2->t_line].l_write)(tp2);
    }
}

sxtvtproc(tp, func)
register struct tty *tp;
{
    register Link_p lp;
    register chan;
    
    switch (func) {
    default:
        break;
    
    case T_WFLUSH: 
        lp = linkTable[tp->t_link];
        if (lp->controllingtty == (tp - &lp->chans[0].tty))
            (*lp->line->t_proc)(lp->line, T_WFLUSH);
        break;

    case T_RESUME: 
        lp = linkTable[tp->t_link];
        (*lp->line->t_proc)(lp->line, T_RESUME);
        break;

    case T_OUTPUT:
        lp = linkTable[tp->t_link];
        chan = tp - &lp->chans[0].tty;
        lp->wpending |= (1<<chan);
        (*lp->line->t_proc)(lp->line, T_OUTPUT);
        break;

    case T_SUSPEND:
        lp = linkTable[tp->t_link];
        (*lp->line->t_proc)(lp->line, T_SUSPEND);
        break;

    case T_RFLUSH:
        lp = linkTable[tp->t_link];
        if (lp->controllingtty == (tp - &lp->chans[0].tty))
            (*lp->line->t_proc)(lp->line, T_RFLUSH);
        break;

    case T_SWTCH:
        lp = linkTable[tp->t_link];
        lp->controllingtty = 0;
        wakeup(lp->chans);
    }
}

sxtnullproc(tp, func)
register struct tty *tp;
{
    register Link_p lp;
    unsigned char bits;
    int pend = 0;

    if (func != 0)
        return;

    lp = linkTable[tp->t_link];
    for (bits = lp->wpending; (bits >>= 1) != 0; pend++);

    ttyflush(&lp->chans[pend], FWRITE);
    lp->wpending = 0;
}

sxtout(tp)
struct tty *tp;
{
    register Link_p lp = linkTable[tp->t_link];
    register struct tty *tp2;
    register int i, k; /* i is unused */
    unsigned char bits;
    int s;
    int rc;

    s = spltty();

    if (lp->lwchan) {
        for (k = 0,  bits = lp->lwchan; (bits >>= 1) !=0; k++);
        tp2 = &lp->chans[k].tty;
        tp2->t_tbuf = tp->t_tbuf;

        if (lp->wrcnt >= 2 && lp->wpending != lp->lwchan) {
            if (tp2->t_tbuf.c_ptr) {
                putcf(CMATCH((struct cblock*)tp2->t_tbuf.c_ptr));
                tp2->t_tbuf.c_ptr = 0;
                tp2->t_tbuf.c_count = 0;
            }
            tp->t_tbuf = tp2->t_tbuf;
        } else {
            if ((rc = (*linesw[tp2->t_line].l_output)(tp2)) != 0) {
                tp->t_tbuf = tp2->t_tbuf;
                tp2->t_tbuf.c_ptr = 0;
                tp2->t_tbuf.c_count = 0;
                lp->wrcnt = lp->wrcnt >= 2 ? 1 : lp->wrcnt+1;
                splx(s);
                return rc;
            }
            
            if (tp2->t_tbuf.c_ptr) {
                putcf(CMATCH((struct cblock*)tp2->t_tbuf.c_ptr));
                tp2->t_tbuf.c_ptr = 0;
                tp2->t_tbuf.c_count = 0;
            }
            tp->t_tbuf = tp2->t_tbuf;
            lp->wpending &= ~lp->lwchan;
        }
    }

    while (lp->wpending) {
        for (k=0; k < lp->nchans; k++) {
            lp->lwchan = (lp->lwchan << 1) & lp->chanmask;
            if (lp->lwchan == 0)
                lp->lwchan = 1;
            if (lp->wpending & lp->lwchan)
                break;
        }

        if (k < lp->nchans) {
            for (k = 0,  bits = lp->lwchan; (bits >>= 1) !=0; k++);
            tp2 = &lp->chans[k].tty;
            if ((rc = (*linesw[tp2->t_line].l_output)(tp2)) != 0) {
                lp->wrcnt = 1;
                tp->t_tbuf = tp2->t_tbuf;
                tp2->t_tbuf.c_ptr = 0;
                tp2->t_tbuf.c_count = 0;
                splx(s);
                return rc;
            }
            
            if (tp2->t_tbuf.c_ptr) {
                putcf(CMATCH((struct cblock*)tp2->t_tbuf.c_ptr));
                tp2->t_tbuf.c_ptr = 0;
                tp2->t_tbuf.c_count = 0;
            }
            tp->t_tbuf = tp2->t_tbuf;
            lp->wpending &= ~lp->lwchan;
        }
    }
    splx(s);
    return 0;
}

sxtin(tp, func)
register struct tty *tp;
{

    register struct tty *tp2;
    register Link_p lp;
    register chan;
    short isave;
    short lsave;

    lp = linkTable[tp->t_link];
    chan = lp->controllingtty;
    tp2 = &lp->chans[chan].tty;
    
    switch (func) {
    case FREAD|FWRITE:
        (*linesw[tp2->t_line].l_input)(tp2, func);
        break;

    case 0:
        tp2->t_rbuf = tp->t_rbuf;
        lsave = tp2->t_lflag;
        isave = tp2->t_iflag;
        tp2->t_iflag = tp->t_iflag;
        tp2->t_lflag = tp->t_lflag;
        (*linesw[tp2->t_line].l_input)(tp2, 0);
        tp2->t_lflag = lsave;
        tp2->t_iflag = isave;
        tp->t_rbuf = tp2->t_rbuf;
        tp2->t_rbuf.c_ptr = 0;
        tp2->t_rbuf.c_count = 0;
    default:
        break;
    }
}

sxtfree(lp)
Link_p lp;
{
    uint n = ((caddr_t)lp - sxtbuf) / LINKSIZE;
    sxtbusy[n] = 0;
}

Link_p sxtalloc()
{
    register int i;
    Link_p lp;

    if (sxtbuf == 0)
        sxtinit();

    if (sxtbuf) {
        for (i=0; i < sxt_cnt; i++) {
            if (sxtbusy[i] == 0) {
                lp = (Link_p)&sxtbuf[i*LINKSIZE];
                bzero(lp, sizeof(struct Link));
                sxtbusy[i] = 1;
                return lp;
            }
        }
    }
    return 0;
}

sxtinit()
{
    if ((sxtbuf=sxt_buf) == 0)
        printf("sxt cannot allocate link buffers\n");
}

sxt_sel(dev, tmflg, r, w, e)
dev_t dev;
{
    int chan;
    int ret;
    struct tty *tp;
    Link_p lp;

    ret = 0;
    chan = CHAN(dev);
    lp = linkTable[LINK(dev)];

    if (lp == (Link_p)1)
        ret |= SEL_EX;
    else if (lp == 0)
        ret |= SEL_EX;
    else if ((lp->open & (1<<chan)) == 0)
        ret |= SEL_EX;
    else {
        tp = &lp->chans[chan].tty;
        if (r && chan != lp->controllingtty) {
            tp->t_cflag |= TTYSEL;
            r = 0;
        }
        if (w && (tp->t_cflag & TOSTOP) && chan != lp->controllingtty) {
            tp->t_cflag |= TTYSEL;
            w = 0;
        }
        ret = sel_tty(tp, tmflg, r, w, e);
    }
    return ret;
}
