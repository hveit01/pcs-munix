/* everything here is PCS specific */
/* @(#)machdep.c    6.3 */
static char* _Version = "@(#) RELEASE:  1.3  Oct 01 1986 /usr/sys/os/machdep.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/seg.h"
#include "sys/map.h"
#include "sys/reg.h"
#include "sys/psl.h"
#include "sys/utsname.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "sys/acct.h"
#include "sys/file.h"
#include "sys/inode.h"

/*
 * Start clock
 */
clkstart()
{
        _pcr |= 4;
}

/*
 * Send an interrupt to process
 */
sendsig(hdlr, signo, arg)
{
    register long exa7;
    long *expc;
    struct exvec *exvec;
    ushort *exstatreg;

    if (hdlr != 1) {
        exvec = u.u_exvec;
        expc = &u.u_exvec->exu.ex1.expc;
        exstatreg = &u.u_exvec->exu.ex1.exstatreg;
        exa7 = exvec->exa7;

        grow((unsigned)(exa7 -= 8));

        /* simulate an interrupt on the user's stack */
        sulong((caddr_t)(exa7+4), *expc);
        suword((caddr_t)(exa7+2), *(short*)exstatreg);
        
        /* with one parameter */
        suword((caddr_t)exa7, signo);

        /* adjust registers */
        exvec->exa7 = exa7;
        *exstatreg &= 0x3FFF;   /* clear trace bits */

        /* redirect to hndlr */
        *expc = hdlr;
    }
}

/*
 * Clear registers on exec
 */
setregs()
{
    register long *rp;
    register i;
    int save;

    rp = &u.u_exvec->exd0;
    while (rp <= &u.u_exvec->exa6)
        *rp++ = 0;
        
    rp = (long*)u.u_fpstate;
    *rp = 0;
    u.u_fpsaved++;
    
    u.u_exvec->exu.ex1.expc = u.u_exdata.ux_entloc & ~1;
    u.u_exvec->exu.ex1.exstatreg = 0;

    for (i = 0; i < NOFILE; i++) {
        if ((u.u_pofile[i]&EXCLOSE) && u.u_ofile[i] != NULL) {
            if (u.u_rofile[i]) {
                save = u.u_rval1;
                u.u_rval1 = (int)u.u_rofile[i];
                u.u_callno = -2;
                getf(i);
                u.u_rval1 = save;
                u.u_rofile[i] = NULL;
            }
            closef(u.u_ofile[i]);
            u.u_ofile[i] = NULL;
        }
    }

    /*
     * Remember file name for accounting.
     */
    u.u_acflag &= ~AFORK;
    bcopy((caddr_t)u.u_dent.d_name, (caddr_t)u.u_comm, DIRSIZ);
}

chksize(text, data, stack)
{
    register n;

    n = text + data + stack;
    if (n > maxumem) {
        u.u_error = ENOMEM;
        return(-1);
    }
    return(0);
}

/*
 * dump out the core of a process
 */
coredump(ip, n)
register struct inode *ip;
{
    register preg_t *t_prp, *d_prp, *s_prp;
    register reg_t  *rp;
    register struct proc *pp;
    char dummy[162];    /* some unused pcs data structure */

    /*  Put the region sizes into the u-block for the
     *  dump.
     */
    
    pp = u.u_procp;
    if(t_prp = findpreg(pp, PT_TEXT))
        u.u_tsize = t_prp->p_reg->r_pgsz;
    else
        u.u_tsize = 0;
    if(d_prp = findpreg(pp, PT_DATA))
        u.u_dsize = d_prp->p_reg->r_pgsz;
    else
        u.u_dsize = 0;
    if(s_prp = findpreg(pp, PT_STACK))
        u.u_ssize = s_prp->p_reg->r_pgsz;
    else
        u.u_ssize = 0;
    
    if (ip->i_flag & ILAND)
        u.u_limit = 200;

    /*  Check the sizes against the current ulimit and
     *  don't write a file bigger than ulimit.  Try to
     *  write the stack if the data + stack is too big.
     *  If that fails, at least they've got the registers.
     */

    if (USIZE + u.u_dsize + u.u_ssize > dtop(u.u_limit)) {
        u.u_dsize = 0;
        if (USIZE + u.u_ssize > dtop(u.u_limit))
            u.u_ssize = 0;
    }

    /*
     *  Write the u-block to the dump file.
     */
    if ((ip->i_flag & ILAND) == 0)
        itrunc(ip);
    makecorefil(n);
    u.u_offset = 0;
    u.u_base = (caddr_t)&corefil;
    u.u_count = sizeof(corefil);
    u.u_segflg = 1;
    u.u_fmode = FWRITE;
    wcore(ip);

    /*
     *  Write the data to the dump file.
     */
    
    u.u_segflg = 0;

    if (u.u_dsize) {
        rp = d_prp->p_reg;
        u.u_base = (caddr_t)(d_prp->p_regva + ptob(rp->r_pgoff));
        u.u_count = ptob(u.u_dsize);
        wcore(ip);
    }

    /*
     *  Write out the stack: 
     */

    if (u.u_ssize) {
        rp = s_prp->p_reg;
        u.u_base = (caddr_t)(SYSVA - ptob(rp->r_pgsz));
        u.u_count = ptob(u.u_ssize - USIZE);
        wcore(ip);
    }
    
    /*
     * Notify network
     */
    if (ip->i_flag & ILAND) {
        u.u_callno = -29;
        uisend(u.u_cdirdev, 0);
    }
}

extern unsigned end;

logtophys(addr)
int addr;
{
    if (addr < (unsigned)SYSVA || addr >= (unsigned)&end) {
        printf("error: logtophys %x\n", addr);
        return addr;
    }
    return (ptob(svtop(addr)) + 0x40000) + (addr & POFFMASK);
}

map_gpgs_dma(addr, c)
{
    extern unsigned gpgsdma;
    register pte_t *pt, *pt1;

    pt = (pte_t*)svtopte(addr);
    pt1 = (pte_t*)&_dmapt[gpgsdma];
    while (c-- != 0)
        pt1++->pgi.pg_pte = pt++->pgi.pg_pte;

    return ptob(gpgsdma);
}

map_dma(addr, size)
{
    register pte_t *pt, *pt1;

    unsigned base;
    int c;

    c = btotp(size - 1 + addr) - btotp(addr) + 1;
    base = malloc(dmamap, c);

    pt = (pte_t*)svtopte(addr);
    pt1 = (pte_t*)&_dmapt[base];
    while (c-- != 0)
        pt1++->pgi.pg_pte = pt++->pgi.pg_pte;

    return ptob(base) | (addr & POFFMASK);
}

map_18dma(addr, size)
{
    register pte_t *pt, *pt1;

    unsigned base;
    int c;

    c = btotp(size - 1 + addr) - btotp(addr) + 1;
    base = malloc(dma18map, c);

    pt = (pte_t*)svtopte(addr);
    pt1 = (pte_t*)&_dmapt[base];
    while (c-- != 0)
        pt1++->pgi.pg_pte = pt++->pgi.pg_pte;

    return ptob(base) | (addr & POFFMASK);
}

dmainit()
{
    mfree(dma18map, 64, 0);
    mfree(dmamap, 448, 64);
    
    if (map_dma((caddr_t)SYSVA, (((caddr_t)&end)-SYSVA)) != 0x40000)
        panic("dmainit");
}

savfp()
{
    register char* a5 = u.u_fpstate;
    0xf315; /* fsave a5@ */
    if (*(ushort*)a5 != 0) {
        a5 = u.u_fpstate;
        0xf225; 0xe0ff; /* fmovem fp0-fp7, a5@- */
        0xf225; 0xbc00; /* fmoveml fpcr/fpsr/fpiar, a5@- */
    }
}

restfp()
{
    register char *a5, *a4;
    a4 = u.u_fpstate;
    if (u.u_m881 == 0)
        *(ushort*)a4 = 0;
    else {
        if (*(ushort*)a4) {
            a5 = (char*)&u.u_fpvec.fpcr;
            0xf21d; 0x9c00; /* fmovem a5@+, fpcr/fpsr/fpiar */
            0xf21d; 0xd0ff; /* fmovem a5@+, fp0-fp7 */
        }
    }
    a5 = a4;
    0xf355; /* frestore a5@ */
}
