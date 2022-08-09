/* @(#)pipe.c   6.1 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/pipe.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/sel.h"    /*pcs*/

struct mount *pipemnt;

/*
 * The sys-pipe entry.
 * Allocate an inode on the root device.
 * Allocate 2 file structures.
 * Put it all together with flags.
 */
pipe()
{
    register struct inode *ip;
    register struct file *rf, *wf;
    union a {
        struct fd_pair32 {
            int fd[2];
        } *fd32;
        struct fd_pair16 {
            short fd[2];
        } *fd16;
    } *uap;
    int r;  

    uap = (union a*)u.u_ap;

    ip = ialloc(pipemnt, IFIFO, 0);
    if(ip == NULL)
        return;
    rf = falloc(ip, FREAD);
    if(rf == NULL) {
        iput(ip);
        return;
    }
    r = u.u_rval1;
    wf = falloc(ip, FWRITE);
    if(wf == NULL) {
        rf->f_count = 0;
        rf->f_next = ffreelist;
        ffreelist = rf;
        u.u_ofile[r] = NULL;
        iput(ip);
        return;
    }
    /* loc_9C: */
    if (u.u_exvec->exno == 46) {
        sulong(&uap->fd32->fd[1], u.u_rval1);
        sulong(&uap->fd32->fd[0], r);
    } else {
        /* loc_D8: */
        suword(&uap->fd16->fd[1], u.u_rval1);
        suword(&uap->fd16->fd[0], r);
    }
    /* loc_102: */
    wf->f_flag = FWRITE;    /*pcs*/
    wf->f_inode = ip;       /*pcs*/
    rf->f_flag = FREAD;     /*pcs*/
    rf->f_inode = ip;       /*pcs*/

    ip->i_count = 2;
    ip->i_frcnt = 1;
    ip->i_fwcnt = 1;
    prele(ip);
}

/*
 * Open a pipe
 * Check read and write counts, delay as necessary
 */

openp(ip, mode)
register struct inode *ip;
register mode;
{
    if (mode&FREAD) {
        if (ip->i_frcnt++ == 0)
            wakeup((caddr_t)&ip->i_frcnt);
    }
    if (mode&FWRITE) {
        if (mode&FNDELAY && ip->i_frcnt == 0) {
            u.u_error = ENXIO;
            return;
        }
        if (ip->i_fwcnt++ == 0)
            wakeup((caddr_t)&ip->i_fwcnt);
    }
    if (mode&FREAD) {
        while (ip->i_fwcnt == 0) {
            if (mode&FNDELAY || ip->i_size)
                return;
            sleep(&ip->i_fwcnt, PPIPE);
        }
    }
    if (mode&FWRITE) {
        while (ip->i_frcnt == 0)
            sleep(&ip->i_frcnt, PPIPE);
    }
}

/*
 * Close a pipe
 * Update counts and cleanup
 */

closep(ip, mode)
register struct inode *ip;
register mode;
{
    register i;
    daddr_t bn;

    if (mode&FREAD) {
        if ((--ip->i_frcnt == 0) && (ip->i_fflag&IFIW)) {
            ip->i_fflag &= ~IFIW;
            wakeup((caddr_t)&ip->i_fwcnt);
        }
    }
    if (mode&FWRITE) {
        if ((--ip->i_fwcnt == 0) && (ip->i_fflag&IFIR)) {
            ip->i_fflag &= ~IFIR;
            wakeup((caddr_t)&ip->i_frcnt);
        }
    }
    if ((ip->i_frcnt == 0) && (ip->i_fwcnt == 0)) {
        for (i=NFADDR-1; i>=0; i--) {
            bn = ip->i_faddr[i];
            if (bn == (daddr_t)0)
                continue;
            ip->i_faddr[i] = (daddr_t)0;
            free(ip->i_mount, bn);
        }
        ip->i_size = 0;
        ip->i_frptr = 0;
        ip->i_fwptr = 0;
        ip->i_flag |= IUPD|ICHG;
    }
    
    if (ip->i_flag & ISEL) {        /*pcs*/
        ip->i_flag &= ~ISEL;        /*pcs*/
        wakeup((caddr_t)&nselect);  /*pcs*/
    }
}

/*
 * Lock a pipe.
 * If its already locked,
 * set the WANT bit and sleep.
 */
plock(ip)
register struct inode *ip;
{
    if ((ip->i_flag & ILAND) == 0) {    /*pcs*/
        while(ip->i_flag&ILOCK) {
            ip->i_flag |= IWANT;
            sleep((caddr_t)ip, PINOD);
        }
        ip->i_flag |= ILOCK;
    }                                   /*pcs*/
}

/*
 * Unlock a pipe.
 * If WANT bit is on,
 * wakeup.
 * This routine is also used
 * to unlock inodes in general.
 */
prele(ip)
register struct inode *ip;
{
    if (ip->i_flag & ILAND) return;     /*pcs*/
    ip->i_flag &= ~ILOCK;
    if (ip->i_flag & IWANT) {
        ip->i_flag &= ~IWANT;
        wakeup((caddr_t)ip);
    }
}
