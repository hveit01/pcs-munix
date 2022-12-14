/* @(#)page.c   1.5 */
static char *_Version = "@(#) RELEASE:  1.1  Aug 12 1986 /usr/sys/os/page.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/inode.h"
#include "sys/var.h"
#include "sys/mount.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/buf.h"
#include "sys/map.h"
#include "sys/pfdat.h"
#include "sys/proc.h"
#include "sys/swap.h"
#include "sys/sysinfo.h"
#include "sys/debug.h"

int firstfree, maxfree, freemem;

extern int maxupts;

#if SPTLOGON == 1
#define SPTLGSZ 128
struct sptlog {
    int type;   /* 0=sptalloc; 1=sptfree */
    unsigned int begva,
        endva;
    unsigned int retva;
} sptlog[SPTLGSZ];
int sptlgndx = 0;
#endif

#if PFLOGON == 1
#define PFLGSZ 256
struct pfdlog {
    char type;  /* 3=memfree; 2=vfault; 1=allocate; 0=pfree */
    char pfuse;
    short pfnum;
    unsigned int process;
    unsigned int retva;
    struct region *regp;
} pfdlog[PFLGSZ];
int pflgndx = 0;
#endif

#if PHASHLOGON == 1
#define PHASHLGSZ 256
struct phashlog {
    int type:4, /* 3=pbremove; 2=premove; 1=pinsert; 0=pfind */
        pfnum:28;
    struct dbd dbd;
    short   dev;
    short   blkno;
    unsigned int process;
} phashlog[PHASHLGSZ];
int phashndx = 0;
#endif

/*
 * Allocate pages and fill in page table
 *  rp      -> region pages are being added to.
 *  pt      -> address of page table
 *  size        -> # of pages needed
 *  validate    -> Mark pages valid if set.
 * returns:
 *  0   Memory allocated as requested.
 *  1   Had to unlock region and go to sleep before
 *      memory was obtained.  After awakening, the
 *      first page was valid so no page was allocated.
 */

ptmemall(rp, pt, size, validate)
reg_t   *rp;
pte_t   *pt;
int validate;
{
    ASSERT(rp->r_flags & RG_LOCK);

    memreserve(rp, size);
    if (pt->pgi.pg_pte & PG_V) { /*pcs*/
        freemem += size;
        return(1);
    }
    rp->r_nvalid += size;
    ptfill(pt, size, rp->r_type, validate);
    return(0);
}


/*
 * Allocate pages and fill in page table for system virtual space
 *  pt      -> address of page table
 *  size        -> # of pages needed
 * returns:
 *  0   Memory allocated as requested.
 *  1   Failure
 */

sptmemall(pt, size)
pte_t   *pt;
int size;
{
    memreserve(NULL, size);
    if (pt->pgm.pg_v) {
        freemem += size;
        return(1);
    }
    ptfill(pt, size, RT_PRIVATE, 1);
    return(0);
}

/*
 *  Fill in ptes
 */
ptfill(pt, size, type, validate)
register pte_t  *pt;
register int    size;
register int    type;
register int    validate;
{
#if PFLOGON == 1
    int frame;
#endif
    register struct pfdat *pfd;
    register struct pfdat *ph;
    pte_t curpt; /*20*/

    ph = &phead;
    if (validate)
        validate = PG_V;    /*pcs*/
    for (; --size >= 0; pt++) {
        ASSERT((pt->pgi.pg_pte & (PG_V | PG_REF | PG_NDREF)) == 0);
        pfd = ph->pf_next;
        ASSERT(pfd != ph);
        ASSERT(pfd->pf_flags&P_QUEUE);
        ASSERT(pfd->pf_use == 0);

        /* Delink page from free queue and set up pfd
         */

        ph->pf_next = pfd->pf_next;
        pfd->pf_next->pf_prev = pfd->pf_prev;
        if (pfd->pf_flags&P_HASH)
            premove(pfd);
        pfd->pf_use = 1;
        pfd->pf_flags = 0;
        curpt = *pt; /*pcs*/

#       if PFLOGON == 1
            pfdlog[pflgndx].type = 1;
            pfdlog[pflgndx].process = (int)curproc;
            pfdlog[pflgndx].pfnum = pfd-pfdat;
            pfdlog[pflgndx].pfuse = pfd->pf_use;
            pfdlog[pflgndx].retva = (&frame)[5];
            pfdlog[pflgndx].regp = NULL;
            pflgndx++;
            if (pflgndx >= PFLGSZ) pflgndx = 0;
#       endif

        /*
         * Insert in page table
         */

        curpt.pgm.pg_pfn = pfd - pfdat;
/*      pt->pgi.pg_pte &= ~PG_M;*/

        setpl((&curpt), validate ? PL_REF : 0);     /*pcs*/

        if (curpt.pgm.pg_cw || type == RT_STEXT)    /*pcs*/
            curpt.pgm.pg_prot = PTE_UR;             /*pcs*/
        else
            curpt.pgm.pg_prot = PTE_UW;             /*pcs*/

        curpt.pgi.pg_pte |= validate;               /*pcs*/
        *pt = curpt;                                /*pcs*/
    }
    clrcache();                                     /*pcs*/
}

/*
 * Shred page table and update accounting for swapped
 * and resident pages
 *  rp  -> ptr to the region structure.
 *  pt  -> ptr to the first pte to free.
 *  dbd -> ptr to disk block descriptor.
 *  size    -> nbr of pages to free.
 */

pfree(rp, pt, dbd, size)
register reg_t  *rp;
register pte_t  *pt;
register dbd_t  *dbd;
register int    size;
{
#if PFLOGON == 1
    int frame;
#endif
    register struct pfdat *pfd;
    register struct pfdat *ph;

    ASSERT(!rp || rp->r_flags & RG_LOCK);

    ph = &phead;
    for (; --size >= 0; pt++) {
        if (pt->pgi.pg_pte & PG_V) {    /*pcs*/
            pfd = &pfdat[pt->pgm.pg_pfn];

#           if PFLOGON == 1
                pfdlog[pflgndx].type = 0;
                pfdlog[pflgndx].process = (int)curproc;
                pfdlog[pflgndx].pfnum = pfd-pfdat;
                pfdlog[pflgndx].pfuse = pfd->pf_use-1;
                pfdlog[pflgndx].retva = (&frame)[5];
                pfdlog[pflgndx].regp = rp;
                pflgndx++;
                if (pflgndx >= PFLGSZ) pflgndx = 0;
#           endif

            /*  Free pages that aren't being used by anyone else
             */
            ASSERT(pfd->pf_use > 0);

            if (--pfd->pf_use == 0) {

                /* Pages that are associated with disk go to
                 * end of queue in hopes that they will be
                 * reused.  All others go to head of queue so
                 * they will be reused quickly.
                 */

                if (dbd == NULL || dbd->dbd_type == DBD_NONE) {
                    /*
                     * put at head 
                     */
                    pfd->pf_next = ph->pf_next;
                    pfd->pf_prev = ph;
                    ph->pf_next = pfd;
                    pfd->pf_next->pf_prev = pfd;
                } else {
                    /*
                     * put at tail 
                     */
                    pfd->pf_prev = ph->pf_prev;
                    pfd->pf_next = ph;
                    ph->pf_prev = pfd;
                    pfd->pf_prev->pf_next = pfd;
                }
                pfd->pf_flags |= P_QUEUE;
                freemem++;
            }

            if (rp)
                rp->r_nvalid--;
        }
        if (dbd  &&  dbd->dbd_type == DBD_SWAP)
            if (swfree1(dbd) == 0)
                pbremove(rp, dbd);

        /*
         * Change to zero pte's.
         */

        pt->pgi.pg_pte = 0;
        setpl(pt, 0);       /*pcs*/
        
        if(dbd)
            dbd++->dbd_type = DBD_NONE;
    }
}

/*
 * Find page by looking on hash chain
 *  dbd -> Ptr to disk block descriptor being sought.
 * returns:
 *  0   -> can't find it
 *  pfd -> ptr to pfdat entry
 */

struct pfdat *
pfind(rp, dbd)
register struct region *rp;
register dbd_t  *dbd;
{
    register dev_t      dev;
    register daddr_t    blkno;
    register pfd_t      *pfd;

    /*  Hash on block and look for match.
     */

    if (dbd->dbd_type == DBD_SWAP) {
        dev = swaptab[dbd->dbd_swpi].st_dev;
        blkno = dbd->dbd_blkno;
    } else {
        register struct inode   *ip;

        ip = rp->r_iptr;
        ASSERT(ip != NULL);
        ASSERT(ip->i_map != NULL);
        dev = ip->i_dev;

        /*
         *  hash the page on the physical block number
         *  which matches
         */
        blkno = ip->i_map[dbd->dbd_blkno]; /*pcs*/
    }

    pfd = phash[blkno&phashmask].pf_hchain;

    for( ; pfd != NULL ; pfd = pfd->pf_hchain) {
        if((pfd->pf_blkno == blkno) && (pfd->pf_dev == dev)) {

#           if PHASHLOGON == 1
                phashlog[phashndx].type = 0;
                phashlog[phashndx].pfnum = pfd-pfdat;
                phashlog[phashndx].dbd = *dbd;
                phashlog[phashndx].dev = dev;
                phashlog[phashndx].blkno = blkno;
                phashlog[phashndx].process = (int)curproc;
                phashndx++;
                if (phashndx >= PHASHLGSZ) phashndx = 0;
#           endif

            if (pfd->pf_flags & P_BAD)
                continue;
            return(pfd);
        }
    }
    return(0);
}

/*
 * Insert page on hash chain
 *  dbd -> ptr to disk block descriptor.
 *  pfd -> ptr to pfdat entry.
 * returns:
 *  none
 */
pinsert(rp, dbd, pfd)
register struct region *rp;
register dbd_t  *dbd;
register pfd_t  *pfd;
{
    register dev_t      dev;
    register daddr_t    blkno;
    register struct inode   *ip; /*pcs*/

    if (dbd->dbd_type == DBD_SWAP) {
        ip = NULL;
        dev = swaptab[dbd->dbd_swpi].st_dev;
        blkno = dbd->dbd_blkno;
    } else {
        ip = rp->r_iptr;
        ASSERT(ip != NULL);
        ASSERT(ip->i_map != NULL);
        dev = ip->i_dev;

        /*
         *  hash the page on the physical block number
         *  which matches
         */
        blkno = ip->i_map[dbd->dbd_blkno]; /*pcs*/
    }

    /*
     * insert newcomers at tail of bucket
     */
    {
        register struct pfdat *p, *pfd1; /*pcs*/

        for(p = &phash[blkno&phashmask] ; pfd1 = p->pf_hchain ;
            p = pfd1) {
            if((pfd1->pf_blkno == blkno) &&
               (pfd1->pf_dev == dev)){
                panic("pinsert dup");
            }
        }
        p->pf_hchain = pfd;
        pfd->pf_hchain = pfd1;
    }

#   if PHASHLOGON == 1
        phashlog[phashndx].type = 1;
        phashlog[phashndx].pfnum = pfd-pfdat;
        phashlog[phashndx].dbd = *dbd;
        phashlog[phashndx].dev = dev;
        phashlog[phashndx].blkno = blkno;
        phashlog[phashndx].process = (int)curproc;
        phashndx++;
        if (phashndx >= PHASHLGSZ) phashndx = 0;
#   endif


    /*  Set up the pfdat.
     */
    pfd->pf_dev = dev;
    pfd->pf_swpi = dbd->dbd_swpi;
    pfd->pf_ino = ip;
    pfd->pf_blkno = blkno;
    pfd->pf_flags |= P_HASH;
}


/*
 * remove page from hash chain
 *  pfd -> page frame pointer
 * returns:
 *  0   Entry not found.
 *  1   Entry found and removed.
 */
premove(pfd)
register struct pfdat *pfd;
{
    register struct pfdat *pfd1, *p;
    int rval;

    rval = 0;
    for (p = &phash[pfd->pf_blkno&phashmask]; pfd1 = p->pf_hchain; p = pfd1) {
        if (pfd1 == pfd) {
            p->pf_hchain = pfd->pf_hchain;
            rval = 1;
            break;
        }
    }

#   if PHASHLOGON == 1
        phashlog[phashndx].type = 2;
        phashlog[phashndx].pfnum = pfd-pfdat;
        phashlog[phashndx].dbd.dbd_blkno = rval;  /* no dbd available */
        phashlog[phashndx].dev = pfd->pf_dev;
        phashlog[phashndx].blkno = pfd->pf_blkno;
        phashlog[phashndx].process = (int)curproc;
        phashndx++;
        if (phashndx >= PHASHLGSZ) phashndx = 0;
#   endif

    /*
     * Disassociate page from disk and
     * remove from hash table
     */
    pfd->pf_blkno = BLKNULL;
    pfd->pf_hchain = NULL;
    pfd->pf_flags &= ~P_HASH;
    pfd->pf_dev = 0;
    pfd->pf_ino = NULL; /*pcs*/
    return(rval);
}

/*
 * Allocate system virtual address space and
 * allocate pages (usually for user page table).
 */
caddr_t pgalloc(size, mode, base) /*pcs*/
register int size, mode, base;
{
#if SPTLOGON == 1
    int frame;
#endif
    register i, sp;

    /*
     * Allocate system virtual address space
     */
    if ((sp = malloc(pgmap, size))  ==  0) {    /*pcs*/
#ifdef OSDEBUG
        printf ("No kernel virtual space\n");
        printf ("size %d %d %d\n",size, mode, base);
#endif
        return(NULL);
    }
    /*
     * Allocate and fill in pages
     */
    if (base  ==  0) {
        sptmemall(&sbrpte[sp], size);
        clearpage(((pte_t *) sbrpte)[sp].pgm.pg_pfn);
    }

    /*
     * Setup page table entries
     */
    for (i = 0; i < size; i++) {
/*      ( (pte_t *)sbrpte)[sp+i].pgm.pg_prot = 0;*/
        ( (pte_t *)sbrpte)[sp+i].pgi.pg_pte &= ~PG_PROT;    /*pcs*/

        if (base > 0)
            ( (int *)sbrpte)[sp + i] = mkpte(mode, base++);
        else
            ( (int *)sbrpte)[sp + i] |= mode;
/*      invsatb(ptosv(sp + i));*/
    }
    clrcache(); /*pcs*/

#   if SPTLOGON == 1
        sptlog[sptlgndx].type = 0;
        sptlog[sptlgndx].begva = ptosv(sp);
        sptlog[sptlgndx].endva = ptosv(sp+size) -1;
        sptlog[sptlgndx].retva = (&frame)[5];
        sptlgndx++;
        if (sptlgndx >= SPTLGSZ) sptlgndx = 0;
#   endif

    return (caddr_t)(ptosv(sp)); /*pcs*/
}

pgfree(vaddr, size, flag) /*pcs*/
register int size;
{
#if SPTLOGON == 1
    int frame;
#endif
    register i, sp;

    sp = svtop(vaddr);

#   if SPTLOGON == 1
        sptlog[sptlgndx].type = 1;
        sptlog[sptlgndx].begva = vaddr;
        sptlog[sptlgndx].endva = ptosv(sp+size) -1;
        sptlog[sptlgndx].retva = (&frame)[5];
        sptlgndx++;
        if (sptlgndx >= SPTLGSZ) sptlgndx = 0;
#   endif

    if (flag)
        pfree(NULL, &sbrpte[sp], NULL, size);
    for (i = 0; i < size; i++) {
        ( (int *) sbrpte)[sp + i] = 0;
    }
    clrcache();

    mfree(pgmap, size, sp);
}


/*pcs*/
caddr_t sptalloc(size)
{
    register caddr_t addr;
    register int sp;
    
    while ((sp=malloc(sptmap, size)) == 0) {
        addr = pgalloc( ctop(size), PG_V|PG_KW, 0);
        if (addr == 0)
            return 0;
        mfree(sptmap, ctop(size)<<2, svtoc(addr));
    }
    
    addr = (caddr_t)ctosv(sp);
    bzero(addr, ctob(size));
    return addr;
} 

/*pcs*/
sptfree(addr, size, flag)
caddr_t addr;
{
    mfree(sptmap, size, svtoc(addr));
    if (flag)
        spttidy();
}

/*pcs*/
spttidy()
{
    register struct map *mpp;
    register int s, e;  /* start and end of map */

    for (mpp = &sptmap[1]; mpp->m_size != 0; mpp++) {
        s = ctop(mpp->m_addr);
        e = ctotp(mpp->m_addr + mpp->m_size) - 1;
        if (s <= e) {
            malloc_at(sptmap, ptoc(s), ptoc(e - s + 1));
            pgfree(ptosv(s), e - s + 1, 1);
        }
    }
} 

/*
 * Initialize memory map
 *  first   -> first free page #
 *  last    -> last free page #
 * returns:
 *  none
 */
meminit(first, last)
register int first;
{
    register struct pfdat *pfd;
    register int i;

    firstfree = first;
    maxfree = last;
    freemem = (last - first);
    maxmem = freemem;
    maxumem = MAXUMEM;
    maxupts = maxumem / NPGPT;
    /*
     * Setup queue of pfdat structures.
     * One for each page of available memory.
     */
    pfd = &pfdat[first];
    phead.pf_next = &phead;
    phead.pf_prev = &phead;
    /*
     * Add pages to queue, high memory at end of queue
     * Pages added to queue FIFO
     */
    for (i = freemem; --i >= 0; pfd++) {
        pfd->pf_next = &phead;
        pfd->pf_prev = phead.pf_prev;
        phead.pf_prev->pf_next = pfd;
        phead.pf_prev = pfd;
        pfd->pf_flags = P_QUEUE;
        pfd->pf_use = 0;
    }
}

/*
 * Flush pages associated with an inode
 *  ip  -> pointer to inode table
 * returns:
 *  none
 */
ipflush(ip)
register struct inode *ip;
{
    register int    i;
    register pfd_t  *pfd;
    register dev_t  edev;

    /*  Remove pages which are on the same device as
     *  the requested inode.
     */

    edev = ip->i_dev;

    for (i = firstfree,pfd = pfdat + i; i < maxfree; i++, pfd++)
        if((pfd->pf_dev == edev)  &&  (pfd->pf_flags & P_HASH)){
                premove(pfd);
        }
}

inoflush(ip)
register struct inode *ip;
{
    register int i;
    register pfd_t  *pfd;
    
    for (i = firstfree,pfd = pfdat + i; i < maxfree; i++, pfd++)
        if (pfd->pf_ino == ip && (pfd->pf_flags & P_HASH))
            premove(pfd);
} 

/*
 * flush all pages associated with a mount device
 *  mp  -> mount table entry
 * returns:
 *  none
 */
punmount(mp)
register struct mount *mp;
{
    register int i;
    register struct pfdat *pfd;
    register struct inode *ip;

    bflush(mp->m_dev);
/*  for (ip = &inode[0]; ip < (struct inode *)v.ve_inode; ip++) {
        if (ip->i_dev == mp->m_dev)
            ipflush(ip);
    }*/
    for (i = firstfree,pfd = pfdat + i; i < maxfree; i++, pfd++) {
        if (mp->m_dev == pfd->pf_dev)
            if (pfd->pf_flags & P_HASH)
                premove(pfd);
    }
}

/*
 * Find page by looking on hash chain
 *  dbd Ptr to disk block descriptor for block to remove.
 * returns:
 *  0   -> can't find it
 *  i   -> page frame # (index into pfdat)
 */
pbremove(rp, dbd)
register struct region *rp;
register dbd_t  *dbd;
{
    register struct pfdat   *pfd;
    register struct pfdat   *p;
    register daddr_t    blkno;
    register dev_t      dev;

    /*
     * Hash on block and look for match
     */

    if (dbd->dbd_type == DBD_SWAP) {
        dev = swaptab[dbd->dbd_swpi].st_dev;
        blkno = dbd->dbd_blkno;
    } else {
        register struct inode   *ip;

        ip = rp->r_iptr;
        ASSERT(ip != NULL);
        ASSERT(ip->i_map != NULL);
        dev = ip->i_dev;

        /*
         *  hash the page on the physical block number
         *  which matches
         */
        blkno = ip->i_map[dbd->dbd_blkno];
/*      blkno = FsLTOP(dev, dbd->dbd_blkno);
        if ((dbd->dbd_adjunct & DBD_FP) == 0)
            blkno = blkno + 1;*/
    }

    for (p = &phash[blkno&phashmask]; pfd = p->pf_hchain; p = pfd) {
        if ((pfd->pf_blkno == blkno) && (pfd->pf_dev== dev)) {

#           if PHASHLOGON == 1
                phashlog[phashndx].type = 3;
                phashlog[phashndx].pfnum = pfd-pfdat;
                phashlog[phashndx].dbd = *dbd;
                phashlog[phashndx].dev = dev;
                phashlog[phashndx].blkno = blkno;
                phashlog[phashndx].process = (int)curproc;
                phashndx++;
                if (phashndx >= PHASHLGSZ) phashndx = 0;
#           endif

            p->pf_hchain = pfd->pf_hchain;

            pfd->pf_blkno = BLKNULL;
            pfd->pf_hchain = NULL;
            pfd->pf_flags &= ~P_HASH;
            pfd->pf_dev = 0;
            pfd->pf_ino = NULL; /*pcs*/
            return(1);
        }
    }

    return(0);

}

/*
 * Reserve size memory pages.  Returns with freemem
 * decremented by size.  Return values:
 *  0 - Memory available immediately
 *  1 - Had to sleep to get memory
 */
memreserve(rp, size)
register reg_t *rp;
{
    register struct proc *p;
    extern char runout;

    ASSERT(!rp || rp->r_flags & RG_LOCK);

    if (freemem >= size) {
        freemem -= size;
        return(0);
    }
    if (rp != NULL)
        regrele(rp);
    p = u.u_procp;
    while (freemem < size) {
        if (runout) {
            runout = 0;
            wakeup((caddr_t)&runout);
        }
        p->p_stat = SXBRK;
        sxbrkcnt++; /*pcs*/
        swtch();
    }
    freemem -= size;
    if (rp != NULL)
        reglock(rp);
    return(1);
}
