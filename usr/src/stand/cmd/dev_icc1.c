#include "data.h"
#include <sys/icc/pikitor.h>

/* qbus init structure for ICC  */
struct iccdev {
    ushort cr0, cr1, cr2;   /* only low bytes valid */
    ushort setint;
};

ICC_PORT *icc_port;
static struct {
    short maj, min;
} pik_version;                          /* version# of pikitor */
static char icc_initret[16];
char icc_version[160];
static int scc_charavail;

icc_detect()
{
    register struct iccdev *io = (struct iccdev*)ICCADDR;
    int dummy1;
    int delay;
    int icc_phys;
    int dummy2;
    ICC_RESULT result;

    hasicc = mem_writeword(io, 0) != -1;
    if (!hasicc)
        return -1;

    icc_port = (ICC_PORT*)ICCMBX;
                                        /* icc workspace at SYSVA+0xf7000 */
    icc_phys = ICCMBX-SYSVA;            /* phys addr = 0xf7000 */
    icc_clearbuf();
    if (coldinit == 0)
        printf("\n\nconnecting to icc\n");
    else {
        icc_port->result = 0;           /* clear result */
        io->cr0 = icc_phys >> 16;       /* high byte (of 3 bytes) */
        io->cr1 = icc_phys >> 8;        /* middle byte */
        io->cr2 = icc_phys;             /* low byte */
        io->setint = SETINT_RESET;      /* reset interrupt */
        
        delay = 100000;
        do {
            if (icc_port->result)       /* wait for reset done */
                break;
        } while (--delay);
    }

    /* setup regular mailbox */
    io->cr0 = icc_phys >> 16;           /* high byte (of 3 bytes) */
    io->cr1 = icc_phys >> 8;            /* middle byte */
    io->cr2 = icc_phys;                 /* low byte */
    io->setint = 1;                     /* normal interrupt */

    icc_noprint = 0;                    /* allow ICC console */

    if (coldinit == 0) {
        icc_port->par[0] = phys_offset(&pik_version);   /* 2 short */
        icc_port->par[1] = phys_offset(icc_initret);    /* 16 bytes */
        icc_port->par[2] = phys_offset(icc_version);    /* 160 bytes */
        icc_version[0] = '\0';
        result = icc_call(ICC_INITIAL, 5842900);    /* initialize */
        if (result == E_TIMEOUT)
            return -1;

        printf(icc_version);
        printf("Pikitor Version %4x %4x\n", pik_version.maj, pik_version.min);
    }
    return 0;
}

ICC_RESULT icc_call(func, wait)
int func, wait;
{
    icc_port->func = func;              /* set command */
    
    for(;;) {
        if (--wait == 0) {              /* timeout */
            icc_init();                 /* reinitialize ICC */
            return E_TIMEOUT;           /* return error */
        }
        clrcache();                     /* clear cache to ensure correct data */
        if (icc_port->func == 0)
            return icc_port->result;
    }
}

icc_init()
{
    int icc_phys;
    register struct iccdev *io = (struct iccdev*)ICCADDR;

    icc_clearbuf();                     /* clear mailbox */

    io->setint = SETINT_RESET;          /* issue reset */
    icc_phys = SYSVA-ICCMBX;            /* physical addr of mailbox */
    io->cr0 = icc_phys >> 16;           /* high byte (of 3 bytes) */
    io->cr1 = icc_phys >> 8;            /* middle byte */
    io->cr2 = icc_phys;                 /* low byte */
    io->setint = 1;                     /* normal int */
}

/* clear ICC mailbox */
icc_clearbuf()
{
    register int *p = (int*)ICCMBX;
    while (p < (int*)(ICCMBX + 0xc00))
        *p++ = 0;
}

/* download to ICC */
icc_load()
{
    register int pfn;
    
    for (pfn = 0x100; pfn < 0x200; pfn++)
        _dmapt[pfn].pgi.pg_pte = pfn + 0xafffff00; /* PG_V|PG_KW|PG_KR+(pfn-0x100) */
    
    icc_port->par[0] = 0x100000;        /* 1MB */
    icc_port->par[1] = textsize;
    icc_port->par[2] = ptob(btop(textsize))+ 0x100000;
    icc_port->par[3] = datasize;
    icc_port->par[4] = 1024;

    if (icc_call(ICC_LOAD, 0x390f4) < 0)
        printf("? Error: cannot load to ICC\n");
}

/* send ICC_START command to ICC */
icc_start()
{
    icc_port->par[0] = 1024;
    if (icc_call(ICC_START, 233716) < 0)
        printf("? Error: cannot start ICC\n");
}

icc_debug()
{
    icc_call(ICC_DEBUG, -1);
}

icc_idle()
{
    icc_call(ICC_IDLE, -1);
}

int icc_st_open(bp, type)
register struct iobuf *bp;
int type;
{
    ICC_RESULT result;
    
    if (!hasicc) {
        printf("Missing ICC 0\n");
        return -1;
    }

    icc_port->par[0] = type;
    result = icc_call(IS_MODE, 7011480);
    if (result >= 0) {
        icc_port->par[0] = bp->b_fnum;  /* file number */
        result = icc_call(IS_SKIP, 23371600);
    }
    if (result < 0) {
        printf("IS: cannot open\n");
        return -1;
    }
    return 0;
}

int icc_is_open(bp)
struct iobuf *bp;
{
    return icc_st_open(bp, 0);
}

int icc_iq_open(bp)
struct iobuf *bp;
{
    return icc_st_open(bp, 1);
}

icc_st_close(bp)
register struct iobuf *bp;
{
    icc_port->par[0] = 0;
    icc_call(IS_SKIP, 23371600);
}

int icc_st_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    if (bp->b_count % 512) {
        printf("IS: count not a multipla of 512\n");
        exit();
    }
    
    icc_port->par[0] = bp->b_count;
    icc_port->par[1] = bp->b_dma;
    icc_port->par[2] = 1;
    if (icc_call( rwcmd==1 ? IS_READ : IS_WRITE, 2337160) < 512) {
        printf("IS: cannot %s\n", rwcmd==1 ? "read" : "write");
        return -1;
    }
    return bp->b_count;
}

int icc_iw_open(bp)
register struct iobuf *bp;
{
    ICC_RESULT result;
    
    if (!hasicc) {
        printf("Missing ICC 0\n");
        return -1;
    }

    icc_port->par[0] =  bp->b_unit;
    result = icc_call(IW_OPEN, 7011480);
    if (result <= 0) {
        if (!noopenerr)
            printf("IW: cannot open\n");
        return -1;
    }
    return 0;
}

int icc_iw_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    bpsecbuf_phys(bp);                  /* make b_dma */
    if (bp->b_count % 512) {
        printf("IW: count not a multiple of 512\n");
        exit();
    }
    
    icc_port->par[0] = bp->b_unit;
    icc_port->par[1] = bp->b_sector;
    icc_port->par[2] = bp->b_count;
    icc_port->par[3] = bp->b_dma;
    if (icc_call(rwcmd==1 ? IW_READ : IW_WRITE, 467432) < 0) {
        printf("IW: cannot %s bno = 0x%lx %d.\n",
            rwcmd==1 ? "read" : "write", bp->b_sector, bp->b_sector);
        return -1;
    }
    return bp->b_count;
}

/* send a char through ICC_PORT mailbox to ICC SCC */
scc_write(ch)
char ch;
{
    if (icc_noprint)
        return;
    
    icc_port->par[5] = ch;
    if (icc_call(PUTC, 1168580) == E_NONEXIST)
        print_timeout(7);               /* timeout of ICC SCC */
}

/* get a char from ICC SCC */
int scc_read()
{
    int ch;
    
    if (scc_charavail) {
        ch = scc_charavail;
        scc_charavail = 0;
        return ch;
    }

    icc_call(GETC, -1);
    return icc_port->result;
}

int scc_haschar()
{
    if (icc_noprint)                    /* ICC console disabled? */
        return 0;
    
    icc_call(GETC0, 116858);            /* check for char */
    scc_charavail = icc_port->result;
    return scc_charavail;
}
