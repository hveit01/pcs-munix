/* @(#)grow.c   1.3 */
static char* _Version = "@(#) RELEASE:  1.1  Oct 09 1986 /usr/sys/os/grow.c ";

#include <sys/types.h>
#include <sys/tuneable.h>
#include <sys/bitmasks.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/var.h>
#include <sys/page.h>
#include <sys/pfdat.h>
#include <sys/seg.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/debug.h>

pte_t *uptalloc();      /* page table allocator */

/*
 * brk and sbrk system calls
 */
sbreak()
{
    struct a {
        int nva;
    };
    register preg_t *prp;
    register reg_t  *rp;
    register int    nva;
    register int    change;
    register int    n;

    /*  Find the processes data region.
     */
    prp = findpreg(u.u_procp, PT_DATA);
    if (prp == 0)
        return;
    rp = prp->p_reg;
    reglock(rp);
    
    nva = ((struct a*)u.u_ap)->nva;
    if (nva < u.u_datorg || nva <= 0)
/* loc_44: */
        nva = u.u_datorg;
/* loc_4A: */

    if (nva >= u.u_datorg + 0x2000000) {
        u.u_error = ENOMEM;
        regrele(rp);
        return;
    }
/* loc_6E: */
    if ((change = btop((int)nva) - btotp(prp->p_regva)
         - (rp->r_pgsz + rp->r_pgoff)) == 0) {
        regrele(rp);
        return;
    }
/* loc_A2: */
    if (growreg(&u, prp, change, 0, DBD_DZERO) < 0) {
        regrele(rp);
        return;
    }
    regrele(rp);

    if (change < 0)
        clratb();           /*pcs*/
}

uclear(uva, count)
register caddr_t uva;
register count;
{
    register i;

    for (i=0; i<count; i++, uva++)
        if(subyte(uva, 0) < 0) {
            return(-1);
        }
}

/*
 * grow the stack to include the SP
 * true return if successful.
 */
grow(sp)
unsigned sp;
{
    register preg_t *prp;
    register reg_t  *rp;
    register    si;

    /*  Find the processes stack region.
     */

    prp = findpreg(u.u_procp, PT_STACK);
    if(prp == NULL) {
        return(0);
    }
    rp = prp->p_reg;

    reglock(rp);

    if (sp >= (uint)USRSTACK - ptob(rp->r_pgsz - USIZE)) {
        regrele(rp);
        return(0);
    }

    si = btop((uint)USRSTACK - sp) - (rp->r_pgsz - USIZE) + SINCR;
    if (si <= 0){
        regrele(rp);
        return(0);
    }

    if(growreg(&u, prp, si, 0, DBD_DZERO) < 0){
        regrele(rp);
        return(0);
    }

    regrele(rp);
    return(1);
}
