/* @(#)probe.c  1.1 */
static char* _Version = "@(#) RELEASE:  1.1  Aug 12 1986 /usr/sys/os/probe.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/map.h>    /*pcs*/
#include <sys/systm.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/buf.h>
#include <sys/var.h>
#include <sys/ipc.h>
#include <sys/errno.h>

/* returns an index into _dmapt, or 0 if invalid */
int useracc(vaddr, length, code)
register caddr_t vaddr;
register int length;
{
    register int i, ptlen;
    register pte_t *pt;
    register pte_t *pt1;
    int base; /*pcs*/
    int olen;
    int seg, seg2; /*pcs*/

    /*
     * off end of page table
     */
    if (vaddr > (caddr_t)SYSVA)
        return 0;

    seg2 = -1;
    seg = snum(vaddr);

    length = btotp(length - 1 + (int)vaddr) - btotp((int)vaddr) + 1;
    olen = length;
    while (--length >= 0) {
        if (seg2 != seg && (pt = mapa(vaddr)) == 0)
            return 0;
        seg2 = seg;
        if ((pt->pgm.pg_prot & 4) == 0) /* kernel mem, not user bit */
            return 0;

        if ((code&B_READ) && (pt->pgm.pg_prot & PTE_KW) == 0) {
            if (pt->pgm.pg_cw) {
                /*
                 * if physio, then break CW;
                 * adaptor cannot do it
                 */
                if (code&B_PHYS) {
                    i = fubyte(vaddr);
                    subyte(vaddr, i);
                }
            } else
                return 0;
        }
        pt++;
        vaddr += NBPP;
        seg = snum(vaddr);
    }

    /*
     * lock pages for physio
     */
    if (code&B_PHYS) {
        length = olen;
        vaddr -= length * NBPP;
        seg2 = -1;
        seg = snum(vaddr);
        if ((base = malloc(dmamap, length)) == 0) {
            printf("dma map overflow\n");
            return 0;
        }
        pt1 = (pte_t*)&_dmapt[base];
        while (--length >= 0) {
            if (seg2 != seg) 
                pt = mapa(vaddr);
            seg2 = seg;
            while (!pt->pgm.pg_v)
                i = fubyte(vaddr);
            pt->pgm.pg_lock = 1;
            *pt1++ = *pt++;
            vaddr += NBPP;
            seg = snum(vaddr);
        }
    }
    return base;
}

/* release an index into _dmapt */
int userrel(vaddr, length, code, base)
register caddr_t vaddr;
register int length;
unsigned int base;
{
    register pte_t *pt;
    register pte_t *pt1;
    
    int seg, seg2;
    int olen;

    seg2 = -1;
    seg = snum(vaddr);
    length = ((length - 1 + (int)vaddr) >> BPPSHIFT) - (((int)vaddr) >> BPPSHIFT) + 1;
    olen = length;
    
    pt1 = (pte_t*)&_dmapt[base];
    while (--length >= 0) {
        if (seg2 != seg)
            pt = mapa(vaddr);
        seg2 = seg;
        pt->pgm.pg_lock = 0;
        if (code&B_READ)
            setpl(pt, PL_REF|PL_MOD);
        else
            orpl(pt, PL_REF);
        pt++;
        pt1++->pgi.pg_pte = 0;
        vaddr += NBPP;
        seg = snum(vaddr);
    }
    mfree(dmamap, olen, base);
}

uaccess(p, prp)
register struct proc *p;
register preg_t *prp;
{ 
    register pte_t *pt;
    register int i;
    extern spte_t *userpte;

    pt = ublkptaddr(p, prp);
    for (i = 0; i < 1; i++) {
/* loc_2D0: */
        userpte[i] = *pt++;
    }
/* loc_2E0: */
    clrcache();
} 

pte_t* ublkptaddr(p, prp)
struct proc *p;
preg_t *prp;
{
    register reg_t *rp;
    register pte_t *ppt;
    register int seg;

    if (prp == 0)
        prp = findpreg(p, RT_SHMEM);
        
/* loc_31C: */
    rp = prp->p_reg;
    seg = ptos(rp->r_pgoff + rp->r_pgsz) - 1;
    
    ppt = &rp->r_list[seg][255];
    return ppt;
} 

unsigned vtop(seg)
register pte_t *seg;
{
    register pte_t *pt;
    pt = (pte_t*)svtopte(seg);
    return (((uint)pt->pgm.pg_pfn) << 2) | (MP_VALID | (btotc(seg) & 3));
}