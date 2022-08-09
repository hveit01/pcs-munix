/* @(#)lockf.c  1.1 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/lockf.c ";

#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/filsys.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/var.h>

struct locklist *deadlock();

#define F_WAITLOCK  0x10

lockf() {

    register struct a {
        int fildes;
        int cmd;
        long size;
    } *uap;
    register struct locklist **llp;
    register struct locklist *lp;
    struct file *fp;
    struct inode *ip;
    off_t start;
    off_t end;

    uap = (struct a*)u.u_ap;
    fp = getf(uap->fildes);
    if (fp == 0)
        return;

    ip = fp->f_inode;
    if ((ip->i_mode & IFMT) != IFREG) {
        u.u_error = EINVAL;
        return;
    }
    start = fp->f_offset;
    if (uap->size > 0) {
        end = start + uap->size;
        if (end <= 0) 
            end = 0x40000000;
    } else {
        if (uap->size < 0) {
            end = start;
            start += uap->size;
            if (start < 0)
                start = 0;
        } else
            end = 0x40000000;
    }
    if (uap->cmd == F_ULOCK) {
        llp = &ip->i_locklist;
        while ((lp = ((struct locklist*)llp)->ll_link) != 0) { /* &ip->i_locklist operator */
            if (lp->ll_proc != u.u_procp) {
                llp = &lp->ll_link;
                continue;
            }
            if (end <= lp->ll_start)
                break;
            if (lp->ll_end <= start) {
                llp = &lp->ll_link;
                continue;
            }
            if (start <= lp->ll_start && lp->ll_end <= end) {
                *llp = lp->ll_link;
                lockfree(lp);
                continue;
            }
            if (lp->ll_flags & F_WAITLOCK) {
                lp->ll_flags &= ~F_WAITLOCK;
                wakeup(lp);
            }
            if (lp->ll_start < start && end < lp->ll_end) {
                if (lockadd(lp, end, lp->ll_end) != 0)
                    return;
                lp->ll_end = start;
                break;
            }
            if (start <= lp->ll_start && end < lp->ll_end) {
                lp->ll_start = end;
                break;
            }
            lp->ll_end = start;
            llp = &lp->ll_link;
        }
        return;
    }
    if (locked(uap->cmd, ip, start, end) != 0 || uap->cmd == F_TEST)
        return;
        
    llp = &ip->i_locklist;
    lp = ((struct locklist*)llp)->ll_link;
    if (lp == 0) {
        lockadd(llp, start, end);
        return;
    }
    if (end < lp->ll_start) {
        lockadd(llp, start, end);
        return;
    }
    if (end <= lp->ll_start && u.u_procp != lp->ll_proc) {
        lockadd(llp, start, end);
        return;
    }
    if (end >= lp->ll_start && start < lp->ll_start)
        lp->ll_start = start;
    llp = &lp->ll_link;
    for (;;) {
        if ((   lp = ((struct locklist*)llp)->ll_link) == 0) {
            if (start <= ((struct locklist*)llp)->ll_end &&
                u.u_procp == ((struct locklist*)llp)->ll_proc) {
                if (end > ((struct locklist*)llp)->ll_end)
                    ((struct locklist*)llp)->ll_end = end;
                return;
            } else {
                lockadd(llp, start, end);
                return;
            }
        }
        if (lp->ll_start < start) {
            llp = &lp->ll_link;
            continue;
        } else
            break;
    }
    if (end <= ((struct locklist*)llp)->ll_end) 
        return;
    if (start <= ((struct locklist*)llp)->ll_end && 
        u.u_procp == ((struct locklist*)llp)->ll_proc)
        ((struct locklist*)llp)->ll_end = end;
    else {
        if (lockadd(llp, start, end) != 0)
            return;
        llp = &((*llp)->ll_link);
    }
    while ((    lp = ((struct locklist*)llp)->ll_link) != 0) {
        if (u.u_procp != lp->ll_proc)
            return;
        if (((struct locklist*)llp)->ll_end < lp->ll_start) 
            return;
        if (((struct locklist*)llp)->ll_end <= lp->ll_end) {
            ((struct locklist*)llp)->ll_end = lp->ll_end;
            ((struct locklist*)llp)->ll_link = lp->ll_link;
            lockfree(lp);
            return;
        }
        ((struct locklist*)llp)->ll_link = lp->ll_link;
        lockfree(lp);
    }
} 

locked(cmd, ip, start, end)
register struct inode *ip;
{
    register struct locklist *lp;

    lp = ip->i_locklist;
    while (lp && lp->ll_start < end) {
        if (lp->ll_proc == u.u_procp || lp->ll_end <= start) {
            lp = lp->ll_link;
            if (lp == 0)
                return 0;
        } else {
            if (cmd == F_TLOCK || cmd == F_TEST) {
                u.u_error = EACCES;
                return 1;
            }
            if (deadlock(lp) != 0)
                return 1;
            lp->ll_flags |= F_WAITLOCK;
            
            sleep(lp, 39); 
            lp = ip->i_locklist;
            if (u.u_error)
                return 1;
        }
    }
    return 0;
}

struct locklist *
deadlock(lp) 
register struct locklist *lp;
{ 
    register struct locklist* lpw;

    while (lp->ll_proc->p_stat == SSLEEP) {
        lpw = (struct locklist*)(lp->ll_proc->p_wchan);

        if (lpw < locklist ||
            &locklist[v.v_nflocks] <= lpw)  
            break;

        if (lpw->ll_proc == u.u_procp) {
            u.u_error = EDEADLK;
            return lpw;
        }

        lp = lpw;
    }
    return 0;
}

unlock(ip)
register struct inode *ip;
{
    register struct locklist *lp;
    register struct locklist **llp;

    llp = &ip->i_locklist;
    if (llp) {
        while ((lp = ((struct locklist*)llp)->ll_link) != 0) {
            if (lp->ll_proc == u.u_procp) {
                ((struct locklist*)llp)->ll_link = lp->ll_link;
                lockfree(lp);
            } else
                llp = &lp->ll_link;
        }
    }
}

static 
struct locklist *
lockalloc()
{
    register struct locklist *lp;
    register struct locklist *llp;
    
    lp = locklist;
    
    if (lp->ll_proc ==0) {
        lp->ll_proc = &proc[0];
        
        for (llp = &locklist[1]; llp < &locklist[v.v_nflocks]; llp++) {
            /* loc_4A6: */
            lockfree(llp);
            /* loc_4B4: */
        }
    }
    /* loc_4D2: */
    if ((llp = lp->ll_link) == 0) {
        u.u_error = EDEADLK;
        return 0;
    }
    /* loc_4E4: */
    lp->ll_link = llp->ll_link;
    llp->ll_link = 0;
    return llp;
} 

static
lockfree(lp)
register struct locklist *lp;
{
    register struct locklist *llp;
    
    llp = locklist;
    if (lp->ll_flags & F_WAITLOCK) {
        lp->ll_flags &= ~F_WAITLOCK;
        wakeup(lp);
    }
    lp->ll_link = llp->ll_link;
    llp->ll_link = lp;
}

static
lockadd(lp, s, e)
register struct locklist* lp;
{
    register struct locklist *llp;
    
    llp = lockalloc();
    if (llp == 0)
        return 1;
    
    llp->ll_link = lp->ll_link;
    lp->ll_link = llp;
    llp->ll_proc = u.u_procp;
    llp->ll_start = s;
    llp->ll_end = e;
    return 0;
}
