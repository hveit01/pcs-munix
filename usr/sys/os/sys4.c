/* @(#)sys4.c   6.4 */
static char* _Version = "@(#) RELEASE:  1.3  Sep 15 1986 /usr/sys/os/sys4.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/page.h"
#include <sys/region.h>
#include "sys/proc.h"
#include "sys/var.h"
#include "sys/timeb.h" /*pcs*/
#include "sys/munet/munet.h" /*pcs*/

/*
 * Everything in this file is a routine implementing a system call.
 */

hertz()
{
    u.u_rval1 = hz;
} 

m881used()
{
    extern int have881;
    if (have881)
        u.u_m881 = 1;
}

gtime()
{ 
    u.u_rtime = time;
} 

stime()
{
    register unsigned i;
    register struct a {
        long time;
    } *uap;
    register time_t t;

    uap = (struct a *)u.u_ap;
    if (suser()) {
        t = fulong(uap->time);
        if (t != -1) {
            bootime += t - time;
            time = t;
            if (haveclock)
                write_clock();
        }
    }
}

setuid()
{
    register unsigned uid;
    register struct a {
        int uid;
    } *uap;

    uap = (struct a *)u.u_ap;
    uid = uap->uid;
    if (uid >= MAXUID) {
        u.u_error = EINVAL;
        return;
    }
    if (u.u_uid != 0 && (uid == u.u_ruid || uid == u.u_procp->p_suid))
        u.u_uid = uid;
    else if (u.u_procp->p_flag & SFSERV || /*pcs*/
            suser()) {
        u.u_uid = uid;
        u.u_procp->p_uid = uid;
        u.u_procp->p_suid = uid;
        u.u_ruid = uid;
    }
}

getuid()
{
    u.u_rval1 = (u.u_ruid<<16) + u.u_uid;
}

setgid()
{
    register unsigned gid;
    register struct a {
        int gid;
    } *uap;

    uap = (struct a *)u.u_ap;
    gid = uap->gid;
    if (gid >= MAXUID) {
        u.u_error = EINVAL;
        return;
    }
    if (u.u_rgid == gid ||
        u.u_gid == gid || u.u_procp->p_flag & SFSERV || /*pcs*/
        suser()) {
        u.u_gid = gid;
        u.u_rgid = gid;
    }
}

getgid()
{
    u.u_rval1 = (u.u_rgid << 16) + u.u_gid;
}

getpid()
{
    u.u_rval1 = (u.u_procp->p_pid << 16) + u.u_procp->p_ppid;
}

setpgrp()
{
    register struct proc *p = u.u_procp;
    register struct a {
        int flag;
    } *uap;

    uap = (struct a *)u.u_ap;
    if (uap->flag) {
        if (p->p_pgrp != p->p_pid)
            u.u_ttyp = NULL;
        p->p_pgrp = p->p_pid;
    }
    u.u_rval1 = p->p_pgrp;
}

sync()
{
    update();
}

nice()
{
    register n;
    register struct a {
        int niceness;
    } *uap;

    uap = (struct a *)u.u_ap;
    n = uap->niceness;
    if (n < 0 && !suser()) /*pcs*/
        n = 0;
    n += u.u_procp->p_nice;
    if (n >= 2*NZERO)
        n = 2*NZERO -1;
    if (n < 0)
        n = 0;
    u.u_procp->p_nice = n;
    u.u_rval1 = n - NZERO;
}

/*
 * Unlink system call.
 * Hard to avoid races here, especially
 * in unlinking directories.
 */
unlink()
{
    register struct inode *ip, *pp;
    struct a {
        char    *fname;
    };

    pp = namei(uchar, DONT_FOLLOW | 2);
    if (pp == NULL)
        return;
    /*
     * Check for unlink(".")
     * to avoid hanging on the iget
     */
    if (pp->i_number == u.u_dent.d_ino) {
        ip = pp;
    } else
        ip = iget(pp->i_mount, u.u_dent.d_ino);
    if (ip == NULL)
        goto out1;
    if (((ip->i_mode&IFMT) == IFDIR ||
        (ip->i_mode&IFMT) == IFLNK)&& 
        !suser())
        goto out;
    /*
     * Don't unlink a mounted file.
     */
    if (ip->i_dev != pp->i_dev) {
        u.u_error = EBUSY;
        goto out;
    }
    if (ip->i_flag&ITEXT)
        xrele(ip);  /* try once to free text */
    if (ip->i_flag&ITEXT && ip->i_nlink == 1) {
        u.u_error = ETXTBSY;
        goto out;
    }
    u.u_offset -= sizeof(struct direct);
    u.u_base = (caddr_t)&u.u_dent;
    u.u_count = sizeof(struct direct);
    u.u_dent.d_ino = 0;
    u.u_segflg = 1;
    u.u_fmode = FWRITE|FSYNC;
    writei(pp);
    if (u.u_error)
        goto out;
    ip->i_nlink--;
    ip->i_flag |= ICHG;

out:
    iput(ip);
    if (pp != ip)
out1:
        iput(pp);
}

chdir()
{
    chdirec(&u.u_cdir);
}

chroot()
{
    if (!suser())
        return;
    chdirec(&u.u_rdir);
}

chdirec(ipp)
register struct inode **ipp;
{
    register struct inode *ip;
    struct a {
        char    *fname;
    };

    ip = namei(uchar, 0);
    if (ip == NULL)
        return;
    if ((ip->i_mode&IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        goto bad;
    }
    if (access(ip, IEXEC))
        goto bad;
    if (&u.u_cdir == ipp)   /*pcs*/
        u.u_cdirdev = -1;   /*pcs*/
    else                    /*pcs*/
        u.u_crootdev = -1;  /*pcs*/
    
    prele(ip);
    if (*ipp) {
        plock(*ipp);
        iput(*ipp);
    }
    *ipp = ip;
    return;

bad:
    iput(ip);
}

chmod()
{
    register struct inode *ip;
    register struct a {
        char    *fname;
        int fmode;
    } *uap;

    uap = (struct a *)u.u_ap;
    if ((ip = owner()) == NULL)
        return;
    ip->i_mode &= ~07777;
    if (u.u_uid) {
        uap->fmode &= ~ISVTX;
        if (u.u_gid != ip->i_gid)
            uap->fmode &= ~ISGID;
    }
    ip->i_mode |= uap->fmode&07777;
    ip->i_flag |= ICHG;
    if (ip->i_flag&ITEXT && (ip->i_mode&ISVTX)==0)
        xrele(ip);
    iput(ip);
}

chown()
{
    register struct inode *ip;
    register struct a {
        char    *fname;
        int uid;
        int gid;
    } *uap;

    uap = (struct a *)u.u_ap;
    if ((ip = owner(IEXEC)) == NULL) /*pcs*/
        return;
    ip->i_uid = uap->uid;
    ip->i_gid = uap->gid;
    if (u.u_uid != 0)
        ip->i_mode &= ~(ISUID|ISGID);
    ip->i_flag |= ICHG;
    iput(ip);
}

ssig()
{
    register a;
    register struct proc *p;
    struct a {
        int signo;
        int fun;
    } *uap;

    uap = (struct a *)u.u_ap;
    a = uap->signo;
    if (a <= 0 || a > NSIG || a == SIGKILL) {
        u.u_error = EINVAL;
        return;
    }
    u.u_rval1 = u.u_signal[a-1];
    u.u_signal[a-1] = uap->fun;
    u.u_procp->p_sig &= ~(1L<<(a-1));
    if (a == SIGCLD) {
        a = u.u_procp->p_pid;
        for (p = &proc[1]; p < (struct proc *)v.ve_proc; p++) {
            if (a == p->p_ppid && p->p_stat == SZOMB)
                psignal(u.u_procp, SIGCLD);
        }
    }
}

kill()
{
    register struct proc *p, *q;
    register arg;
    register struct a {
        int pid;
        int signo;
    } *uap;
    int f;

    uap = (struct a *)u.u_ap;
    if (uap->signo < 0 || uap->signo > NSIG) {
        u.u_error = EINVAL;
        return;
    }
    /* Prevent proc 1 (init) from being SIGKILLed */
    if (uap->signo == SIGKILL && uap->pid == 1) {
        u.u_error = EINVAL;
        return;
    }
    f = 0;
    arg = uap->pid;
    if (arg > 0)
        p = &proc[1];
    else
        p = &proc[2];
    q = u.u_procp;
    if (arg == 0 && q->p_pgrp == 0) {
        u.u_error = ESRCH;
        return;
    }
    for(; p < (struct proc *)v.ve_proc; p++) {
        if (p->p_stat == 0)
            continue;
        if (arg > 0 && p->p_pid != arg)
            continue;
        if (arg == 0 && p->p_pgrp != q->p_pgrp)
            continue;
        if (arg < -1 && p->p_pgrp != -arg)
            continue;
        if (! (u.u_uid == 0 ||
            u.u_uid == p->p_uid ||
            u.u_ruid == p->p_uid ||
            u.u_uid == p->p_suid ||
            u.u_ruid == p->p_suid ))
            if (arg > 0) {
                u.u_error = EPERM;
                return;
            } else
                continue;
        f++;
        if (uap->signo)
            psignal(p, uap->signo);
        if (arg > 0)
            break;
    }
    if (f == 0)
        u.u_error = ESRCH;
}

times()
{
    register struct a {
        time_t  (*times)[4];
    } *uap;

    uap = (struct a *)u.u_ap;
    if (copyout((caddr_t)&u.u_utime, (caddr_t)uap->times, sizeof(*uap->times)))
        u.u_error = EFAULT;
    u.u_rtime = lbolt;
}

profil()
{
    register struct a {
        short   *bufbase;
        long     bufsize;   /*pcs*/
        unsigned pcoffset;
        unsigned pcscale;
    } *uap;

    uap = (struct a *)u.u_ap;
    u.u_prof.pr_base = uap->bufbase;
    u.u_prof.pr_size = uap->bufsize >> 1; /*pcs*/
    u.u_prof.pr_off = uap->pcoffset;
    u.u_prof.pr_scale = uap->pcscale;
}

/*
 * alarm clock signal
 */
alarm()
{
    register struct proc *p;
    register c;
    register struct a {
        int deltat;
    } *uap;

    uap = (struct a *)u.u_ap;
    p = u.u_procp;
    c = p->p_clktim;
    p->p_clktim = uap->deltat;
    u.u_rval1 = c;
}

nap()
{
    register int *dummy;    /*pcs unused*/
    register struct a {
        int dely;
    } *uap;
    
    uap = (struct a *)u.u_ap;   
    delay(uap->dely);
}

/*
 * indefinite wait.
 * no one should wakeup(&u)
 */
pause()
{
    for(;;)
        sleep((caddr_t)&u, PSLEP);
}

/*
 * mode mask for creation of files
 */
umask()
{
    register struct a {
        int mask;
    } *uap;
    register t;

    uap = (struct a *)u.u_ap;
    t = u.u_cmask;
    u.u_cmask = uap->mask & 0777;
    u.u_rval1 = t;
}

/*
 * Set IUPD and IACC times on file.
 */
utime()
{
    register struct a {
        char    *fname;
        time_t  *tptr;
    } *uap;
    register struct inode *ip;
    time_t tv[2];

    uap = (struct a *)u.u_ap;
    if (uap->tptr != NULL) {
        if (copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof(tv))) {
            u.u_error = EFAULT;
            return;
        }
    } else {
        tv[0] = time;
        tv[1] = time;
    }
    ip = namei(uchar, 0);
    if (ip == NULL)
        return;
    if (u.u_uid != ip->i_uid && u.u_uid != 0) {
        if (uap->tptr != NULL)
            u.u_error = EPERM;
        else
            access(ip, IWRITE);
    }
    if (!u.u_error) {
        ip->i_flag |= IACC|IUPD|ICHG;
        iupdat(ip, &tv[0], &tv[1]);
    }
    iput(ip);
}

extern int maxupts;
ulimit()
{
    register n;
    register struct a {
        int cmd;
        long    arg;
    } *uap;

    uap = (struct a *)u.u_ap;
    switch(uap->cmd) {
    case 2:
        if (uap->arg > u.u_limit && !suser())
            return;
        u.u_limit = uap->arg;
    case 1:
        u.u_roff = u.u_limit;
        break;

        case 3:{
                register preg_t *prp, *dprp, *prp2;
                preg_t *prp3; /*pcs*/

                /*      Find the data region
                 */

                dprp = findpreg(u.u_procp, PT_DATA);
                if(dprp == NULL)
                        u.u_roff = 0;
                else {
                        /*      Now find the region with a virtual
                         *      address greater than the data region
                         *      but lower than any other region
                         */
                        prp2 = NULL;
                        for(prp = u.u_procp->p_region; prp->p_reg; prp++) {                         
                                if (prp->p_type == PT_STACK)
                                    prp3 = prp;
                                if(prp->p_regva <= dprp->p_regva)
                                        continue;
                                if(prp2==NULL || prp->p_regva < prp2->p_regva)
                                        prp2 = prp;
                        }
                        
                        u.u_roff = stob(maxupts - btos(SYSVA - (int)prp3->p_regva));
                        if (prp2 != NULL)
                                u.u_roff = min( (off_t)prp2->p_regva, u.u_roff);
                }
                break;
        }

    default:
        u.u_error = EINVAL;
    }
}

ftime()
{ 
    register unsigned long t;
    register struct a {
        struct timeb *tb;
    } *uap;

    struct timeb tmp;

    uap = (struct a *)u.u_ap;

    spltimer();
    t = lbolt % hz;
    tmp.time = time;
    spl0();

    tmp.millitm = t * (1000 / hz);
    tmp.timezone = -60;
    tmp.dstflag = 0;

    if (copyout(&tmp, uap->tb, sizeof(struct timeb)) < 0)
        u.u_error = EFAULT;
}
