static char* _Version = "@(#) RELEASE:  1.2  Jul 01 1987 /usr/sys/os/fault.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/sysinfo.h"
#include "sys/systm.h"
#include "sys/mount.h"
#include "sys/filsys.h"
#include "sys/fblk.h"
#include "sys/page.h"
#include "sys/pfdat.h"
#include "sys/proc.h"
#include "sys/swap.h"
#include "sys/buf.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/ino.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/region.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/tuneable.h"
#include "sys/var.h"

extern int freemem;

/*pcs: Vax does this in assembler */
pte_t* mapa(arg0)
register daddr_t arg0;
{
    register preg_t* p;
    register blk;

    if (arg0 >= (uint)SYSVA)
        return 0;

    if ((_ptbr[snum(arg0)] & MP_VALID) != MP_VALID)
        return 0;

    if ((p = findreg(u.u_procp, arg0)) == 0)
        return 0;
    
    blk = (ulong)((caddr_t)arg0 - p->p_regva);
    blk = btotp(blk);
    return &(p->p_reg->r_list[blk / NPPS][blk % NPPS]);
}
/*pcs*/

/*
 * Protection fault handler
 *  vaddr   -> virtual address of fault
 */
pfault(vaddr)
{
    register pte_t* pt;
    register pfd_t* pfd;
    register dbd_t* dbd;
    
    reg_t* rp;
    preg_t* prp;
    pte_t pttmp;
    int tmp;
    int oldpri;
    int oldspl; /*unused, relic from VAX impl */

    oldpri = curpri;    /* Save priority for later restoral */

    /*  Get a pointer to the region which the faulting
     *  virtual address is in.
     */

    if ((prp = findreg(u.u_procp, vaddr)) == 0)
        return 0;
    rp = prp->p_reg;
    reglock(rp);
    
    tmp = btotp(vaddr - (int)prp->p_regva);
    pt = (pte_t*)&(rp->r_list[tmp / NPGPT][tmp % NPGPT]);
    if (pt->pgm.pg_cw == 0) {
        regrele(rp);
        return 0;
    }
    
    /*  VAX faults protection then translation.
     *  We want it the other way around
     */
    
    if (pt->pgm.pg_v == 0) {
        regrele(rp);
        if ((tmp=vfault(vaddr)) <= 0)
            return tmp;
        reglock(rp);
    }
    
    minfo.pfault++;

    /*  Find dbd as mapped by region
     */
    
    tmp = btotp(vaddr - (int)prp->p_regva);
    dbd = (dbd_t*)&(rp->r_list[tmp / NPGPT][tmp % NPGPT]) + NPGPT;
    pfd = pfdat + pt->pgm.pg_pfn;

    /*  Copy on write
     *  If use is > 1, or page is associated with a file, copy it
     */ 
    if (pfd->pf_use > 1 || dbd->dbd_type == DBD_FILE) {
        minfo.cw++;
        pttmp.pgi.pg_pte = 0;
        tmp = pt->pgm.pg_lock;
        pt->pgm.pg_lock = 1;
        ptmemall(rp, &pttmp, 1, 1);
        pt->pgm.pg_lock = tmp;
        copypage(pt->pgm.pg_pfn, pttmp.pgm.pg_pfn);
        pfree(rp, pt, dbd, 1);
        *pt = pttmp;        
    } else {
        /*  Break the disk association of the page
         */
        minfo.steal++;
        if (pfd->pf_flags & P_HASH)
            premove(pfd);
        if (dbd->dbd_type == DBD_SWAP)
            swfree1(dbd);
        dbd->dbd_type = DBD_NONE;
    }

    /*  Make page table entry writeable and
     *  clear translation buffer
     */
    
    pt->pgm.pg_cw = 0;
    if (rp->r_type != RT_STEXT)
        pt->pgm.pg_prot = PTE_UW;
    
    pt->pgi.pg_pte |= PG_V;
    setpl(pt, PL_REF|PL_MOD);
    invsatb(vaddr);
    regrele(rp);
    curpri = oldpri;
    u.u_procp->p_pri = oldpri;
    return 1;
}

/*
 * Translation fault handler
 *  vaddr   -> virtual address
 */
vfault(vaddr)
{
    register pte_t* pt;
    register reg_t* rp;
    register dbd_t* dbd;
    
    preg_t* prp;
    int tmp;
    int oldpri;
    int oldspl; /* unused, relic from Vax */
    
    oldpri = curpri;    /* Save CPU priority for later restoral */

    /*  Lock the region containing the page that faulted.
     */

    if ((prp = findreg(u.u_procp, vaddr)) == 0)
        return 0;
    rp = prp->p_reg;
    reglock(rp);

    /*  Find pte and dbd as mapped by region
     */

    tmp = btotp(vaddr - (int)prp->p_regva);
    pt = &rp->r_list[tmp / NPGPT][tmp % NPGPT];
    dbd = (dbd_t*)pt + NPGPT;
    
    if (pt->pgi.pg_pte == 0) {
        regrele(rp);
        return 0;
    }
    
    minfo.vfault++;

    /*  Check that the page has not been read in by
     *  another process while we were waiting for
     *  it on the reglock above.
     */
    
    if (pt->pgm.pg_v) {
        regrele(rp);
        curpri = u.u_procp->p_pri = oldpri;
        return 1;
    }

    /*  See what state the page is in.
     */
    
    switch(dbd->dbd_type) {
    case DBD_DFILL:
    case DBD_DZERO:

        /*  Demand zero or demand fill page.
         */

        minfo.demand++;
        
        if (ptmemall(rp, pt, 1, 0)) {
            regrele(rp);
            curpri = u.u_procp->p_pri = oldpri;
            return 1;
        }
        if (dbd->dbd_type == DBD_DZERO)
            clearpage(pt->pgm.pg_pfn);
        dbd->dbd_type = DBD_NONE;
        pt->pgi.pg_pte |= PG_V;
        orpl(pt, PL_REF);
        break;

    case DBD_SWAP:
    case DBD_FILE: {
        /*register*/ pte_t* ptep;
        /*register*/ dbd_t* dbdp;
        register i;
        register count;
        struct pfdat* pfd;
        pte_t* endreg;
        int type;
        int swpi;
        int maxprepage;
        int curblk;
/*      int nextblk;*/

        /*  If the page we want is in memory already, take it
         */

        ;
        if (pageincache(rp, pt, dbd)) {
            minfo.cache++;
            break;
        }

        /*  Otherwise, get page(s) from disk. First reserve memory
         *  for the ptfill below.  We must do this now to prevent
         *  pinsert dups.
         */

        if (memreserve(rp, tune.t_prep + 1)) {
            /*  We went to sleep waiting for memory.
             *  Check if the page we're after got loaded in
             *  the mean time.  If so, give back the memory
             *  and return
             */
            if (pt->pgi.pg_pte & PG_V) {
                freemem += tune.t_prep + 1;
                break;
            }
            if (pageincache(rp, pt, dbd)) {
                minfo.cache++;
                freemem += tune.t_prep + 1;
                break;
            }
        }
        
        /*  Scan ptes and dbds looking for a run of
         *  contiguous pages to load from disk
         */
        
        tmp = btots(vaddr - (int)prp->p_regva);
        if (stopg(tmp+1) < rp->r_pgoff + rp->r_pgsz)
            endreg = &rp->r_list[tmp][NPGPT];
        else {
            i = rp->r_pgoff + rp->r_pgsz - stopg(tmp);
            endreg = &rp->r_list[tmp][i];
        }
        
        type = dbd->dbd_type;
        swpi = dbd->dbd_swpi;
        maxprepage = tune.t_prep;

        count = 1;
        ptep = pt + 1;
        dbdp = dbd + 1;
        curblk = dbd->dbd_blkno;
        
        while (ptep < endreg &&
            (ptep->pgi.pg_pte & PG_V)==0 &&
            count < maxprepage &&
            dbdp->dbd_type == type &&
            pfind(rp, dbdp) == 0) {

            if (type == DBD_SWAP) {
                if (dbdp->dbd_swpi != swpi ||
                    dbdp->dbd_blkno != curblk+8) /*pcs*/
                    break;
            }

            count++;
            ptep++;
            dbdp++;
            curblk += 8; /*pcs*/
        }
        
        /*  Give back excess memory we're holding and fill
         *  in page tables with real pages.
         */

        freemem += (tune.t_prep + 1 - count);
        rp->r_nvalid += count;
        ptfill(pt, count, rp->r_type, 0);
        
        /*
         *  We now have a series of ptes with page frames
         *  assigned, whose rightful contents are not in the
         *  page cache. Pinsert the page frames, and read from
         *  disk.
         */

        vaddr &= ~(NBPP-1);
        
        if (type == DBD_SWAP) {
            int swapdel;
            
            minfo.swap++;
            
            swapdel = swaptab[swpi].st_flags & ST_INDEL;
            if (swapdel == 0) {
                ptep = pt;
                dbdp = dbd;
                for (i=0; i < count; i++) {
                    pinsert(rp, dbdp,
                        &pfdat[ptep->pgm.pg_pfn]);
                    ptep++;
                    dbdp++;
                }
            }

            swapchunkin(pt, count);

            ptep = pt;
            dbdp = dbd;
            for (i=0; i < count; i++) {
                if (swapdel != 0) {
                    swfree1(dbdp);
                    dbdp->dbd_type = DBD_NONE;
                }
                
                /*  Mark the I/O done, and awaken anyone
                 *  waiting for pfdats
                 */

                pfd = &pfdat[ptep->pgm.pg_pfn];
                pfd->pf_flags |= P_DONE;
                if (pfd->pf_flags & P_WANT) {
                    pfd->pf_flags &= ~P_WANT;
                    wakeup(pfd);
                }
                
                ptep->pgi.pg_pte |= PG_V;
                setpl(ptep, PL_REF);

                ptep++;
                dbdp++;
            }
        } else { /* DBD_FILE */
            dev_t edev;
            struct dbd *tmpdbd, *enddbd;    /*pcs: tmpdbd is unused*/
            struct buf* bp = NULL;
            short saveprot;
            int nblks;                      /*pcs: blocks required for file*/
            long* blkmap;                   /*pcs: i_map of blocks for file*/
            int va;                         /*pcs: virtual addr count to map file to */
            int endblk;                     /*pcs: final block to read */
            pte_t savepte;                  /*pcs: tmp save for current pte*/
            register blk;                   /*pcs: counter for blocks */
            
            /*
             *  We're reading from a file.
             *  Pinsert the page frames, except for the
             *  last frame of the mapped region. This
             *  frame will eventually be partially cleared
             *  so it will not match the disk, and should
             *  not be cleared
             */
            
            minfo.file++;
            
            edev = rp->r_iptr->i_dev;
            ptep = pt;
            dbdp = dbd;
            
            for (i=0; i < count; i++) {
                if (dbdp->dbd_adjunct & DBD_LAST) break;
                else {
                    pinsert(rp, dbdp, &pfdat[ptep->pgm.pg_pfn]);
                    ptep++;
                    dbdp++;
                }
            }
            
            /* Read the pages from the file */
            
            enddbd = (struct dbd*)endreg + NPGPT;
            ptep = pt;
            dbdp = dbd;
            
            nblks = (rp->r_iptr->i_size + FsBSIZE(edev) - 1) / FsBSIZE(edev);
            while (count > 0) {
                blkmap = rp->r_iptr->i_map;
                va = vaddr;
                savepte = *ptep;
                
                ptep->pgi.pg_pte |= PG_V;
                orpl(ptep, PL_REF);
                
                /* we have to set the protections
                 * for bcopy and bzero
                 */
                saveprot = ptep->pgm.pg_prot;
                ptep->pgm.pg_prot = PTE_KW;
                invsatb(vaddr);
                
                curblk = dbdp->dbd_blkno;
                endblk = (NBPP / FsBSIZE(edev)) + curblk - 1;
                if (endblk >= nblks)
                    endblk = nblks - 1;
                
                for (blk = curblk; blk <= endblk; blk++) {
                    if (blk == endblk && (count == 1 ||
                        (blk+1) >= nblks))
                        bp = bread(edev, blkmap[blk]);
                    else
                        bp = breada(edev, blkmap[blk], blkmap[blk+1]);

                    if (bp->b_flags & B_ERROR) {
                        *ptep = savepte;
                        prdev("page read error", edev);
                        brelse(bp);
                        while (count > 0) {
                            killpage(rp, ptep);
                            ptep++;
                            count--;
                        }
                        regrele(rp);
                        curpri = u.u_procp->p_pri = oldpri;
                        return -1;
                    }
                    
                    bcopy(bp->b_un.b_addr, va, FsBSIZE(edev));
                    brelse(bp);
                    va += FsBSIZE(edev);
                }
                if (dbdp->dbd_adjunct & DBD_LAST) {
                    i = poff(rp->r_filesz);
                    if (i > 0)
                        bzero(vaddr+i, NBPP-i);
                }
                
                ptep->pgm.pg_prot = saveprot;
                andpl(ptep, ~PL_MOD);
                invsatb(vaddr);

                /*  wakeup anyone waiting for the
                 *  page frame
                 */             
                pfd = &pfdat[ptep->pgm.pg_pfn];
                pfd->pf_flags |= P_DONE;
                if (pfd->pf_flags & P_WANT) {
                    pfd->pf_flags &= ~P_WANT;
                    wakeup(pfd);
                }
                ptep++;
                dbdp++;
                count--;
                vaddr += NBPP;
            }
        }
        
        break;
    }
    case DBD_NONE:
        regrele(rp);
        curpri = u.u_procp->p_pri = oldpri;
        return 0;
    default:
        panic("vfault - bad dbd_type");
    }
    regrele(rp);
    curpri = u.u_procp->p_pri = oldpri;
    return 1;
}

/*
 * Check if the page described by dbd is in memory.
 * If it is, latch onto it.  Return values:
 *  0 - page no where in sight.
 *  1 - got page
 */
pageincache(rp, pt, dbd)
register reg_t *rp;
register pte_t *pt;
register dbd_t *dbd;
{
    register pfd_t *pfd;
    
    /*  Look in page cache
     */
    if (pfd = pfind(rp, dbd)) {

        /*  We found it.  If the page is on the freelist,
         *  remove it.  If freemem is zero, someone already
         *  has reserved the page, and we cannot use it.
         */
        if (pfd->pf_flags & P_QUEUE) {
            if (freemem <= 0) {
                premove(pfd);
                return 0;
            }
/*loc_BC6:*/
            freemem--;
            pfd->pf_use = 1;
            pfd->pf_flags &= ~P_QUEUE;
            pfd->pf_prev->pf_next = pfd->pf_next;
            pfd->pf_next->pf_prev = pfd->pf_prev;
            pfd->pf_next = 0;
            pfd->pf_prev = 0;
        } else {
/*loc_C06:*/

            /*  Wait for the i/o to complete.  If bad is set,
             *  after waking, return failure.
             */
            
            pfd->pf_use++;
            while ((pfd->pf_flags & P_DONE) == 0) {
                pfd->pf_flags |= P_WANT;
                sleep(pfd, PMEM); /*0*/
/*loc_C30:*/
            }
            if (pfd->pf_flags & P_BAD) {
                pfd->pf_use--;
                return 0;
            }
        }
/*loc_C58:*/
        rp->r_nvalid++;
        pt->pgm.pg_pfn = pfd - pfdat;
        if (rp->r_type == RT_STEXT)
            pt->pgm.pg_prot = PTE_UR;
/*loc_C80: */

        pt->pgi.pg_pte |= PG_V;
        setpl(pt, PL_REF);
        return 1;
    }
/*loc_C9A:*/
    return 0;
}

/*
 * Clean up after a read error during vfault processing.
 * This code frees the previously allocated page, and marks
 * the pfdat as bad.  It leaves the pte, and dbd in their original
 * state.  It assumes the pte is presently invalid.
 */
killpage(rp, pt)
reg_t *rp;
register pte_t *pt;
{
    register pte_t save_pte;
    register pfd_t *pfd;
    
    save_pte = *pt;
    pt->pgm.pg_v = 1;
    pfd = &pfdat[pt->pgm.pg_pfn];
    pfd->pf_flags |= P_BAD|P_DONE;
    if (pfd->pf_flags & P_WANT) {
        pfd->pf_flags &= ~P_WANT;
        wakeup(pfd);
    }
/*loc_D04:*/
    pfree(rp, pt, 0, 1);
    *pt = save_pte;
}
