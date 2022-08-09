/* @(#)region.c 1.15 */
static char *_Version = "@(#) RELEASE:  2.0  Jul 09 1987 /usr/sys/os/region.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/page.h"
#include "sys/seg.h"
#include "sys/pfdat.h"
#include "sys/signal.h"
#include "sys/dir.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/inode.h"
#include "sys/var.h"
#include "sys/buf.h"
#include "sys/debug.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/map.h"
#include "sys/tuneable.h" /*pcs*/
#include "sys/sysinfo.h"  /*pcs*/

reg_t   nullregion;
preg_t  nullpregion;

extern int sbittab[];
extern int maxupts;

void reginit()
{
    register reg_t *rp;

    rfree.r_forw = &rfree;
    rfree.r_back = &rfree;

    ractive.r_forw = &ractive;
    ractive.r_back = &ractive;

    for (rp = region; rp < &region[v.v_region]; rp++) {
        rp->r_back = rfree.r_back;
        rp->r_forw = &rfree;
        rfree.r_back->r_forw = rp;
        rfree.r_back = rp;
    }
}

reglock(rp)
register reg_t *rp;
{
    while (rp->r_flags & RG_LOCK) {
        rp->r_flags |= RG_WANTED;
        sleep((caddr_t)rp, PZERO);
    }
    rp->r_flags |= RG_LOCK;
}

regrele(rp)
register reg_t *rp;
{
    rp->r_flags &= ~RG_LOCK;
    if (rp->r_flags & RG_WANTED) {
        rp->r_flags &= ~RG_WANTED;
        wakeup((caddr_t)rp);
    }
}

/*
 * Allocate a new region.
 * Returns a locked region pointer or NULL on failure
 * The region is linked into the active list.
 */

reg_t *
allocreg(ip, type)
register struct inode   *ip;
{
    register reg_t *rp;

    if ((rp = rfree.r_forw) == &rfree) {
        printf("Region table overflow\n");
        u.u_error = ENOMEM;
        return(NULL);
    }
    /*
     * Remove from free list
     */
    rp->r_back->r_forw = rp->r_forw;
    rp->r_forw->r_back = rp->r_back;

    /*  Initialize region fields and bump inode reference
     *  count.
     */

    rp->r_incore = 0;
    rp->r_type = type;
    rp->r_iptr = ip;
    rp->r_flags = RG_LOCK;
    if(ip != NULL)
        ip->i_count++;

    if(type != RT_PRIVATE) {
        /*
         * Link onto active list
         */
        rp->r_forw = ractive.r_forw;
        rp->r_back = &ractive;
        ractive.r_forw->r_back = rp;
        ractive.r_forw = rp;
    }

    return(rp);
}

/*
 * Free an unused region table entry.
 */
void
freereg(rp)
register reg_t *rp; /* pointer to a locked region */
{
    register struct inode   *ip;
    register int        i;
    register int        lim;
    register int        size;
    register pte_t      *pt;
    register int        tsize;

    ASSERT(rp->r_flags & RG_LOCK);

    /*  If the region is still in use, then don't free it.
     */

    if(rp->r_refcnt != 0)
        return;


    /*
     * Decrement use count on associated inode
     */

    if (ip = rp->r_iptr) {
        if (rp->r_type == RT_STEXT) {
            ip->i_flag &= ~ITEXT;
            rp->r_rmt_ino = 0;          /*pcs*/
            rp->r_rmt_dev = (dev_t)-1;  /*pcs*/
            rp->r_rmt_id = -1;          /*pcs*/
        }
        if (ip->i_flag&ILOCK)
            ip->i_count--;
        else {
            plock(ip);                  /*pcs*/
            iput(ip);
        }
    }

    /*  Free the memory pages and the page tables and
     *  disk use maps.  These latter are always allocated
     *  together in pairs in a contiguous 256 word piece
     *  of kernel virtual address space.  Note that the
     *  pfree for the first page table is special because
     *  pages may not have been allocated from the beginning
     *  of the segment.  The last page table is also special
     *  since it may not have been fully allocated.
     */
    
    tsize = rp->r_pgoff + rp->r_pgsz;
    i = ptots(rp->r_pgoff);
    lim = ptos(tsize);
    ASSERT(i <= lim);

    for(  ;  i < lim  ;  i++){
        pt = rp->r_list[i];
        ASSERT(pt != 0);
        size = tsize - stopg(i);
        if(size > NPGPT)
            size = NPGPT;
        if(rp->r_pgoff > stopg(i)){
            size -= rp->r_pgoff - stopg(i);
            pt += rp->r_pgoff - stopg(i);
        }
        pfree(rp, pt, pt + NPGPT, size);
        sptfree(rp->r_list[i], 2, 0);       /*pcs*/
    }

    /*  Free the list.
     */
    spttidy();                              /*pcs*/
    availsmem += rp->r_pgsz;                /*pcs*/
    listfree(rp->r_list, rp->r_listsz);     /*pcs*/

    if(rp->r_type != RT_PRIVATE) {
        /*
         * Remove from active list
         */

        rp->r_back->r_forw = rp->r_forw;
        rp->r_forw->r_back = rp->r_back;
    }

    regrele(rp);
    *rp = nullregion;

    /*
     * Link into free list
     */

    rp->r_forw = rfree.r_forw;
    rp->r_back = &rfree;
    rfree.r_forw->r_back = rp;
    rfree.r_forw = rp;
}

/*
 * Attach a region to a process' address space
 */
preg_t *
attachreg(rp, up, vaddr, type, prot)
register reg_t *rp; /* pointer to region to be attached */
struct user *up;    /* pointer to U-block of process (needed by fork) */
caddr_t vaddr;      /* virtual address to attach at */
int type;       /* Type to make the pregion. */
int prot;       /* permissions for segment table entries. */
{
    register preg_t *prp;
    register int *st, ste, i;

    ASSERT(rp->r_flags & RG_LOCK);

    /*  Check attach address.
     *  It must be segment aligned.
     */

    if (((int)vaddr & SOFFMASK) || 
        (type != RT_SHMEM && vaddr >= (caddr_t)SYSVA) ) { /*pcs*/
        u.u_error = EINVAL;
        return(NULL);
    }

    /*  Allocate a pregion.  We should always find a
     *  free pregion because of the way the system
     *  is configured.
     */

    prp = findpreg(up->u_procp, PT_UNUSED);
    if (prp == NULL) {
        u.u_error = EMFILE;
        return(NULL);
    }

    /*  init pregion
     */

    prp->p_reg = rp;
    prp->p_regva = vaddr;
    prp->p_type = type;
    if(prot == SEG_RO)
        prp->p_flags |= PF_RDONLY;

    /*  Check that region does not go beyond end of virtual
     *  address space.
     */

    if (chkattach(up, prp)) { /*pcs*/

        /* Region beyond end of address space.
         * Undo what has been done so far
         */

        *prp = nullpregion;
        u.u_error = EINVAL;
        return(NULL);
    }

    /*  Load the segment table.
     */

    loadstbl(up); /*pcs*/

    ASSERT(up->u_procp->p_flag&SLOAD);
    ++rp->r_incore;
    ++rp->r_refcnt;
    up->u_procp->p_size += rp->r_pgsz;

    return(prp);
}

/*
 * Detach a region from the current process' address space
 */
void
detachreg(up, prp)
struct user *up;
register preg_t *prp;
{
    register reg_t  *rp;
    register int    i;
    register int    *st;
    struct proc     *pp;
    register int segnum, pnum;

    pp = up->u_procp;
    rp = prp->p_reg;
    ASSERT(rp != NULL);
    ASSERT(rp->r_flags & RG_LOCK);
    if (rp->r_list == NULL)
        goto nullreg;
    ASSERT(rp->r_list[0] != 0);

    if (pp == curproc) {                            /*pcs*/
        /*
        * Invalidate segment table entries pointing at the region
        */
        segnum = snum(prp->p_regva);                /*pcs*/
        pnum = ptos((rp->r_pgsz + rp->r_pgoff));    /*pcs*/
        for (i = 0; i < pnum; i++)                  /*pcs*/
            _ptbr[segnum++] = 0;                    /*pcs*/
        clratb();                                   /*pcs*/
    }
    /*  Decrement process size by size of region.
     */

    up->u_procp->p_size -= rp->r_pgsz;

    /*
     * Decrement use count and free region if zero
     * and RG_NOFREE is not set, otherwise unlock.
     */
nullreg:
    --rp->r_incore;
    if (--rp->r_refcnt == 0 && !(rp->r_flags & RG_NOFREE))
        freereg(rp);
    else
        regrele(rp);
    
    /*  Clear the proc region we just detached.
     */
    
    for (i = prp - up->u_procp->p_region; i < pregpp-1; i++, prp++)
        *prp = *(prp+1);

    *prp = nullpregion;
}

/*
 * Duplicate a region
 *  rp: region to be dup'd
 */

reg_t *
dupreg(rp, type)
register reg_t *rp;
int type;
{
    register int    i;
    register int    j;
    register int    size;
    register pte_t  *ppte;
    register pte_t  *cpte;
    register pte_t  **plp;
    register pte_t  **clp;
    register reg_t  *rp2;
    int protection;

    ASSERT(rp->r_flags & RG_LOCK);

    /* If region is shared, there is no work to do.
     * Just return the passed region.  The region reference
     * counts are incremented by attachreg
     */

    if((rp->r_type != RT_PRIVATE) && (type == SEG_CW))
        return(rp);
    
    if ((availsmem - rp->r_pgsz) < tune.t_minasmem)     /*pcs*/
        return (NULL);                                  /*pcs*/
    else {                                              /*pcs*/
        availsmem -= rp->r_pgsz;                        /*pcs*/
        escdeadlock(rp, rp->r_pgsz);                    /*pcs*/
    }                                                   /*pcs*/

    /*
     * Need to copy the region.
     * Allocate a region descriptor
     */

    if ((rp2 = allocreg(rp->r_iptr, rp->r_type)) == NULL) {
        availsmem += rp->r_pgsz;                        /*pcs*/
        return(NULL);
    }   
    /*  Allocate a list for the new region.
     */

    rp2->r_listsz = rp->r_listsz;
    rp2->r_list = (pte_t **)listalloc(rp2->r_listsz);   /*pcs*/
    if(rp2->r_list == NULL){
        freereg(rp2);
        availsmem += rp->r_pgsz;                        /*pcs*/
        return(NULL);
    }

    /*
     * Copy pertinent data to new region
     */

    rp2->r_pgsz = rp->r_pgsz;
    rp2->r_pgoff = rp->r_pgoff;
    rp2->r_nvalid = rp->r_nvalid;
    rp2->r_filesz = rp->r_filesz;

    /* Scan the parents page table  list and fix up each page table.
     * Allocate a page table and map table for the child and
     * copy it from the parent.
     */

    if (type != SEG_CW)
        protection = (type == SEG_RO) ? PTE_UR : PTE_UW;
    for(i = 0  ;  i < ptos(rp->r_pgoff + rp->r_pgsz)  ;  i++){
        plp = &rp->r_list[i];
        clp = &rp2->r_list[i];

        /* Allocate a page table and map table for the child.
         */

        if((cpte = (pte_t *)sptalloc(2)) == 0){         /*pcs*/

            /* We could not allocate a page table and map.
             * Set the size to the number of page tables
             * we have actually allocated and then free
             * the region.
             */

            rp2->r_pgsz = stopg(i);
            freereg(rp2);
            availsmem += rp->r_pgsz;                        /*pcs*/
            return(NULL);
        }

        /* If we are before the offset, then there are no
         * pages to duplicate.  Otherwise, compute the number
         * of pages.  The first page table may be special
         * because of the offset and the last because the
         * size may not be an integral number of segments.
         */

        *clp = cpte;
        if(i < ptots(rp->r_pgoff))
            continue;
        ppte = *plp;

        /* Get the total number of unmapped pages remaining.
         * This is the total size of the region minus the
         * number of segments for which we have allocated
         * page tables already.
         */

        size = rp->r_pgoff + rp->r_pgsz - stopg(i);

        /* If this size is greater than a segment, then
         * we will only process a segment.
         */

        if(size > NPGPT)
            size = NPGPT;

        /* Check for the first segment after the offset.
         * This is not a full segment in general.
         */
        j = rp->r_pgoff;                            /*pcs*/
        if ((j -= stopg(i)) >0) {                   /*pcs*/
            size -= j;                              /*pcs*/
            ppte += j;                              /*pcs*/
            cpte += j;                              /*pcs*/
        }                                           /*pcs*/

        ASSERT(size > 0  &&  size <= NPGPT);

        /* Check each parents page and then copy it to
         * the childs pte.  Also check the map table
         * entries.
         */

        for (j = 0;  j < size;  j++, ppte++, cpte++) {
            dbd_t map;

            /* Change writeable pages to copy-on write.
             */
            if (type == SEG_CW) {
                if(ppte->pgm.pg_prot == PTE_UW) {
                    ppte->pgm.pg_prot = PTE_UR;
                    ppte->pgm.pg_cw = 1;
                }

                *cpte = *ppte;
            } else {
                *cpte = *ppte;
                cpte->pgm.pg_prot = protection;
            }

            /* if the page is in core, increment
             * the use count on the pfdat */
            if (ppte->pgm.pg_v) {
                struct pfdat *pfd;

                pfd = pfdat + ppte->pgm.pg_pfn;
                ASSERT(pfd->pf_use != 0); 
                pfd->pf_use++;
            }

            /* Increment the swap use count for pages which
             * are on swap.
             */
            map = *(dbd_t *)(ppte + NPGPT);
            if (map.dbd_type == DBD_SWAP) {
                ASSERT(swpuse(&map) != 0);
                if(!swpinc(&map, "dupreg")){

                    /* swap use count overflowed.
                     * Free the region and return
                     * an error.
                     */

                    ((dbd_t *)(cpte + NPGPT))->dbd_type =
                        DBD_NONE;
                    freereg(rp2);
                    availsmem += rp->r_pgsz;
                    u.u_error = ENOMEM;
                    return(NULL);
                }
            }
            *(dbd_t *)(cpte + NPGPT) = map;
        }
    }
    clratb();

    return(rp2);
}

/*
 * Change the size of a region
 *  change == 0  -> no-op
 *  change  < 0  -> shrink
 *  change  > 0  -> expand
 * For expansion, you get (fill) real pages (change-fill) demand zero pages
 * For shrink, the caller must flush the ATB
 * Returns 0 on no-op, -1 on failure, and 1 on success.
 */

growreg(up, prp, change, fill, type)
struct user *up;
register preg_t *prp;
{
    register pte_t  *pt;
    register int    i;
    register reg_t  *rp;
    register int    size;
    register int    osize;
    int lotohi;
    int start, end;
    int k;
    struct dbd *dbd;

    rp = prp->p_reg;

    ASSERT(rp != NULL);
    ASSERT(rp->r_flags & RG_LOCK);
    ASSERT(change >= 0 || (-change <= rp->r_pgsz));
    ASSERT(rp->r_refcnt == 1);

    if (change == 0)
        return(0);
    
    lotohi = prp->p_type != PT_STACK;       /*pcs*/
    osize = rp->r_pgoff + rp->r_pgsz;
    
    if (change < 0) {

        /*  The region is being shrunk.  Compute the new
         *  size and free up the unneeded space.
         */
        availsmem -= change;                /*pcs*/
        if (lotohi) {
            start = osize + change;
            end = osize;
        } else {
            start = rp->r_pgoff;
            end = rp->r_pgoff - change;
        }

        i = ptots(start);

        for(  ;  i < ptos(end)  ;  i++){
            /*  Free up the allocated pages for
             *  this segment.
             */
            pt = rp->r_list[i];
            size = (long)end - stopg(i);        /*pcs*/
            if(size > NPGPT)
                size = NPGPT;
            if(start > stopg(i)){
                size -= start - stopg(i);
                pt += start - stopg(i);
            }
            pfree(rp, pt, pt + NPGPT, size);
        }

        /*  Free up the page tables which we no
         *  longer need.
         */

        rptexpand(prp, change, lotohi);     /*pcs*/
    } else {
        /*  We are expanding the region.  Make sure that
         *  the new size is legal and then allocate new
         *  page tables if necessary.
         */
        if ((availsmem-change) < tune.t_minasmem) { /*pcs*/
            up->u_error = EAGAIN;                   /*pcs*/
            return(-1);                             /*pcs*/
        }

        if (chkgrowth(up, prp, change) != 0 ||      /*pcs*/
            rptexpand(prp, change, lotohi) != 0) {  /*pcs*/
            up->u_error = ENOMEM;                   
            return(-1);                             
        }

        /*  Initialize the new page tables and allocate
         *  pages if required.
         */
        availsmem -= change;
        if (lotohi) {
            start = osize;
            end = osize + change;
        } else {
            start = rp->r_pgoff;
            end = rp->r_pgoff + change;
        }

        i = ptots(start);
        for( ; i < ptos(end) ; i++){
            pt = rp->r_list[i];
            size = (long)end - stopg(i);                /*pcs*/
            if (size > NPGPT)
                size = NPGPT;
            if (start > stopg(i)){
                size -= start - stopg(i);
                pt += start - stopg(i);
            }

            ASSERT(size > 0  &&  size <= NPGPT);


            if (lotohi) {
                if(fill){
                    register int    fillsz;

                    fillsz = min(fill, size);           /*pcs*/
                    if(ptmemall(rp, pt, fillsz, 1))
                    panic("growreg - ptmemall failed");
                    dbd = (dbd_t *)(pt + NPGPT);
                    for (k=0; k<fillsz; k++, dbd++)
                        dbd->dbd_type = type;
                    if (type == DBD_DZERO) {
                        pte_t *tmp;

                        tmp = pt;
                        for (k=0; k<fillsz; k++) {
                           clearpage(tmp->pgm.pg_pfn);
                           tmp++;
                        }
                    }

                    fill -= fillsz;
                    size -= fillsz;
                    pt += fillsz;
                }

                if(size){
                    register    proto;
                    pte_t       pte;

                    pte.pgi.pg_pte = 0;
                    pte.pgm.pg_prot = (type==DBD_NONE)?
                                0 : PTE_UW;

                    proto = pte.pgi.pg_pte;

                    while (--size >= 0){
                       pt->pgi.pg_pte = proto;
                       ((dbd_t *)(pt+NPGPT))->dbd_type =
                                    type;
                       pt++;
                    }
                }
            } else {
                int demsize;

                if (fill < change) {
                    register    proto;
                    pte_t       pte;

                    pte.pgi.pg_pte = 0;
                    pte.pgm.pg_prot = (type==DBD_NONE)?
                                0 : PTE_UW;

                    proto = pte.pgi.pg_pte;

                    demsize = min(size, change-fill);
                    fill += demsize;
                    size -= demsize;
                    while (demsize-- > 0) {
                       pt->pgi.pg_pte = proto;
                       ((dbd_t *)(pt+NPGPT))->dbd_type =
                                    type;
                       pt++;
                    }
                }

                if (size) {
                    if (ptmemall(rp, pt, size, 1))
                    panic("growreg p1 - ptmemall failed");
                    dbd = (dbd_t *)(pt + NPGPT);
                    for (k=0; k<size; k++, dbd++)
                        dbd->dbd_type = type;
                    if (type == DBD_DZERO) {
                        pte_t *tmp;

                        tmp = pt;
                        for (k=0; k<size; k++) {
                           clearpage(tmp->pgm.pg_pfn);
                           tmp++;
                        }
                    }

                    pt += size;
                }
            }
        }
    }

    rp->r_pgsz += change;
    up->u_procp->p_size += change;
    if (prp->p_type == PT_STACK)
        up->u_ssize = prp->p_reg->r_pgsz;   /*pcs*/
    loadstbl(up);
    return(1);
}

/*
 * Check that grow of a pregion is legal
 */
chkgrowth(up, prp, change, lotohi)
register struct user *up;
register preg_t *prp;
register int change;
{
    register reg_t  *rp;
    struct pregion  *prp2; /*pcs*/
    register int    size;
    struct proc     *pp;
    caddr_t         va1, va1end;
    caddr_t         va2, va2end;
    
    if (change < 0)
        return (0);

    pp = up->u_procp;
    rp = prp->p_reg;

    if ((short)(rp->r_pgsz + change) < 0)                   /*pcs*/
        return (-1);                                        /* ^ */
    size = stob(ptos(rp->r_pgoff + rp->r_pgsz + change));   /* | */
    va1 = prp->p_regva;
    va1end = va1 + size;
    if (prp->p_type != PT_STACK && (unsigned)va1end >= (unsigned)SYSVA)
        return (-1);
    for (prp2 = pp->p_region; prp2->p_reg; prp2++) {
        if (prp != prp2) {
            rp = prp2->p_reg;                                   
            size = stob(ptos(rp->r_pgoff + rp->r_pgsz));
            va2 = prp2->p_regva;
            va2end = va2 + size;
            if ((va2 < va1end && va2end > va1) ||
                (va1 < va2end && va1end > va2))
                return (-1);                                /* | */
        }                                                   /* v */
    }                                                       /*pcs*/
    
    return (0);
}

/*
 * Check that attach of a pregion is legal
 */
chkattach(up, prp)
struct user *up;
preg_t *prp;
{
    return chkgrowth(up, prp, 0);
}

/*completely pcs specific*/
loadstbl(up)
struct user *up;
{
    register preg_t *prp;
    register reg_t  *rp;
    register struct ptbrmap *pt, *pmap;
    register struct proc *p;
    register int pnum, segnum;
    int size;

    p = up->u_procp;

    if (up->u_nsegs) 
        listfree(up->u_pmap, up->u_nsegs);

    size = 0;
    for (prp = p->p_region; (rp=prp->p_reg) != NULL; prp++)
        size += ptos(rp->r_pgoff + rp->r_pgsz);

    if (size==0) {
        up->u_nsegs = 0;
        up->u_pmap = 0;
        return;
    }
    if ((pt = (struct ptbrmap*)listalloc(size)) == NULL)
        panic("loadstbl: cannot get memory");
    pmap = pt;
    for (prp = p->p_region; (rp=prp->p_reg); prp++) {
        segnum = btots(prp->p_regva) & COFFMASK;
        for (pnum = 0; pnum < ptos(rp->r_pgoff+rp->r_pgsz); pnum++, segnum++, pt++) {
            pt->p_segno = segnum;
            pt->p_descr = vtop(rp->r_list[pnum]);
        }
    }
    up->u_nsegs = size;
    up->u_pmap = pmap;
    if (up->u_procp == curproc)
        setptbr(up);
}

extern short colourpt[];

setptbr(up)
struct user *up;
{
    register int nsegs;
    register struct ptbrmap *pmap;
    static short ptsave;
    
    nsegs = up->u_nsegs;
    pmap = up->u_pmap;
    
    splhi();
    ptsave = _ptbr[0x3f7];
    _ccr &= ~7; /* clear atb, ptbr, cache */
    _ptbr[0x3f7] = ptsave;
    if (colourpt[0]) {
        _ptbr[0x300] = colourpt[0];
        if (colourpt[1])
            _ptbr[0x301] = colourpt[1];
    }
    spl0();
    while (nsegs--) {
        _ptbr[pmap->p_segno] = pmap->p_descr;       
        pmap++;
    }
}

/*
 * Expand page table space for process
 *  change  -> # of page tables to add
 */
rptexpand(prp, change, lotohi)
register struct pregion *prp;
int change, lotohi;
{
    register struct region *rp;
    register pte_t **pt1;
    pte_t **ptp;
    pte_t **pt2;
    register int osz, nsz, nlsz;
    int nsize;
    
    rp = prp->p_reg;
    if (lotohi)
        osz = rp->r_pgoff + rp->r_pgsz;
    else
        osz = rp->r_pgsz;
    
    nsz = osz + change;
    escdeadlock(rp, nsz);
    if (ptos(nsz) < ptos(osz)) {
        /* new size is < old size
         * must free pages */
        if (lotohi) {
            /* release and clear the excess pages */
            for (pt1 = &rp->r_list[ptos(nsz)]; pt1 < &rp->r_list[ptos(osz)]; pt1++) {
                sptfree(*pt1, 2, 0);
                *pt1 = 0;
            }
            spttidy();
        } else {
            /* there is an offset: release pages, shrink gap */
            nsize = ptos(osz) - ptos(nsz);
            ptp = rp->r_list;
            for (pt2 = &rp->r_list[nsize]; pt2  < &rp->r_list[ptos(osz)]; ) {
                sptfree(*ptp, 2, 0);
                *ptp++ = *pt2;
                *pt2++ = 0;
            }
            /* clear pages at the end */
            for ( ; *ptp != 0; ) {
                sptfree(*ptp, 2, 0);
                *ptp++ = 0;
            }
            spttidy();
            /* adjust offset */
            rp->r_pgoff -= stopg(nsize);
            prp->p_regva += stob(nsize);
        }
        /* shrink page table list if too large */
        if (ptos(nsz) < rp->r_listsz) {
            if (nsz > 0) {
                /* allocate table of correct size and copy entries */
                pt2 = (pte_t**)listalloc(ptos(nsz));
                if (pt2 == 0)
                    return 0;
                bcopy(rp->r_list, pt2, ptos(nsz)*4);
            } else
                /* new size is 0 */
                pt2 = 0;
            /* discard old table, and make new one permanent */
            listfree(rp->r_list, rp->r_listsz);
            rp->r_list = pt2;
            rp->r_listsz = ptos(nsz);
        }
    }
    /* did the table shrink? Adjust new offset */
    if (change <= 0) {
        if (lotohi==0)
            rp->r_pgoff -= change;
        return 0;
    }
    
    /* new list table size */
    nlsz = ptos(nsz);
    if (nlsz > rp->r_listsz) {
        /* is new table larger? grow it */
        if ((pt2 = (pte_t**)listalloc(nlsz)) == 0)
            return -1;
        /* copy old entries if necessary */
        if (rp->r_list) {
            bcopy(rp->r_list, pt2, ptos(osz)*4);
            listfree(rp->r_list, rp->r_listsz);
        }
        /* make new table permanent */
        rp->r_list = pt2;
        rp->r_listsz = nlsz;
    }
    
    /* no offset? */
    if (lotohi) {
        ptp = pt1 = &rp->r_list[ptos(osz)];
        pt2 = &rp->r_list[ptos(nsz)];
    } else {
        /* we must add an offset */
        nsize = ptos(nsz) - ptos(osz);
        ptp = &rp->r_list[ptos(osz)];
        pt2 = &rp->r_list[ptos(nsz)];
        /* copy items */
        while (ptp > rp->r_list)
            *--pt2 = *--ptp;
        pt1 = ptp;
        /* adjust offset */
        rp->r_pgoff  += stopg(nsize) - change;
        prp->p_regva -= stob(nsize);
    }
    /* free the items of the gap */
    for ( ; pt1 < pt2; pt1++) {
        *pt1 = (pte_t*)sptalloc(2);
        if (*pt1 == 0) {
            while (--pt1 >= ptp) 
                sptfree(*pt1, 2, 0);
            spttidy();
            return -1;
        }
    }
    
    return 0;
}

loadreg(prp, vaddr, ip, off, count)
register preg_t     *prp;
caddr_t         vaddr;
register struct inode   *ip;
{
    register reg_t  *rp;
    register int    change;
    int more;

    /*  Grow the region to the proper size to load the file.
     */
    
    more = 0;

    rp = prp->p_reg;
    change = vaddr - prp->p_regva;

    if(growreg(&u, prp, btotp(change), 0, DBD_NONE) < 0) { /*pcs*/
        return(-1);
    }

    if (rp->r_type != RT_PRIVATE)
        more = 4;
    
    if (growreg(&u, prp, btop(change+count+more) - btotp(change), 0, DBD_DFILL) < 0) /*pcs*/
        return(-1);         /*pcs*/

    rp->r_pgoff = (change = btotp(change));
    rp->r_pgsz -= change;   /*pcs*/
    availsmem += change;    /*pcs*/

    /*  Set up to do the I/O.
     */

    u.u_segflg = 0;
    u.u_base = vaddr;
    u.u_count = count;
    u.u_offset = off;

    /*  We must unlock the region here because we are going
     *  to fault in the pages as we read them.  No one else
     *  will try to use the region before we finish because
     *  the RG_DONE flag is not set yet.
     */

    regrele(rp);
    if (ip->i_flag & ILAND) {
        if (uireadi(ip) < 0) {
            reglock(rp);
            return -1;
        }
    } else
        readi(ip);

    reglock(rp);
    rp->r_flags |= RG_DONE;
    if(rp->r_flags & RG_WAITING){
        rp->r_flags &= ~RG_WAITING;
        wakeup(&rp->r_flags);
    }
    if(u.u_count) {
        return(-1);
    }

    /*  Clear the last (unused)  part of the last page.
     */
    
    vaddr += count;
    count = ptob(1) - poff(vaddr);
    if(count > 0 && count < ptob(1))
        if(uclear(vaddr, count) < 0) {
            return(-1);
        }

    return(0);
}

mapreg(prp, vaddr, ip, off, count)
preg_t  *prp;
caddr_t     vaddr;
struct inode    *ip;
int     off;
register int    count; /*d7*/
{
    register int    i; /*d6*/
    register int    j; /*d5*/
    register int    nppblk; /*20*/
    register reg_t  *rp; /*a5*/
    int             edev; /*24*/
    int             gap; /*28*/
    int             seglim; /*2c*/
    int             boff; /*30*/
    int             size; /*34*/
    dbd_t           *dbd; /*38*/
    short           firsttime;/*3a*/
    int             more;/*40 pcs*/

    more = 0;

    /*  If the block number list is not built,
     *  then build it now.
     */
    
    if(ip->i_map == 0)
        bldblklst(ip);

    /*  Get region pointer and effective device number.
     */

    rp = prp->p_reg;
    edev = ip->i_dev;

    /*  Compute the number of file system blocks in a page.
     *  This depends on the file system block size.
     */

    nppblk = NBPP / (FsBSIZE(edev)); /*pcs*/

    /*  Allocate invalid pages for the gap at the start of
     *  the region and demand-fill pages for the actual
     *  text.
     */
    
    gap = vaddr - prp->p_regva;
    if(growreg(&u, prp, btotp(gap), 0, DBD_NONE) < 0) {
        return(-1);
    }
    if (rp->r_type != RT_PRIVATE)
        more = 4;
    
    if(growreg(&u, prp, btop(count+gap+more) - btotp(gap), 0, DBD_DFILL) < 0)
        return(-1);
    
    gap = btotp(gap);               /*pcs*/
    rp->r_pgoff = gap; 
    size = rp->r_pgsz;
    rp->r_pgsz -= gap;              /*pcs*/
    availsmem += gap;
    rp->r_filesz = count + off;
    
    boff = FsBNO(edev, off & ~POFFMASK);    /* File offset in blocks. */
    i = ptots(gap);
    seglim = ptos(size);

    firsttime = 1;

    for(  ;  i < seglim  ;  i++){
        register int    lim;
        register pte_t  *pt;

        if(gap > stopg(i))
            j = gap - stopg(i);
        else
            j = 0;

        lim = size - stopg(i);
        if(lim > NPGPT)
            lim = NPGPT;

        pt = (pte_t *)rp->r_list[i] + j;
        dbd = (dbd_t *)pt + NPGPT;

        for(  ;  j < lim ;  j++, pt++, dbd++, boff += nppblk) { /*pcs*/

           /*   If these are private pages, then make
            *   them copy-on-write since they will
            *   be put in the hash table.
            */
            if(rp->r_type == RT_PRIVATE){
                pt->pgm.pg_prot = PTE_UR;
                pt->pgm.pg_cw = 1;
            }
            dbd->dbd_blkno = boff; /*pcs*/
            dbd->dbd_type = DBD_FILE;

            if (firsttime) {
                dbd->dbd_adjunct = DBD_FIRST;
                firsttime = 0;
            } else
                dbd->dbd_adjunct = 0;
        }
    }

    /*  Mark the last page for special handling
     */
    
    dbd[-1].dbd_adjunct |= DBD_LAST;

    rp->r_flags |= RG_DONE;
    return(0);

}

/*  Find the region corresponding to a virtual address.
 */

preg_t *findreg(p, vaddr)
register struct proc    *p;
register caddr_t    vaddr;
{
    register preg_t *prp;
    register preg_t *oprp;

    oprp = p->p_region;
    for(prp = &p->p_region[1] ; prp->p_reg ; prp++)
        if(vaddr >= prp->p_regva  &&
           (prp->p_regva > oprp->p_regva || vaddr < oprp->p_regva))
            oprp = prp;
    if (oprp->p_reg != 0 && vaddr >= oprp->p_regva) {
        if (vaddr <
            (oprp->p_type==RT_SHMEM ? (caddr_t)SYSVA :
                (caddr_t)(oprp->p_regva + ptob(oprp->p_reg->r_pgsz + oprp->p_reg->r_pgoff))))
            return oprp;
    }
    return NULL;
}

/*  Find the pregion of a particular type.
 */

preg_t *
findpreg(pp, type)
register struct proc *pp;
register int    type;
{
    register preg_t *prp;

    for(prp = pp->p_region ; prp->p_reg ; prp++){
        if(prp->p_type == type)
            return(prp);
    }

    /*  We stopped on an unused region.  If this is what
     *  was called for, then return it unless it is the
     *  last region for the process.  We leave the last
     *  region unused as a marker.
     */

    if((type == PT_UNUSED)  &&  (prp < &pp->p_region[pregpp - 1]))
        return(prp);
    return(NULL);
}

/*
 * Change protection of ptes for a region
 */
void
chgprot(prp, prot)
register preg_t *prp;
{
    register reg_t  *rp;
    register pte_t  *pt;
    register int    i;
    register int    j;
    register int    seglim;
    register int    pglim;

    if(prot == SEG_RO) {
        prp->p_flags |= PF_RDONLY;
        prot = PTE_UR;
    } else {
        prp->p_flags &= ~PF_RDONLY;
        prot = PTE_UW;
    }
    rp = prp->p_reg;

    /*  Look at all of the segments of the region.
     */
    
    i = ptots(rp->r_pgoff);
    seglim = ptos(rp->r_pgoff + rp->r_pgsz);

    for(  ;  i < seglim  ;  i++){

        /*  Look at all of the pages of the segment.
         */

        if(rp->r_pgoff > stopg(i))
            j = rp->r_pgoff - stopg(i);
        else
            j = 0;
        pt = rp->r_list[i] + j;
        pglim = rp->r_pgoff + rp->r_pgsz - stopg(i);
        if(pglim > NPGPT)
            pglim = NPGPT;
        
        ASSERT(j >= 0  &&  j <= NPGPT);
        ASSERT(pglim >= j  &&  pglim <= NPGPT);

        for(  ;  j < pglim  ;  j++, pt++){

            /*  set protection
             */

            pt->pgm.pg_prot = prot;
        }
    }
    clratb();
}

caddr_t listalloc(n) 
{ 
    register unsigned base;
    register caddr_t addr;

    while ( (base = malloc(listmap, n)) == 0) {
        addr = pgalloc(btoc(n), PG_V|PG_KW, 0);
        if (addr == 0)
            return 0;
        mfree(listmap, ctob(btoc(n)), ctotp((int)addr - SYSVA));
    }
    return (caddr_t)(((int)base * 4) + SYSVA);
} 

listfree(addr, sz)
caddr_t addr;
{ 
    register struct map *m;
    register int start, end, n;

    if (sz == 0)
        return;

    mfree(listmap, sz, ctotp((int)addr-SYSVA));
    for (m = listmap+1; m->m_size; m++) {
        
        /* This is a really weird one: should have used btoc and btotc macros,
         * but they cast to (long) first, generating the wrong asr vs. lsr
         * instructions. So to get the same result as precompiled binary,
         * synthesize these macros with (unsigned).
           The ctob macro is correct as it will always generate a logical lsl */
#define BTOC(x)  (((x)+(NBPC-1))>>BPCSHIFT)
#define BTOTC(x) ((x)>>BPCSHIFT)
        start = BTOC(m->m_addr);
        end = BTOTC(m->m_addr+m->m_size) - 1;
        if (start <= end) {
            malloc_at(listmap, n = ctob(start), ctob(end - start + 1));
            pgfree(ptosv(start), end - start + 1, 1);
        }
    }
}

extern int freemem;

escdeadlock(rp, sz)
reg_t *rp;
{
    sz = ptos(sz) + 1;
    memreserve(rp, sz);
    freemem += sz;
}