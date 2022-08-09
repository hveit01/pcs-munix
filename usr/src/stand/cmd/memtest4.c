#include "data.h"

/* get start and end addresses */
mem_getlimits(startp, endp)
unsigned int *startp, *endp;
{
    char buf[48];
    pte_t prot;
    int sysram;
    unsigned int endaddr, startaddr;
    int size;

    sysram = SYSRAM;                    /* start of reserved system area */

    for (;;) {
        printf("Startaddress (>=#100000): ");
        gets(buf);
        if (buf[0]=='\0' || buf[0] == '\r')
            *startp = HIGHMEM;
        else
            *startp = atoi(buf);

        if (*startp >= 0x80000000)      /* device memory, e.g. COLOR card? */
            break;
        if (*startp <= HIGHMEM && *startp >= MINITOR) /* in Minitor? */
            break;
        if (*startp >= 0x100000 && *startp < 0x1000000) /* physical RAM? */
            break;
        /* address is not acceptable, so ask again */
    }
    
    for (;;) {
        if (*startp < 0x1000000)            /* allowed range for phys RAM */
            printf("Endaddress (<#1000000): ");
        else if (*startp < HIGHEND)
            printf("Endaddress (<#3fe00000): ");
        else if (*startp < MINITOR)
            printf("Endaddress (<#3ff80000): ");
        else
            printf("Endaddress: ");
    
        gets(buf);
        if (buf[0] == '\0' || buf[0] == '\r') {
            if (*startp >= HIGHMEM)
                *endp = HIGHMEM + 0xffffc;  /* 1MB-sizeof(int) */
            else
                *endp = 0x3ffffc;       /* 4MB - sizeof(int) */
        } else
            *endp = atoi(buf);

        if (*startp < *endp) {
            if (*endp < 0x1000000)      /* accept 1MB to 16MB */
                break;
            if (*startp < HIGHEND && *endp < MINITOR)
                break;                  /* accept HIGH RAM */
            if (*startp < SYSVA && *endp < HIGHEND)
                break;                  /* accept mapping area */
        }
    }

    prot.pgi.pg_pte = PG_V|PG_KR|PG_KW;
    size = *endp - *startp + 1;         /* size of area */
    endaddr = *endp & 0x3fffffff;       /* throw away bits 30, 31 */
    startaddr = *startp & 0x3fffffff;   /* throw away bits 30, 31 */
    if (endaddr >= SYSVA) {
        if (endaddr < HIGHEND) {
            printf("Memory Test of Q-Bus Memory\n");
            /* map address to _sbrpte[0x200] and above */
            test_mapqbus(endaddr, endaddr - 0x3fc00000, size, prot);
        } else {
            if (*startp >= 0x80000000) /* frame buffer memory */
                printf("Frame   Buffer Memory test\n");
            else
                printf("Supervisor Memory Test\n");
            mem_initspt(endaddr, SYSVA, size, prot);
        }
    } else {
        if (*startp >= 0x80000000)
            printf("Frame Buffer Memory Test\n");
        else
            printf("User Memory Test\n");
        mem_setptbr(sysram, endaddr, size);
    }
}

mem_setptbr(pgtbl, endaddr, size)
pte_t *pgtbl;
int endaddr;
int size;
{
    int i;
    short *ptbr;
    pte_t prot;
    char *end;
    int ptnum;

#define ONEMB   0x100000

    prot.pgi.pg_pte = PG_V|PG_KR|PG_KW; /* page protection */
    end = (char*)endaddr;
    ptnum = (int)pgtbl - SYSVA;
    ptbr = &_ptbr[endaddr / ONEMB];
    
    for (i=0; i < size; i+= ONEMB) {    /* in steps of 1MB */
        ptnum = (int)pgtbl - SYSVA;     /* calculate PT index */
        mem_setspt(pgtbl, ptnum, end, ONEMB, prot); /* fill page table */
        *ptbr = btotc(ptnum) | 0xffffc000;  /* set PTBR */
        ptbr++;
        pgtbl = &pgtbl[NPGPT];          /* to next 1MB in pagetbl 1MB=NPGPT*NBPP */
        end = &end[ONEMB];              /* advance to next MB in memory */
    }
    clratbcache();                      /* clear caches */
}

/* fill page table with page entries */
mem_setspt(pgtbl, ptnum, endaddr, size, prot)
pte_t *pgtbl;
char *endaddr;
int ptnum, size, prot;
{
    register int i, pfn;
    register pte_t *p;

    mem_initspt(pgtbl, ptnum, btotc(size), prot);
    
    pfn = btotp(endaddr);
    p = pgtbl;
    for (i = 0; i < size; i += NBPP) {
        p->pgi.pg_pte = pfn | prot;
        p++; pfn++;
    }
}

/* map QBUS memory on page table for memtest */
test_mapqbus(va, pa, size, prot)
int va, pa, size, prot;
{
    register int i, pfn;
    register pte_t *p;

    pfn = btotp(pa);
    p = &_sbrpte[btotp(va - HIGHMEM) + 0x600];
    for (i=0; i < size; i += NBPP) {
        p->pgi.pg_pte = pfn + prot;
        p++; pfn++;
    }
}

/* initialize the SPT for the range start...start+size
 * for given ramsize
 * e.g. start=0x3fa00000...0x3fd0000 (size=0x300000)
 * for 4MB
 * -> fill spt[0x200] = prot + 0
 *    spt[0x201] = prot + 1
 *    ...
 *    spt[0x4ff] = proto + 0x4ff
 */
mem_initspt(va, pa, size, prot)
int va, pa, size, prot;
{
    register int i, pfn;
    register pte_t *p;

    if (pa > 0x1000000)                 /* larger than 16MB? */
        return -1;

    if (va < SYSVA)                     /* less than SYSVA? */
        return -1;
    
    if (va > HIGHEND)                   /* may not be in QBUS area */
        return -1;

    pfn = btotp(pa);
    p = &_sbrpte[btotp(va)];
    for (i=0; i < size; i += NBPP) {
        p->pgi.pg_pte = pfn + prot;
        p++; pfn++;
    }
    
    /* bug: returns undefined value */
}
