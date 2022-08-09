/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.1  Jun 25 1987 /usr/sys/io/sel.c";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/sel.h"
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

int nselect;

seltimeo(pp)
struct proc *pp;
{
    pp->p_flag &= ~SSEL;
    wakeup(&nselect);
}

/* select system call */
select()
{
    register struct file *fp;
    register struct inode *ip;
    register i;
    register wrd; /*word in fd_set */
    register bit; /*bit in fd_set*/
    
    struct a {
        int nfds;
        fd_set *readfds;
        fd_set *writefds;
        fd_set *exceptfds;
        struct timeval *tmout;
    } *uap;
    int tmflg1;
    int tmflg2;
    int nfds;
    int nwords;
    fd_set ireadfds;
    fd_set iwritefds;
    fd_set iexceptfds;
    fd_set oreadfds;
    fd_set owritefds;
    fd_set oexceptfds;
    int toval;
    int sel;
    int nfound;
    
    
    uap = (struct a*)u.u_ap;
    nfds = uap->nfds;
    if (uap->tmout == 0) {
        toval = -1;
    } else {
        struct timeval tmo;
        if (copyin(uap->tmout, &tmo, sizeof(struct timeval)) != 0)
            return;
        if (tmo.tv_sec < 0 || tmo.tv_usec < 0 || tmo.tv_usec > 1000000) {
            u.u_error = EINVAL;
            return;
        }
        toval = tmo.tv_sec*hz + (tmo.tv_usec * hz / 1000000);
    }
    
    if (nfds > NOFILE)
        nfds = NOFILE;

    nwords = (nfds + 31) / 32; /* #words required for nfds select bits */
    if (nwords > SEL_NSET)
        nwords = SEL_NSET;

    for (i=0; i < SEL_NSET; i++)
        ireadfds.fds_bits[i] =
        iwritefds.fds_bits[i] =
        iexceptfds.fds_bits[i] =
        oreadfds.fds_bits[i] =
        owritefds.fds_bits[i] =
        oexceptfds.fds_bits[i] = 0;

    if (uap->readfds) {
        if (copyin(uap->readfds, &ireadfds, nwords<<2) != 0)
            goto eferr;
    }
    if (uap->writefds) {
        if (copyin(uap->writefds, &iwritefds, nwords<<2) != 0)
            goto eferr;
    }
    if (uap->exceptfds) {
        if (copyin(uap->exceptfds, &iexceptfds, nwords<<2) != 0) {
eferr:      u.u_error = EFAULT;
            return;
        }
    }

    if (toval == 0) {
        tmflg1 = 0;
        tmflg2 = 0;
    } else {
        tmflg1 = 1;
        tmflg2 = 1;
    }

    nfound = 0;

/* I'd liked to have a for(;;) and break/continue, instead of 'goto loop'
 * but compiler did not produce the same code */
loop:
    spl7();
    for (i=0; i<nfds; i++) {
        int r, w, e;
        bit = 1L << (i & 31);
        wrd = i >> 5;
        r = (ireadfds.fds_bits[wrd] & bit) != 0;
        w = (iwritefds.fds_bits[wrd] & bit) != 0;
        e = (iexceptfds.fds_bits[wrd] & bit) != 0;
        if (r || w || e) {
            if ((fp = u.u_ofile[i]) != 0) {
                ip = fp->f_inode;
                sel = 0;
                if ((ip->i_mode&IFMT)==IFIFO) {
                    if (fp->f_vtty1 != F_UNUSED)
                        sel = sel_vtty(fp, tmflg1, r, w, e);
                    else
                        sel = sel_pipe(ip, tmflg1, r, w, e);
                } else if ((ip->i_mode&IFMT)==IFCHR) {
                    dev_t maj, unit;
                    maj = major((dev_t)ip->i_rdev);
                    unit= minor((dev_t)ip->i_rdev);
                    if (cdevsw[maj].d_ttys)
                        sel |= sel_tty(&cdevsw[maj].d_ttys[unit], tmflg1, r, w, e);
                    if (cdevsw[maj].d_sel)
                        sel |= (*cdevsw[maj].d_sel)(unit, tmflg1, r, w, e);
                }
                if (sel) {
                    nfound++;
                    if (sel & SEL_IN)
                        oreadfds.fds_bits[wrd] |= bit;
                    if (sel & SEL_OUT)
                        owritefds.fds_bits[wrd] |= bit;
                    if (sel & SEL_EX)
                        oexceptfds.fds_bits[wrd] |= bit;
                }
            } else {
                u.u_error = EBADF;
                tmflg1 = 0;
                break;
            }
        }
    }   
        
    if (nfound==0 && tmflg1) {
        int res;
        u.u_procp->p_flag |= SSEL;
        if (toval > 0)
            timeout(seltimeo, u.u_procp, toval);
        nselect++;
        res = sleep(&nselect, 0x123);
        nselect--;
        if (toval > 0 && (u.u_procp->p_flag & SSEL)) {
            toval = cancelto(seltimeo, u.u_procp);
            if (toval==0)
                tmflg1 = 0;
        }
        if (res) {
            u.u_error = EINTR;
            u.u_procp->p_flag &= ~SSEL;
            /*leave loop */
        } else
        if ((u.u_procp->p_flag & SSEL)) {
            u.u_procp->p_flag &= ~SSEL;
            goto loop;
        }
    }
/*end of loop */

    if (tmflg2) {
        for (i=0; i<nfds; i++) {
            bit = 1L << (i & 31);
            wrd = i >> 5;
            if ((ireadfds.fds_bits[wrd] & bit) ||
                  (iwritefds.fds_bits[wrd] & bit) ||
                  (iexceptfds.fds_bits[wrd] & bit)) {
                if ((fp = u.u_ofile[i]) != 0) {
                    ip = fp->f_inode;
                    if ((ip->i_mode&IFMT) == IFIFO) {
                        if (fp->f_vtty1 != F_UNUSED)
                            sel_vtty(fp, -1, 0, 0, 0);
                        else
                            sel_pipe(ip, -1, 0, 0, 0);
                    } else if ((ip->i_mode&IFMT) == IFCHR) {
                        short maj, unit;
                        maj = major((short)ip->i_rdev);
                        unit= minor((short)ip->i_rdev);
                        if (cdevsw[maj].d_ttys)
                            sel_tty(&cdevsw[maj].d_ttys[unit], -1, 0, 0, 0);
                        if (cdevsw[maj].d_sel)
                            (*cdevsw[maj].d_sel)(unit, -1, 0, 0, 0);
                    }
                }
            }
        }
    }
    spl0();

    if (u.u_error == 0) {
        if (uap->readfds) {
            if (copyout(&oreadfds, uap->readfds, nwords<<2) != 0)
                goto eferr2;
        }
        if (uap->writefds) {
            if (copyout(&owritefds, uap->writefds, nwords<<2) != 0)
                goto eferr2;
        }
        if (uap->exceptfds) {
            if (copyout(&oexceptfds, uap->exceptfds, nwords<<2) != 0) {
eferr2:         u.u_error = EFAULT;
            }
        }
    }
    u.u_r.r_val = nfound;
}

sel_pipe(ip, tmflg, r, w, e)
struct inode *ip;
{
    
    register flag = 0;
    if (tmflg == -1)
        ip->i_flag &= ~ISEL;
    else {
        if (r && ip->i_size != 0)
            flag |= SEL_IN;
        if (w && ip->i_size < PIPSIZ)
            flag |= SEL_OUT;
        if (e && ((ip->i_size == 0 && ip->i_fwcnt==0) ||
                   ip->i_frcnt == 0))
            flag |= SEL_EX;
        if (flag == 0 && tmflg)
            ip->i_flag |= ISEL;
    }
    return flag;
}

sel_tty(tp, tmflg, r, w, e)
struct tty *tp;
{
    register flag = 0;

    if (tmflg == -1)
        tp->t_cflag &= ~TTYSEL;
    else {
        if (r && tp->t_delct > 0)
            flag |= SEL_IN;
        if (w && tp->t_outq.c_cc < tthiwat[tp->t_cflag & CBAUD])
            flag |= SEL_OUT;
        if (e && (tp->t_state & CARR_ON)==0)
            flag |= SEL_EX;
        if (flag == 0 && tmflg)
            tp->t_cflag |= TTYSEL;
    }     
    return flag;
}

sel_true()
{
    return SEL_IN|SEL_OUT;
}
