/* part 1 of the memtest routines
 * interface to the monitor
 */
#include "data.h"

extern clrca_dis(), clrcache_cb3();
extern mem_initspt();
extern char mem_version[];
extern mem_runtests();
extern int mem_endtests;

static mem_clear();

/* run a memory test
 * clear memory first
 * reloc != 0 - copy testcode to reloc addr
 * reloc = 0  - run testcode out of ROM
 */
typedef int (*FUNCPTR)();
 
/* public API to monitor */
mem_test(reloc)
FUNCPTR reloc;
{
    int prot;
    
    printf("\n");
    clrca_dis();                        /* disable processor cache */
    clrcache_cb3();                     /* clear and disable cache, clear CCR bit 3 */

    printf("\n");
    printf("\t***********************************************************\n");
    printf("\t*                                                         *\n");
    printf("\t*                 QU 68/32 Memory Test                    *\n");
    printf("\t*                                                         *\n");
    printf("\t***********************************************************\n");
    printf("\t\t%s\n", mem_version);

    printf("\n");
    mem_clear();                        /* clear memory */

    prot = PG_V|PG_KR|PG_KW;            /* default page protection */
    mem_initspt(SYSVA, 0, 1024, prot);  /* initialize 1024 pages in SPT */

    setvbr(SYSVA);                      /* point vector table to low memory */
    if (reloc==0)
        mem_runtests();                 /* run memory tests out of ROM */
    else {
        register char *from = (char*)mem_runtests;
        register char *to =   (char*)reloc;
        register char *end =  (char*)&mem_endtests;
        FUNCPTR func;
        
        while (from < end)              /* copy relocatable memory routines */
            *to++ = *from++;

        func = reloc;                   /* call memory tests */
        (*func)();
    }
    
    setvbr(MINITOR);                    /* restore VBR to ROM */
    
    printf("end of memtest\n");
}

/* map upto size=4M of PA to VA area (4MB) in SPT
 * and clear VA area
 */
static mem_clear()
{
    short *pbcr;
    int *pastart;
    int *p;
    int memsize;
    register int *vastart, *vaend, vasize;

    printf("clearing memory\n");
    
    pbcr = _pbcr;                      /* memory config */
    
    if (*pbcr & 1) {                    /* bit 0 is set = 4MB memory board 0 */
        pastart = (int*)0x100000;       /* physical addr 1MB */
        vastart = (int*)0x3fa00000;     /* map virtual addr from here ... */
        vaend   = (int*)0x3fd00000;     /* ... to there */
        vasize  =       0x300000;       /* 3MB size */
        mem_initspt(vastart, pastart, vasize, PG_V|PG_KW); /* map to SPT */
        
        for (p = vastart; p < vaend; )  /* clear memory */
            *p++ = 0;

        memsize = 4;                    /* memory is 4 MB */
    }
    if (*pbcr & 2) {
        pastart = (int*)0x400000;       /* physical addr 4MB */
        vastart = (int*)0x3fa00000;     /* map VA from here ... */
        vaend   = (int*)0x3fe00000;     /* ... to there  */
        vasize  =       0x400000;       /* 4 MB */
        mem_initspt(vastart, pastart, vasize, PG_V|PG_KW); /* map to SPT */
        
        for (p = vastart; p < vaend; )  /* clear memory */
            *p++ = 0;

        memsize = 8;                    /* memory is 8 MB */
    }
    if (*pbcr & 4) {
        pastart = (int*)0x800000;       /* physical addr 8MB */
        vastart = (int*)0x3fa00000;     /* map VA from here ... */
        vaend   = (int*)0x3fe00000;     /* ... to there  */
        vasize  =       0x400000;       /* 4 MB */
        mem_initspt(vastart, pastart, vasize, PG_V|PG_KW); /* map to SPT */
        
        for (p = vastart; p < vaend; )  /* clear memory */
            *p++ = 0;

        memsize = 12;                   /* memory is 12 MB */
    }
    if (*pbcr & 8) {
        pastart = (int*)0xC00000;       /* physical addr 12MB */
        vastart = (int*)0x3fa00000;     /* map VA from here ... */
        vaend   = (int*)0x3fe00000;     /* ... to there  */
        vasize  =       0x400000;       /* 4 MB */
        mem_initspt(vastart, pastart, vasize, PG_V|PG_KW); /* map to SPT */
        
        for (p = vastart; p < vaend; )  /* clear memory */
            *p++ = 0;

        memsize = 16;                   /* memory is 12 MB */
    }

    printf("Total memory: %d Megabyte\n", memsize);
}
