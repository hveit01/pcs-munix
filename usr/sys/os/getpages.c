/* @(#)getpages.c   1.5 */
static char* _Version = "@(#) RELEASE:  1.1  Oct 06 1986 /usr/sys/os/getpages.c ";

#include "sys/types.h"
#include "sys/tuneable.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/inode.h"
#include "sys/buf.h"
#include "sys/var.h"
#include "sys/sysinfo.h"
#include "sys/page.h"
#include "sys/pfdat.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/map.h"
#include "sys/swap.h"
#include "sys/debug.h"

/*
 *  In order to accomodate for the absence of reference bits
 *  on the VAX, the valid, ref, and ndref bits in the pte are used
 *  in conjunction to simulate reference bit maintainance through
 *  valid faults. This table describes the bit settings and what they
 *  mean:
 *
 *  v  r  nr    meaning
 *  -----------------------
 *  1  1  1     a valid page; certainly in the process working set
 *  0  1  1     page is in the working set, but has been set invalid
 *          in order to detect a reference. The page frame in
 *          the pte is still owned by the process.
 *  0  0  1     page is not in the working set. the page frame in
 *          the pte is still owned by the process, but it
 *          is a candidate for reclaimation by getpages().
 *  0  0  0     the page is truly invalid. Full vfault processing
 *          is required for this pte.
 */



/*  Data structures for swap/page out
 *  These data elements are currently shared by
 *  vhand and sched.
 */

caddr_t gpgsvaddr;  /* Kernel virtual window for i/o */
pte_t   *gpgspte;   /* System page table address of virtual window */
int gpgslock;       /* Lock for window -- set if in use */
int gpgswant;       /* Want flag for window -- set if needed */
unsigned    gpgsdma;    /*pcs*/


/*  Vhand. Wakes up periodically to update
 *  process working sets.
 */

vhand()
{
    register struct proc *p;
    register struct proc *p0;
    register reg_t *rp;
    register int cputime;

    /*  Allocate window for swap/page i/o
     */

    gpgsvaddr = pgalloc(NPGPT, PG_V|PG_KW, -1);
    gpgspte = (pte_t*)svtopte(gpgsvaddr);
    gpgsdma = malloc(dmamap, MAXBLK);       /*pcs*/

    /*  We wakeup every tune.t_vhandr seconds iff
     *  freemem is less than tune.t_vhandl
     */
    tune.t_vhandl = maxmem/v.v_vhndfrac;

    p0 = &proc[0];
loop:
    sleep((caddr_t)vhand, 0);

loop2:
    if ((p=p0) >= (struct proc*)v.ve_proc)
        p0 = p = &proc[0];
    
    for (;;) {
        if (p0 >= (struct proc*)v.ve_proc) 
            p0 = &proc[0];
        if (p < (struct proc*)v.ve_proc)
            p++;
        else
            p = &proc[0];
        if (p0 == p) break;
        if (p->p_stat == 0 ||       /*pcs*/
            p->p_stat == SZOMB || p->p_stat == SIDL) 
                continue;
        if ((p->p_flag & (SSYS|SLOCK|SLOAD)) != SLOAD)
                continue;
    
        p->p_flag |= SLOCK;
        ageprocess(p);
        p->p_flag &= ~SLOCK;
    }
    
    /*  Scan shared regions.  If the region is unused
     *  or it has been marked for aging, do it.
     */
    for (rp = ractive.r_forw; rp != &ractive; rp = rp->r_forw) {
        if (rp->r_incore == 0 || rp->r_flags & RG_AGE) {
            rp->r_flags &= ~RG_AGE;
            if (rp->r_flags & RG_LOCK)
                continue;
            reglock(rp);
            getpages(rp, 0);
            ageregion(rp);
            regrele(rp);
        }
    }
    goto loop;
}

/*  Age pages of a process.  Ages pages of private regions
 *  and marks shared regions for aging (RG_AGE) by vhand.
 */
ageprocess(p)
register struct proc *p;
{
    register preg_t *prp;
    register reg_t *rp;

    /*  Scan process regions.
     */
    for (prp = p->p_region; rp = prp->p_reg; prp++) {

        /*  Ignore locked regions
         */
        if (rp->r_flags & RG_LOCK)
            continue;

        /*  If the region is shared, mark it now,
         *  and age it later.  This is to avoid
         *  aging the same region more than once
         *  per cycle.  Otherwise, do it to it.
         */

        if(rp->r_type != RT_PRIVATE)
            rp->r_flags |= RG_AGE;
        else {
            /*  Clear reference bits and steal
             *  unused pages
             */

            reglock(rp);
            getpages(rp, 0);
            ageregion(rp);
            regrele(rp);
        }
    }
}

/*  Clear reference bits on the pages of a region.
 */
ageregion(rp)
register reg_t  *rp;
{
    register pte_t  *pt;
    register int    j;
    register int    pglim;
    register int    i;
    register int    seglim; /*pcs no register*/

    /*  Look at all of the segments of the region.
     */
    i = ptots(rp->r_pgoff);
    seglim = ptos(rp->r_pgoff + rp->r_pgsz);

    for ( ; i < seglim; i++) {
        /*  Look at all of the pages of the segment.
         */

        if (rp->r_pgoff > stopg(i))
            j = rp->r_pgoff - stopg(i);
        else
            j = 0;

        pt = rp->r_list[i] + j;
        pglim = rp->r_pgoff + rp->r_pgsz - stopg(i);
        if (pglim > NPGPT)
            pglim = NPGPT;

        for ( ; j < pglim; j++, pt++) {
            if (pt->pgi.pg_pte & PG_LOCK)
                continue;
            decpl(pt);
        }
    }
} 

/*  Swap out pages from region rp which is locked by
 *  our caller.  If hard is set, take all valid pages,
 *  othersize take only unreferenced pages
 */
getpages(rp, hard)
register reg_t  *rp;
{
    register pte_t  *pt;
    register pte_t  *pt1;
    register dbd_t  *dbd;       /*pcs no register */
    register int    j;
    register int    pglim;
    register int    i;
    register int    seglim;     /*pcs no reigster */
    int migrate;
    extern  int     freemem;

    migrate = 1;

    /*  If the region is marked "don't swap", then don't
     *  steal any pages from it.
     */

    if (rp->r_flags & RG_NOSWAP)
        return;
    
    pt = pt1 = 0;
    i = ptots(rp->r_pgoff);
    seglim = ptos(rp->r_pgoff + rp->r_pgsz);

    /*  Look through all of the segments of the region.
     */
    
    for ( ; i < seglim; i++) {

        /*  Look through segment's page table for valid
         *  pages to dump.
         */

        if (rp->r_pgoff > stopg(i))
            j = rp->r_pgoff - stopg(i);
        else
            j = 0;
        pt = rp->r_list[i] + j;
        pglim = rp->r_pgoff + rp->r_pgsz - stopg(i);
        if (pglim > NPGPT)
            pglim = NPGPT;
        pt1 = 0;
        
        for ( ; j < pglim; j++, pt++) {

            /*  We have a page, see if we want to steal it
             */

            if (hard == 0) {
                if (freemem >= tune.t_vhandl)
                    break;
            }
            /*  Don't steal it, if the page has
             *  been referenced recently
             */

            if ((pt->pgi.pg_pte & PG_V)==0 || pt->pgi.pg_pte & PG_LOCK) {
                swapchunk(rp, pt1, pt);
                pt1 = 0;
                continue;
            }

            if (hard == 0 && (getpl(pt) & PL_ROR) != 0) {
                swapchunk(rp, pt1, pt);
                pt1 = 0;
                continue;
            }

            /*
             *  The page will be made truly invalid. Clear
             *  all valid and ref bits, so the code in
             *  Xvalflt doesn't pick up on a page that's on
             *  its way out
             */
            pt->pgi.pg_pte &= ~PG_V;
            andpl(pt, ~PL_REF);

            /*
             *  See if this page must be written to swap.
             */

            dbd = (dbd_t *)pt + NPGPT;
            
            switch (dbd->dbd_type) {
            case DBD_NONE: {
                register pfd_t *pfd; /*pcs no register */

                /*  This may be a copy-on-write
                 *  page which has been written
                 *  to swap by another process
                 *  which shares the page.  If
                 *  so, just use the same swap
                 *  block.  In the rare case
                 *  that the swap use count
                 *  overflows, we allocate
                 *  another swap page.  This is
                 *  only possible if a process
                 *  does 255 forks without the
                 *  children exec'ing.
                 */

                pfd = &pfdat[pt->pgm.pg_pfn];
                if (pfd->pf_flags & P_HASH) {
                    dbd->dbd_type = DBD_SWAP;
                    dbd->dbd_swpi = pfd->pf_swpi;
                    dbd->dbd_blkno = pfd->pf_blkno;
                    if (swpinc(dbd, "getpages") == 0) {
                        dbd->dbd_type = DBD_NONE;
                        break;
                    }
                    memfree(rp, pt, 1);
                    swapchunk(rp, pt1, pt);
                    pt1 = 0;
                    continue;
                }
                break;
            }
            case DBD_SWAP:
                
                /*  See if this page has been
                 *  modified since it was read
                 *  in from swap.  If not, then
                 *  just use the copy already on
                 *  the swap file unless we are
                 *  trying to delete the swap file.
                 *  If we are, then release the
                 *  current swap copy and write
                 *  the page out to another
                 *  swap file.
                 */
                
                if ((getpl(pt) & PL_MOD) == 0 &&
                    (swaptab[dbd->dbd_swpi].st_flags & ST_INDEL) == 0) {
                    minfo.unmodsw++;
                    memfree(rp, pt, 1);
                    swapchunk(rp, pt1, pt);
                    pt1 = 0;
                    continue;
                }

                /*  The page has been modified.
                 *  Release the current swap
                 *  block and add it to the list
                 *  of pages to be swapped out
                 *  later.
                 */

                if (swfree1(dbd) == 0)
                    if (!pbremove(rp, dbd))
                        panic("getpages - pbremove");
                break;
            case DBD_FILE:
                /*  See if the page has been
                 *  modified.  If it has, then
                 *  we cannot use the copy on
                 *  the file and must assign
                 *  swap for it.
                 */

                if (migrate || getpl(pt) & PL_MOD) {
                    dbd->dbd_type = DBD_NONE;
                    break;
                }
                /*  Page has not been modified.
                 *  Just point to the copy on
                 *  the file.
                 */

                minfo.unmodfl++;
                memfree(rp, pt, 1);
                swapchunk(rp, pt1, pt);
                pt1 = NULL;
                continue;
            }
            
            if (pt1 == 0)
                pt1 = pt;
            else if ((pt - pt1) >= tune.t_maxsc) {
                swapchunk(rp, pt1, pt);
                pt1 = pt;
            }
        }
        
        /*  If there are any pages in this segment to be
         *  written out, then write them now.
         */

        swapchunk(rp, pt1, pt);
        pt1 = NULL;

        if (hard == 0) {
            if (freemem >= tune.t_vhandl)
                break;
        }
    }


    /*
     *  Ran off bottom.  Write out anything which remains.
     */

    swapchunk(rp, pt1, pt);
} 

/*  Swap out a contiguous chunk of user pages.
 */
swapchunk(rp, pt1, pt2)
reg_t       *rp;    /* Ptr to region being swapped. */
pte_t       *pt1;   /* Ptr to 1st pte to swap out. */
register pte_t  *pt2;   /* Ptr to one past last pte to swap out. */
{
    register pte_t  *pt, *spt;
    int     npages;
    dbd_t       *dbd;

    if (pt1 == 0)
        return;
    
    while (gpgslock) {
        gpgswant++;
        sleep((caddr_t)&gpgslock, PSWP);
    }
    gpgslock = 1;
    
    npages = pt2 - pt1;

    pt = pt1;
    spt = gpgspte;
    while (pt < pt2) {
        spt->pgi.pg_pte = pt->pgi.pg_pte | PG_V;
        pt++;
        spt++;
    }

    dbd = (dbd_t*)pt1 + NPGPT;
    swalloc(dbd, npages);
    clrcache();
    swap(dbd, gpgspte, gpgsvaddr, npages, 0);
    memfree(rp, pt1, npages);

    gpgslock = 0;
    if (gpgswant) {
        gpgswant = 0;
        wakeup((caddr_t)&gpgslock);
    }
}

swapchunkin(pt1, n)
pte_t *pt1;
{
    register pte_t *pt, *spt, *pt2;
    dbd_t *dbd;

    while (gpgslock) {
        gpgswant++;
        sleep((caddr_t)&gpgslock, PSWP);
    }
 
    gpgslock = 1;
    
    pt = pt1;
    pt2 = &pt1[n];
    spt = gpgspte;
    while (pt < pt2) {
        spt->pgi.pg_pte = pt->pgi.pg_pte | (PG_V|PG_KW);
        pt++;
        spt++;
    }

    dbd = (dbd_t*)pt1 + NPGPT;
    clrcache();
    swap(dbd, gpgspte, gpgsvaddr, n, 1); 

    gpgslock = 0;
    if (gpgswant) {
        gpgswant = 0;
        wakeup((caddr_t)&gpgslock);
    }
}

/*  Free memory pages.
 */
memfree(rp, pt, size)
register reg_t  *rp;
register pte_t  *pt;
register int size;
{
    register struct pfdat   *pfd;
    register dbd_t      *dbd;
    extern  int     freemem;

    dbd = (dbd_t *)pt + NPGPT;
    rp->r_nvalid -= size;

    for (; --size >= 0; pt++, dbd++) {

        pfd = &pfdat[pt->pgm.pg_pfn];

        /*  See if the page is on swap and not on the
         *  hash list.  If so, insert it there now.
         */
        if (dbd->dbd_type == DBD_SWAP && !(pfd->pf_flags & P_HASH)) {
            pfd->pf_flags |= P_DONE;
            pinsert(rp, dbd, pfd);
        }

        /*  free unused pages.
         */

        ASSERT(pfd->pf_use > 0);
        if (--pfd->pf_use == 0) {
            /*  Put pages at end of queue since they
             *  represent disk blocks and we hope they
             *  will be used again soon.
             */
            pfd->pf_prev = phead.pf_prev;
            pfd->pf_next = &phead;
            phead.pf_prev = pfd;
            pfd->pf_prev->pf_next = pfd;
            pfd->pf_flags |= P_QUEUE;
            freemem++;
            minfo.freedpgs++;
        }
    }
}
