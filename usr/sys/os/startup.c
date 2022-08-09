/* @(#)startup.c    1.7 */
static char* _Version = "@(#) RELEASE:  1.7  Mar 11 1987 /usr/sys/os/startup.c ";

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
#include "sys/utsname.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "sys/acct.h"
#include "sys/tty.h"
#include "sys/var.h"
#include "sys/pfdat.h"
#include "sys/buf.h"
#include "sys/debug.h"
#include "core.h"
#include "sys/bmt/gdi.h"
#include "sys/bmt/gdisys.h"
#include "sys/bmt/cbip.h"
#include "sys/console.h"

/*int p0init_done = 0;*/


int          con_type;          /*pcs*/
struct pfdat    *pfhead, *pfdat;

/*pt_t      *baseseg;*/
spte_t      *copypte;
caddr_t     copyvad;
spte_t      *mmpte;
caddr_t     mmvad;
/*spte_t        *pmapte;
caddr_t     pmavad;
caddr_t     memcvad;*/
spte_t      *userpte;
struct user *uservad;

short       colourpt[2];        /*pcs*/
int         OnBoardIO;          /*pcs*/
extern int  bootconsole;        /*pcs*/
extern      chconsdev();        /*pcs*/
extern int  bip_cnt;            /*pcs*/
extern int  bip_addr[];         /*pcs*/

extern int  maxpmem;    /* Configured physical memory limit */

extern int  etext;
extern int  end;
extern int  sigcode();

/*
 * Initially, running in physical memory without memory map and 
 * using interrupt stack.
 *
 * Allocate system virtual address space and setup virtual windows.
 *
 * Dedicate memory for boot time tables and thread rest on free list.
 *
 * Create address space for proc 0.  After vadrspace returns, we switch
 * to the hardware process context of proc 0.
 */
vadrspace(lastaddr, startpc)
{
    register i, k, n;
    register spte_t *sbr;
    int nspte, ubase;   /* 20, 24 */
    int svbip, trace;   /* 28, 2C */
    pte_t bippte;       /* 30 */

    /*pcs
     * initialize boot console
     */
    chconsdev(bootconsole);

    /*pcs
     * set available memory
     */
    availsmem = nswap >> 3;
    copyvad = (caddr_t)ptosv(1000);
    copypte = (spte_t*)&sbrpte[1000];
    
#define STKTOP stackbas(-1)
    i = btop((int)&end - SYSVA);
    
#define PTE_TRLOW       0x100
#define PTE_COLOURPT1   0x300
#define PTE_COLOURPT2   0x301

#define PTE_BIP         0x600
#define PTE_COLOUR2     0x800
#define PTE_COLOUR1     0xc00

#define BIPVA           ptosv(PTE_BIP)
#define COLOR_PAGES     256

#define BIOS_COLD       ((int)0x3ff80000)
#define BIOS_TRACE      ((int)0x3F800024)
    trace = BIOS_COLD < *(int*)BIOS_TRACE;  /*from BIOS*/
    if (trace) {
        printf("Kernel traced by Monitor\n");
        i = PTE_TRLOW;
    }
    /*loc80*/
    for (; i < (maxpmem ? maxpmem : 0x1000) &&
           clearpage(i) == 0; i++) ;
    /* loc_AC: */
    physmem = i;

    /*
     * Setup text system page table entries (RO)
     * Setup system data and bss and page table itself (RW)
     */
    sbr = (spte_t*)sbrpte;
    for (i = 0; i < btop((int)(&etext) - SYSVA); i++)
        /* loc_BC: */
        sbr++->pgi.pg_pte = PG_V | PG_KR | i;
    for ( ; i < btop((int)(&end) - SYSVA); i++)
        /* loc_E0: */
        sbr++->pgi.pg_pte = PG_V | PG_KW | i;
        
    if (trace) {
        i = PTE_TRLOW;
        sbr = (spte_t*)&sbrpte[i];
    }
    
    /*pcs
     * clear pte area up to BIP area
     */
    svbip = PTE_BIP;
    for (k = i; k < svbip; k++)
        /* loc_124: */
        sbr++->pgi.pg_pte = 0;

    /*
     * Initialize system page table map
     * and turn on the MMU
     */

    mfree(pgmap, svbip-i, i);
    clratb();

    /*
     * Allocate physical memory for dynamic tables 
     */

    ubase = i;
    ubase = mktables(ubase);
    
    /*pcs
     * allocate physical memory for BIP devices
     */
    for (sbr = (spte_t*)&sbrpte[PTE_BIP], i=0; i < bip_cnt; i++) {
        /* loc_16A: */ 
        bippte.pgi.pg_pte = PG_V | PG_UW | btop(bip_addr[i]);
        for (n = 0; n < 64; n++)
            /* loc_18C: */
            sbr++->pgi.pg_pte = bippte.pgi.pg_pte++;
        bip_addr[i] = (i << 18) + BIPVA;
    }
    
    /*
     * Initialize queue of free pages
     */
    meminit(ubase, physmem);

    /*
     * Virtual address space for acessing ublock
     */
    uservad = (struct user *)pgalloc(btop(sizeof u), 0, -1);
    userpte = (spte_t *)svtopte(uservad);

    /*
     * Virtual address space to assist physical page to physical page 
     * copy
     */
    copyvad = (caddr_t)pgalloc(2, 0, -1);
    copypte = (spte_t *)svtopte(copyvad);

    /*
     * Virtual address space for memory driver
     */
    mmvad = (caddr_t)pgalloc(1, 0, -1);
    mmpte = (spte_t *)svtopte(mmvad);
    
    /*pcs
     * allocate memory for colour console 1
     */
    sbr = (spte_t*)sptalloc(1);
    colourpt[0] = vtop(sbr);
    _ptbr[PTE_COLOURPT1] = colourpt[0];
    for (i=0; i < 256; i++)
        sbr++->pgi.pg_pte = PG_V | PG_UW | (i + PTE_COLOUR1);
    if (storacc(CBIP_IOBASE) == 0) {
        colourpt[0] = 0;
        sptfree(&sbr[-256], 1, 1);
    }
    
    /*pcs
     * allocate memory for colour console 2
     */
    sbr = (spte_t*)sptalloc(1);
    colourpt[1] = vtop(sbr);
    _ptbr[PTE_COLOURPT2] = colourpt[1];
    for (i=0; i < 256; i++)
        sbr++->pgi.pg_pte = PG_V | PG_UW | (i + PTE_COLOUR2);
    if (storacc(CBIP_IOBASE + 0x100000) == 0) {
        colourpt[1] = 0;
        sptfree(&sbr[-256], 1, 1);
    }
    
    reginit();
    p0init();
}

/*
 * Create the physical address space for proc 0.  Most proc 0 parameters
 * are initialized in main(), but some must be initialized now.
 * main() is entered on the kernel stack and enough of proc 0 must be
 * set up to have a kernel stack
 */
#ifdef LOCKLOGON
int p0init_done = 0;        /* for region lock logging */
#endif
p0init()
{
    register pt_t *seg;
    register struct user *useg;
    reg_t *rp;  /* 20*/
    preg_t *prp; /*24*/
    spte_t *spt, *upt, tmp; /*28,2c,30*/
    int i; /*34*/

    /*
     * Hand craft a P0, P1 and U block for the Scheduler
     */
    curproc = &proc[0];
    curproc->p_flag = SLOAD|SSYS;
    useg = (struct user*)pgalloc(USIZE, PG_V|PG_KW, 0);
    useg->u_procp = &proc[0];
    
    /*
     * attach a stack region and place the u area where it belongs
     */
    rp = allocreg(NULL, RT_PRIVATE);
    prp = attachreg(rp, useg, SYSVA, PT_STACK, SEG_RW);
    proc->p_addr = _ptbr[0x3f7];
    if (growreg(useg, prp, USIZE, 0, DBD_DZERO) < 0)
        panic("p0init growreg");
    /*loc_414:*/
    upt = (spte_t *)ublkptaddr(proc, prp);
    spt = (spte_t *)&sbrpte[svtop(useg)];
    proc->p_addr = vtop((long)upt & ~0x3ff);
    for (i=0; i<USIZE; i++) {
        /* loc_460: */
        tmp = *upt;
        upt->pgi.pg_pte = spt->pgi.pg_pte & (PG_V|PG_KW|PG_KR|PG_PFNUM) | PG_LOCK;
        upt++;
        *spt++ = tmp;
    }
    pgfree(useg, USIZE, 0);
    regrele(rp);

#   ifdef LOCKLOGON
    p0init_done = 1;
#   endif
}

/*
 * Create system space to hold page allocation and
 * buffer mapping structures and hash tables.
 */
mktables(physpage)
register int physpage;
{
    register int ffguess;
    register int pagedpages, m;
    register preg_t *prp;
    extern int  pregpp;
    register int i;

    /*
     * Sizing of data structures to match physical memory
     * Underestimate
     */
    ffguess = svtop(&end);
    pagedpages = (physmem - ffguess);
    
    /*
     * Numerical convergence to round to a power of 2
     */
    m = pagedpages;
    while (m & (m - 1))
         m = (m | (m - 1)) + 1;

    phashmask = (m>>3) - 1;

    /*
     * Allocate system space for page structures
     */
    i = btop(pagedpages*sizeof(*pfdat));
    pfdat = ((struct pfdat *)pgalloc(i, PG_V | PG_KW, physpage)) - ffguess;
    physpage += i;

    /*
     * Hash table to find buffers
     */
    i = btop((m>>3) * sizeof(*phash));
    phash = ((struct pfdat *)pgalloc(i, PG_V | PG_KW, physpage));
    physpage += i;

    /*
     *  Allocate space for the pregion tables for each process
     *  and link them to the process table entries.
     *  The maximum number of regions allowed for is process is
     *  3 for text, data, and stack plus the maximum number
     *  of shared memory regions allowed.
     */
    
    i = btop(pregpp * sizeof(preg_t) * v.v_proc);
    prp = (preg_t *)pgalloc(i, PG_V|PG_KW, physpage);
    physpage += i;
    for(i = 0  ;  i < v.v_proc  ;  i++, prp += pregpp)
        proc[i].p_region = prp;


    return(physpage);
}

/*
 * Machine-dependent startup code
 */
startup()
{
    register enetaddr* rom_eth;
    register unsigned short* rom_serial;
    extern int icc_boot, nonstdea;
    
    rom_eth = (enetaddr*)0x3ff80108;
    rom_serial = (unsigned short*)0x3ff80104;

    dmainit();
    clkstart();
    if (con_type == icc || bmajor(rootdev) == 0x11)
        icc_boot++;
    /*loc_67E: */
    if (icc_boot)
        icc_init(0);
    /* loc_690: */
    if (rom_eth->hi != rom_eth->mi) {
        utsname.serialnum = *rom_serial;
        if (nonstdea == 0)
            utsname.uiname = *rom_eth;
    }
    
    header();
}

/*
 * Initialize clist by freeing all character blocks.
 */
struct chead cfreelist;
cinit()
{
    register n;
    register struct cblock *cp;
    extern over64k();

    for(n = 0, cp = &cfree[0]; n < v.v_clist; n++, cp++) {
        if (over64k(cp) == 0) {
            cp->c_next = cfreelist.c_next;
            cfreelist.c_next = cp;
        }
    }
    cfreelist.c_size = CLSIZE;
}

over64k(cp)
register struct cblock *cp;
{
    register int i, k;
    i = logtophys(cp->c_data);
    k = i + CLSIZE;
    return (i & ~0xffff) != (k & ~0xFFFF);
}

header()
{
    extern char* CURversion;
    
    printf(CURversion+5, utsname.version, utsname.release);
    
    printf("\nSystem = '%9s', Node = '%9s'\n",
        utsname.sysname, utsname.nodename);
    
    printf("Ethernet address = %4x %4x %4x,  serial number = %x\n",
        utsname.uiname.hi, utsname.uiname.mi, utsname.uiname.lo,
        utsname.serialnum);
    
    memusage("Total memory", ptob(physmem));
    memusage("Available free memory", ptob(maxmem));
}

memusage(str, n)
char *str;
ulong n;
{ 
    register ulong d7;
    
    d7 = n / 1000;
    
    printf("%s = %d.%d%d%d MB\n",
        str, d7/1000, d7/100%10, d7/10%10, d7 % 10);
} 
