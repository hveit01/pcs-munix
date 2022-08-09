/* @(#)text.c   6.5 */
static char *_Version = "@(#) RELEASE:  1.5  Nov 05 1986 /usr/sys/os/text.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/inode.h>
#include <sys/buf.h>
#include <sys/var.h>
#include <sys/sysinfo.h>
#include <sys/seg.h>
#include <sys/page.h>
#include <sys/pfdat.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/munet/munet.h> /*pcs*/

/*
 * Allocate text region for a process
 */
xalloc(ip)
register struct inode *ip;
{
    register reg_t      *rp;
    register preg_t     *prp;
    register int        size;
    ulong               org;    /*pcs*/
    int                 segtype;    /*pcs*/

    if((size = u.u_exdata.ux_tsize) == 0)
        return(0);

    org = u.u_exdata.ux_txtorg; /*pcs*/

    /*  Search the region table for the text we are
     *  looking for.
     */
/*pcs*/
#define IS_REMOTE(rp) \
 (u.u_rmt_ino==(rp)->r_rmt_ino && \
  u.u_rmt_id==(rp)->r_rmt_id && u.u_rmt_dev==(rp)->r_rmt_dev)
  
loop:
    if ((ip->i_flag & ILAND)==0 || u.u_rmt_ino != 0) {              /*pcs*/
        for(rp = ractive.r_forw ; rp != &ractive ; rp = rp->r_forw){
            if ((u.u_rmt_ino != 0 && u.u_rmt_ino==(rp)->r_rmt_ino &&
                 u.u_rmt_id==(rp)->r_rmt_id && u.u_rmt_dev==(rp)->r_rmt_dev) ||
                (u.u_rmt_ino == 0 && ip == (rp)->r_iptr)) {                 /*pcs*/
                reglock(rp);
                if ((u.u_rmt_ino != 0 && (u.u_rmt_ino!=(rp)->r_rmt_ino ||
                     u.u_rmt_id!=(rp)->r_rmt_id || u.u_rmt_dev!=(rp)->r_rmt_dev)) ||
                    (u.u_rmt_ino == 0 && ip != (rp)->r_iptr)) {             /*pcs*/
                    regrele(rp);
                    goto loop;
                }
                while(!(rp->r_flags & RG_DONE)){
                    rp->r_flags |= RG_WAITING;
                    regrele(rp);
                    sleep(&rp->r_flags, PZERO);
/*                  reglock(rp);*/                                  /*pcs*/
                    goto loop;                                      /*pcs*/
                }
                prp = attachreg(rp, &u,  org & ~SOFFMASK, PT_TEXT, SEG_RO); /*pcs*/
                regrele(rp);
                if(prp == NULL)
                    return(-1);
                return((int)prp);
            }
        }
    } /*pcs*/
    
    /*  Text not currently being executed.  Must allocate
     *  a new region for it.
     */

    if((rp = allocreg(ip, RT_STEXT)) == NULL)
        return(-1);

    /* PCS newcastle connection stuff */
    if ((ip->i_flag & ILAND) != 0 && u.u_rmt_ino != 0) {
        rp->r_rmt_ino = u.u_rmt_ino;
        rp->r_rmt_dev = u.u_rmt_dev;
        rp->r_rmt_id = u.u_rmt_id;
    } else {
        rp->r_rmt_ino = NULL;
        rp->r_rmt_dev = (dev_t)-1;
        rp->r_rmt_id = -1;
    }
    if (((ip->i_flag & ILAND)==0 && (ip->i_mode & ISVTX)!=0) ||
        (u.u_rmt_ino && u.u_rmt_mode & ISVTX))
        rp->r_flags |= RG_NOFREE;
    u.u_rmt_ino = NULL;
    u.u_rmt_dev = (dev_t)-1;
    u.u_rmt_id = -1;
    segtype = u.u_exdata.ux_mag == 0413 ? SEG_RO : SEG_RW;
    /*end pcs*/
    
    /*  Attach the region to our process.
     */
    
    if((prp = attachreg(rp, &u, org & ~SOFFMASK, PT_TEXT, segtype)) == NULL){ /*pcs*/
        freereg(rp);
        return(-1);
    }
    
    /*  Load the region or map it for demand load.
     */

    if(u.u_exdata.ux_mag == 0413){
/*
        ASSERT(u.ux_tstart == 0);
*/
        if(mapreg(prp, org, ip, u.ux_tstart, size) < 0){                /*pcs*/
            detachreg(&u, prp);
            ip->i_flag &= ~ITEXT;                                       /*pcs*/
            return(-1);
        }
    } else if (u.u_exdata.ux_mag == 0410 || u.u_exdata.ux_mag == 0411) {/*pcs*/
        rp->r_type = RT_SHMEM;                                          /*pcs*/
        if (loadreg(prp, org, ip, u.ux_tstart, size) < 0) {             /*pcs*/
            rp->r_flags &= ~RG_NOFREE;                                  /*pcs*/
            rp->r_type = RT_STEXT;                                      /*pcs*/
            detachreg(&u, prp);                                         /*pcs*/
            ip->i_flag &= ~ITEXT;                                       /*pcs*/
            return -1;                                                  /*pcs*/
        } else {                                                        /*pcs*/
            rp->r_type = RT_STEXT;                                      /*pcs*/
            chgprot(prp, SEG_RO);                                       /*pcs*/
        }                                                               /*pcs*/
    } else {
        panic("xalloc - bad magic");
    }

    regrele(rp);
    return(0);
}


/*
 * free the swap image of all unused saved-text text segments
 * which are from device dev (used by umount system call).
 */
xumount(dev)
register dev_t dev;
{
    register struct inode   *ip;
    register reg_t      *rp;
    register reg_t      *nrp;
    register        count;

    while(1){
        count = 0;
        for(rp = ractive.r_forw ; rp != &ractive ; rp = nrp){
            reglock(rp);
            if(rp->r_type == RT_UNUSED){
                regrele(rp);
                break;
            }
            nrp = rp->r_forw;
            if ((ip = rp->r_iptr) == NULL){
                regrele(rp);
                continue;
            }
            if (dev != NODEV && dev != ip->i_dev){
                regrele(rp);
                continue;
            }
            if (xuntext(rp))
                count++;
            else
                regrele(rp);
        }
        if(rp == &ractive)
            return(count);
    }
}


/*
 * remove a shared text segment from the region table, if possible.
 */
xrele(ip)
register struct inode *ip;
{
    register reg_t  *rp;
    register reg_t  *nrp;

    if ((ip->i_flag&ITEXT) == 0)
        return;
    
    while(1){
        for(rp = ractive.r_forw ; rp != &ractive ; rp = nrp){
            reglock(rp);
            if(rp->r_type == RT_UNUSED){
                regrele(rp);
                break;
            }
            nrp = rp->r_forw;
            if (ip == rp->r_iptr){
                if(!xuntext(rp))
                    regrele(rp);
            } else {
                regrele(rp);
            }
        }
        if(rp == &ractive)
            return;
    }
}


/*  Try to removed unused regions in order to free up swap
 *  space.
 */

swapclup()
{
    register reg_t  *rp;
    register reg_t  *nrp;
    register struct inode *ip;  /*pcs*/
    register int    rval;
    
    rval = 0;                                                           /*pcs*/     
    while(1){
        for(rp = ractive.r_forw ; rp != &ractive ; rp = nrp){
            nrp = rp->r_forw;                                           /*pcs*/
            if (rp->r_type != RT_SHMEM &&                               /*pcs*/
                ((ip=rp->r_iptr) == NULL || (ip->i_flag & ILOCK)==0)) { /*pcs*/
                if ((rp->r_flags & RG_LOCK) == 0) {                     /*pcs*/
                    reglock(rp);
                    if(rp->r_type == RT_UNUSED){
                        regrele(rp);
                        break;
                    }
/*                  nrp = rp->r_forw;*/
                    if(!xuntext(rp))
                        regrele(rp);
                    else
                        rval = 1;
                }                                                       /*pcs*/
            }
        }
        if(rp == &ractive)
            return(rval);
    }
}

/*
 * remove text image from the region table.
 * the use count must be zero.
 */
xuntext(rp)
register reg_t  *rp;
{
    if (rp->r_refcnt != 0)
        return(0);
    freereg(rp);
    return(1);
}
