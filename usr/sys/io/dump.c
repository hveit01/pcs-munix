/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.1  Aug 20 1986 /usr/sys/io/dump.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/page.h"

extern icc_connect();
extern clrcache();

static unsigned seg;

#define ST_CSPORT (short*)0x3fffff80
#define ST_DPORT  (short*)0x3fffffa0

#define ST_CMD(x)   *st_cst = x
#define ST_STAT()   (*st_cst)
#define ST_READ()   (*st_data)
#define ST_WRIT(x)  *st_data = (x)

static readstatus()
{
    int stat[3];
    register short *st_cst = ST_CSPORT;
    register short *st_data = ST_DPORT;
    register i;

    while ((ST_STAT() & 0x82) == 0);
    ST_CMD(0xc005);
    while (ST_STAT() != 0x81);
    
    for (i=0; i < 3; i++) {
        while ((ST_STAT() & 0x80) == 0);
        stat[i] = ST_READ();
    }
    
    return (stat[0] & 0x100) ? 1 : 0;
}

int *dumppte;
short *dumpvad;

st_dump()
{
    pte_t *pt;
    register i;
    register short *st_cst = ST_CSPORT;
    register short *st_data = ST_DPORT;
    register short *va;

    pt = (pte_t*)dumppte;

    ST_CMD(3);
    while (ST_STAT() != 3);
    readstatus();
    
    ST_CMD(0x2101);
    while (ST_STAT() != 0x81);
    readstatus();

    ST_CMD(0x4005);
    while ((ST_STAT() & 0x82)==0);

    if (ST_STAT() & 2) {
        readstatus();
        return;
    }

    seg = 0;
    for (;;) {
        clrcache();
        pt->pgi.pg_pte = (PG_V|PG_KW) | (seg>>3);
        va = (short*)(((seg << 9) & POFFMASK) | (int)dumpvad);
        seg++;

        i = 256; /* words to transfer */
        if (storacc(va) == 0)
            break;
        do {
            while ((ST_STAT() & 3) == 0);
            ST_WRIT(*va++);
        } while (--i != 0);
        
        while ((ST_STAT() & 3) == 0);
        if (ST_STAT() & 2)
            break;
    }

    ST_CMD(0x6005);
    while (ST_STAT() != 0x81);
    readstatus();

    ST_CMD(0x2101);
    while (ST_STAT() != 0x81);
    readstatus();
}

int Sbrpte[0x700];

dump()
{
    int ch;
    register i;

    spltty();
    icc_connect();

    do {
        printf("\nDo you want a dump (y/n) ? : ");
        ch = getchar();
        putchar('\n');
        if (ch == 'n')
            return;
    } while (ch != 'y');

loop:
    printf("Dump to IS/ST (i/s) ? : ");
    ch = getchar();
    putchar('\n');
        
    for (i = 0; i < 0x700; i++)
        Sbrpte[i] = sbrpte[i];

    dumpvad = (short*)0x3fdff000; /* _mfp - 1 page */
    dumppte = &sbrpte[0x5ff];
        
    switch (ch) {
    case 'I':
    case 'i':
        is_dump();
        break;
    case 'S':
    case 's':
        st_dump();
        break;
    default:
        goto loop;
    }
    printf("\nend of dump\n");
}
