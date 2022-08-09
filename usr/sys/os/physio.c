/* @(#)physio.c 1.2 */
static char *_Version = "@(#) RELEASE:  1.1  Jul 16 1986 /usr/sys/os/physio.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/dir.h"
#include "sys/map.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/var.h"
#include "sys/inode.h"
#include "sys/page.h"
#include "sys/seg.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/swap.h"

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */


/* Max. number of pages to swap per I/O */

#define NPAGE 1     /*pcs*/

/*
 * swap I/O
 */
swap(dbd, ptbl, coreaddr, npage, rdflg)
dbd_t       *dbd;
register spte_t *ptbl;
{
    register struct buf *bp;
    register int        blkno;
    register int    c;
    register dev_t      dev;
    int ospl; /*var20*/
    struct proc *p;

    p = u.u_procp;
    syswait.swap++;
    ospl = splhi();
    dev = swaptab[dbd->dbd_swpi].st_dev;
    blkno = dbd->dbd_blkno;

    while ((bp = pfreelist.av_forw) == NULL) {
        pfreelist.b_flags |= B_WANTED;
        sleep((caddr_t)&pfreelist, PRIBIO+1);
    }
    pfreelist.av_forw = bp->av_forw;
    spl0();
    bp->b_proc = p;
    bp->b_flags = B_BUSY | B_PHYS | B_PT | B_PAGEIO | rdflg; /*pcs*/
    bp->b_dev = dev;
    bp->b_un.b_addr = (caddr_t)coreaddr;
/*  bp->b_pt = ptbl;*/

    p->p_flag |= SLOCK;

    while (npage > 0) {
        bp->b_bcount = ptob((c = npage>NPAGE ? NPAGE : npage));
        bp->b_blkno = blkno;
        bp->b_paddr = map_gpgs_dma(bp->b_un.b_addr, c); /*pcs*/
        (*bdevsw[bmajor(dev)].d_strategy)(bp); /*pcs*/
        u.u_iosw++;
        if (rdflg) {
            sysinfo.swapin++;
            sysinfo.bswapin += c;
        } else {
            sysinfo.swapout++;
            sysinfo.bswapout += c;
        }
        spl6();
        while((bp->b_flags&B_DONE)==0)
            sleep((caddr_t)bp, PSWP);
        spl0();

        if (bp->b_flags & B_ERROR)
            panic("IO err in swap");
        bp->b_un.b_addr += ptob(c);
        bp->b_flags &= ~B_DONE;
        npage -= c;
        blkno += ptod(c);   /*pcs*/
    }

    p->p_flag &= ~SLOCK;

    splhi();
    bp->av_forw = pfreelist.av_forw;
    pfreelist.av_forw = bp;
    if (pfreelist.b_flags&B_WANTED) {
        pfreelist.b_flags &= ~B_WANTED;
        wakeup((caddr_t)&pfreelist);
    }
    splx(ospl);
    syswait.swap--;
}


/*
 * Raw I/O. The arguments are
 * The strategy routine for the device
 * A buffer, which is usually NULL, or special buffer
 *   header owned exclusively by the device for this purpose
 * The device number
 * Read/write flag
 */
physio(strat, bp, dev, rw)
register struct buf *bp;
int (*strat)();
{
    register int c;
    int ocount;
    int hpf;
    int pgbase; /*pcs*/

    ocount = u.u_count;
    if (u.u_count == 0 || u.u_count > ptob(MAXBLK)) {
        printf("physio: u_count invalid (%x)\n", u.u_count);
        u.u_error = EFAULT;
        return;
    }
    if ((pgbase = useracc(u.u_base, (int) u.u_count, rw | B_PHYS)) == 0) { /*pcs*/
        printf("physio: useracc failed\n");
        u.u_error = EFAULT;
        return;
    }
    if (rw)
        sysinfo.phread++;
    else
        sysinfo.phwrite++;
    syswait.physio++;
    hpf = (bp == NULL);
    spl6();
    if (hpf) {
        while ((bp = pfreelist.av_forw) == NULL) {
            pfreelist.b_flags |= B_WANTED;
            sleep((caddr_t)&pfreelist, PRIBIO+1);
        }
        pfreelist.av_forw = bp->av_forw;
    } else while (bp->b_flags&B_BUSY) {
        bp->b_flags |= B_WANTED;
        sleep((caddr_t)bp, PRIBIO+1);
    }
    bp->b_flags = B_BUSY | B_PHYS | B_PT | rw;
    bp->b_error = 0;
    bp->b_proc = u.u_procp;
    bp->b_un.b_addr = u.u_base;
    bp->b_dev = dev;
    bp->b_paddr = ptob(pgbase) | poff(u.u_base); /*pcs*/

    u.u_procp->p_flag |= SLOCK;

    while ((int)u.u_count > 0 && bp->b_error==0) {
        bp->b_bcount = c = u.u_count>255*BSIZE ? 255*BSIZE : u.u_count;
        bp->b_blkno = u.u_offset >> BSHIFT;
        bp->b_flags &= ~B_DONE;
        (*strat)(bp);
        for (;;) {
            spl6(); if (bp->b_flags&B_DONE) break;
            sleep((caddr_t)bp, PRIBIO);
        }
        spl0();
        bp->b_un.b_addr += c;
        bp->b_paddr += c;   /*pcs*/
        u.u_count -= c;
        u.u_offset += c;
    }

    u.u_procp->p_flag &= ~SLOCK;
    if (runin) {
        runin = 0;
        wakeup(&runin);
    }

    /*
     * unlock pages
     */
    userrel(u.u_base, ocount, rw | B_PHYS, pgbase); /*pcs*/

    if (hpf) {
        bp->av_forw = pfreelist.av_forw;
        pfreelist.av_forw = bp;
        if (pfreelist.b_flags&B_WANTED) {
            pfreelist.b_flags &= ~B_WANTED;
            wakeup((caddr_t)&pfreelist);
        }
    } else if (bp->b_flags&B_WANTED)
        wakeup((caddr_t)bp);
    spl0();
    bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_PT);
    u.u_count = bp->b_resid;
    geterror(bp);
    syswait.physio--;
}

physck(nblocks, rw)
daddr_t nblocks;
{
    register unsigned over;
    register off_t upper, limit;
    struct a {
        int fdes;
        char    *cbuf;
        unsigned count;
    } *uap;

    limit = nblocks << BSHIFT;
    if (u.u_offset >= limit) {
        if (u.u_offset > limit || rw == B_WRITE)
            u.u_error = ENXIO;
        return(0);
    }
    upper = u.u_offset + u.u_count;
    if (upper > limit) {
        over = upper - limit;
        u.u_count -= over;
        uap = (struct a *)u.u_ap;
        uap->count -= over;
    }
    return(1);
}
