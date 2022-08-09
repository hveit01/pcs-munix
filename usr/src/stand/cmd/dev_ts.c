#include "data.h"

struct tsdev {
    short tsdb;
    short tssr;
};
#define TSDEV ((struct tsdev*)TSADDR)

struct tscmd {
    short cmd;
    short loba;
    short hiba;
    short size;
};

/* this is to ensure that ts_cmd is 64-byte aligned */
static char _tsalign[66];
static struct tscmd _tscmd;

static struct tscmd *ts_cmd;    /* aligned pointer to _tscmd */

struct tscharac {
    short loba;
    short hiba;
    short size;
    short mode;
};
static struct tscharac ts_chrbuf;

struct tssts {
    short s_sts;
    short len;
    short rbpcr;
    short sx0, sx1, sx2, sx3;
    short _reserved;
};
static struct tssts ts_mesbuf;

static short ts_cmdaddr;
static short ts_mbufaddr;
static short ts_mbufaddrext;
static short ts_cxbufaddr;
static short ts_cxbufaddrext;

int ts_open(bp)
register struct iobuf *bp;
{
    register int unit = bp->b_unit;
    register int i;
    int phys;
    
    if (unit) {                         /* can only use TS0 */
        printf("TS: unit %d not supported\n", unit);
        return -1;
    }

    if (mem_readword(TSADDR) == -1)     /* no device? */
        return -1;

    /* guarantee that TS mailbox is 64-byte aligned */
    ts_cmd = (struct tscmd*)(((int)&_tscmd) & 0x3fffffc0);  /* align */

    phys = phys_offset(ts_cmd);         /* physical addr of ts_cmd */
    ts_cmdaddr = phys;
    ts_cmdaddr |= (phys >> 16) & 3;
    
    phys = phys_offset(&ts_mesbuf);     /* physical addr of ts_mesbuf */
    ts_mbufaddr = phys;
    ts_mbufaddrext = (phys >> 16) & 3;
    
    phys = phys_offset(&ts_chrbuf);     /* physical addr of ts_chrbuf */
    ts_cxbufaddr = phys;
    ts_cxbufaddrext = (phys >> 16) & 3;

    ts_setcharac(TSADDR);               /* device characteristics */

    ts_sendcmd(017, TSADDR);            /* GSTAT command */
    if (TSDEV->tssr & 0100) {           /* offline? */
        printf ("TS: tape offline\n");
        return -1;
    }
    
    ts_sendcmd(02010, TSADDR);          /* REWIND */

    for (i= bp->b_fnum; i-- != 0; ) {   /* file number */
        bp->b_count = 0;
        ts_sendcmd(01010, TSADDR);      /* FWD */
    }
    return 0;
}

ts_setcharac(io)
register struct tsdev *io;
{
    ts_cmd->cmd = 0140004;              /* set characteristics */
    ts_cmd->loba = ts_cxbufaddr;
    ts_cmd->hiba = ts_cxbufaddrext;
    ts_cmd->size = sizeof(struct tscharac);

    ts_chrbuf.loba = ts_mbufaddr;       /* status mailbox */
    ts_chrbuf.hiba = ts_mbufaddrext;
    ts_chrbuf.size = sizeof(struct tssts);
    ts_chrbuf.mode = 0;

    io->tsdb = ts_cmdaddr;              /* send cmd addr to drive */
    while ((io->tssr & 0200)==0);       /* wait for done */
}

ts_close(bp)
register struct iobuf *bp;
{
    register int flags = bp->b_flags;
    if (flags & 2) {
        ts_sendcmd(011, TSADDR);        /* SFORW|READ */
        ts_sendcmd(011, TSADDR);        /* SFORW|READ */
    }
    ts_sendcmd(02010, TSADDR);          /* REWIND */
}

int ts_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    register struct tsdev *io = TSDEV;
    register int cmd;
    int errcnt = 0;
    int dummy;
    int tssr;

    bpsecbuf_phys(bp);                  /* set b_dma */

    cmd = rwcmd==1 ? 1 : (rwcmd==2 ? 5 : rwcmd); /* read, write, other */

    if ((io->tssr & 0200)==0) {         /* no ready? */
        printf("TS: unit not ready\n");
        while ((tssr & 0200)==0);       /* wait for ready */
    }
    
    for (;;) {
        ts_cmd->cmd = 0140017;          /* cmd = ACK|CVC|GSTAT */
        io->tsdb = ts_cmdaddr;          /* send command */
        while ((tssr & 0200)==0);       /* wait for ready */

        ts_cmd->loba = bp->b_dma;
        ts_cmd->hiba = bp->b_dma >>16;
        ts_cmd->size = bp->b_count;
        ts_cmd->cmd = cmd | 0x8000;     /* read/write cmd | ACK */
        io->tsdb = ts_cmdaddr;          /* send command */
        while ((tssr & 0200)==0);       /* wait for ready */

        if ((tssr = io->tssr) & 0x8000) {   /* error? */
            if ((ts_mesbuf.sx0 & 0x8000) ||     /* TMK tape mark? */
                (ts_mesbuf.sx0 & 050000)) {     /* RLS|RLL ? */
                return bp->b_count - ts_mesbuf.rbpcr;
            }
        } else {                        /* no error */
            return bp->b_count - ts_mesbuf.rbpcr;
        }
        
        if ((tssr & 016)==012 || (tssr & 016)==010)
            ts_sendcmd(0410, io);
        else
            errcnt = 9;
        
        if (errcnt >= 9) {
            printf("TS: tape error, tssr=%4x\n", tssr);
            printf("    ext. status rbpcr=%4x xst0=%4x xst1=%4x xst2=%4x xst3=%4x\n",
                ts_mesbuf.rbpcr, ts_mesbuf.sx0, ts_mesbuf.sx1, 
                ts_mesbuf.sx2, ts_mesbuf.sx3);
        }
        
        if (cmd==5) {                   /* was write? */
            while ((io->tssr & 0200)==0); /* wait for ready */
            ts_sendcmd(0411, io);       /* SREV|READ */
        }
        
        errcnt++;
        if (errcnt >= 10) break;
    }
    return -1;
}

int ts_sendcmd(cmd, io)
int cmd;
register struct tsdev *io;
{
    ts_cmd->cmd = cmd | 0140000;        /* add ACK|CVC */
    ts_cmd->loba = 1;
    io->tsdb = ts_cmdaddr;
    return ts_execcmd(io);
}

int ts_execcmd(io)
register struct tsdev *io;
{
    static int ts_tapemark;
    register int tssr;

    while ((io->tssr & 0200)==0);       /* wait for ready */
    tssr = io->tssr;
    
    if ((tssr & 0100000)==0)            /* check for SC */
        return 1;

    if (ts_mesbuf.sx0 & 0x8000) {       /* TMK seen ? */
        ts_tapemark++;
        return 1;
    }
    if (ts_mesbuf.sx0 & 050000)         /* RLS|RLL ?*/
        return 1;

    printf("TS: tape error, tssr=%4x\n", tssr);
    printf("    ext. status rbpcr=%4x xst0=%4x xst1=%4x xst2=%4x xst3=%4x\n",
        ts_mesbuf.rbpcr, ts_mesbuf.sx0, ts_mesbuf.sx1, 
        ts_mesbuf.sx2, ts_mesbuf.sx3);
    return 0;
}
