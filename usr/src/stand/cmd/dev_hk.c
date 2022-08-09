#include "data.h"

#define NHK 8
#define HKHEAD 3
#define HKSECTRK 22
#define HKSECCYL (HKSECTRK*HKHEAD)

/* HK controller registers, see kernel driver */
struct hkdev {
    short hkcs1, hkwc, hkba, hkda;
    short hkcs2, hkds, hker, hkas;
    short hkdc, hkspr, hkdb, hkmr;
    short hkec1, hkec2, hkmr2, hkmr3;
};

static char hk_drives[NHK];
static short hk_hasbad[NHK];
static short hk_cs1[NHK];
static short hk_nblocks[NHK];

struct hkinfo {
    short unit;
    short cmd;
    short cnt;
    short blk;
    short bad;
    int   buf;
    short bad_cs1;
    short bad_cs2;
    short bad_ds;
    short bad_er;
};
static struct hkinfo hk_info;

struct hkbad {
    int hk_dummy;
    int hk_count;
    int hk_badblk[126];
    int hk_altblk[126];
};
static struct hkbad hk_badblktbls[NHK];


/* select drive type RK07
 * wait for drive to become ready (unused)
 * Attributes: bp-based frame
 */
hk_setrk07()
{
    register struct hkdev *io = (struct hkdev*)HKADDR;
    
    do {
        io->hkcs2 = 040;                /* controller clear */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY become 1 */

        io->hkda = 0;                   /* no data */
        io->hkcs2 = 0x401;              /* no operation and set drivetype = RK07 */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY become 1 */

        if ((io->hkcs1 & 0x8000)==0)    /* no error? return */
            break;
    } while ((io->hkcs1 & 0200)==0);    /* loop */
}

hk_open(bp)
register struct iobuf *bp;
{
    register struct hkdev *io = (struct hkdev*)HKADDR;
    int unit;
    int first;

    unit = bp->b_unit;
    first = 0;

retry:
    if (hk_drives[unit] == 0) {         /* has a drive at this unit */
        /* not yet */
        hk_cs1[unit] = 0;               /* try to probe it */
        hk_nblocks[unit] = 26972;       /* #of blocks for RK06 */
        
        io->hkcs2 = 040;                /* controller clear */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY=1 */

        io->hkcs2 = unit;               /* set unit */
        io->hkcs1 = hk_cs1[unit] | 1;   /* select unit + GO */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY=1 */

        if (io->hkcs1 & 0x8000) {       /* drive error? */
            if (io->hkcs2 & 0x1000) {   /* NED error */
                printf("\nHK: Non-Existent Drive %d\n", unit);
                return -1;
            }
            if ((io->hkds & 0x80)==0) { /* not ready? */
                if (!noopenerr && first==0) {
                    printf("Waiting for HK\n");
                    first = 1;
                    goto retry;
                } else {
                    printf("\nHK: Drive %d not ready\n", unit);
                    return -1;
                }
            }                           /* drive is ready */
            if (io->hker & 0x20) {      /* drive type error */
                hk_cs1[unit] = 0x400;   /* set drive type = RK07 */
                hk_nblocks[unit] = 53636; /* #blocks of RK07 */
            } else {
                printf("\nHK: Cannot open unit %d\n", unit);
                return -1;
            }
        }
        
        hk_drives[unit] = 1;            /* drive is present */
    }

    if (hk_hasbad[unit])                /* has badblock table loaded? */
        return;                         /* yes, exit */

    if (!(hk_hasbad[unit] = hk_readbadblk())) { /* read bad block record */
        hk_printerror(2);               /* cannot read bad sector file */
        return -1;
    }

    if (hk_badblktbls[unit].hk_count <= 0x7e) { /* magic okay? */
        printf("HK: Not a valid bad sector file on unit %d\n", unit);
        hk_hasbad[unit] = 0;
        return -1;
    }
    
    /* bug: return undefined value on success */
}

hk_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    
    register struct hkdev *io = (struct hkdev*)HKADDR;
    register int unit;
    register unsigned int nblocks;
    int sector, count, endblk;
    int nbytes, ndone;

    hk_info.unit = bp->b_unit;
    unit = bp->b_unit;
    nblocks = hk_nblocks[unit];
    
    sector = bp->b_sector;
    count = ((bp->b_count + 512)-1) / 512;  /* round up byte count to sectors */
    endblk = sector + count -1;
    if (sector >= nblocks)
        return -1;

    if (nblocks <= endblk)              /* adjust byte count */
        nbytes = (nblocks - sector) * 512; /* if requested I/O exceeds drive size */
    else
        nbytes = bp->b_count;

    bpsecphys(bp);                      /* set DMA addr */
    
    hk_info.cnt = nbytes;
    hk_info.buf = bp->b_dma;
    hk_info.blk = sector;
    if (rwcmd==1)
        hk_info.cmd = 0x10;             /* read cmd */
    else
        hk_info.cmd = 0x12;             /* write cmd */

    for (;;) {
        if (hk_rwsec(hk_info.cnt) >= 0) /* was okay? */
            break;
                                        /* no, failed */
        ndone = (hk_info.bad - hk_info.blk) * 512;
        hk_info.buf += ndone;
        hk_info.cnt -= ndone;
        if (hk_readrepl() == 0) { 
            if (hk_info.cnt >= 0) {
                hk_info.blk = hk_info.bad + 1;
                continue;
            } else
                return nbytes;
        } else {
            hk_printerror(3);           /* no replace sector available */
            return -1;
        }
    }
    return nbytes;
}

int hk_rwsec(nbytes)
int nbytes;
{
    int cyl, sectrk, head, cs1;
    register struct hkdev *io = (struct hkdev*)HKADDR;
    register int i, unit;
    
    for (i=28, unit = hk_info.unit; i-- != 0; ) {
        io->hkcs2 = 0x08;               /* controller clear */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY=1 */

        io->hkcs2 = unit;               /* select unit */
        io->hkcs1 = hk_cs1[unit] | 1;   /* execute cmd with GO */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY=1 */
        
        if ((io->hkds & 0x40)==0) {     /* volume not valid? */
            io->hkcs1 = hk_cs1[unit] | 2 | 1;   /* function PACK | GO */
            hk_hasbad[unit] = 0;
            while ((io->hkcs1 & 0200)==0);  /* wait for CRDY=1 */
        }

        cyl = hk_info.blk / HKSECCYL;   /* # of sec/cyl */
        sectrk = hk_info.blk % HKSECCYL; /* sector in cylinder */
        head = sectrk / HKSECTRK;       /* head# */
        sectrk /= HKSECTRK;             /* sector in track */
        
        io->hkdc = cyl;
        io->hkda = (head<<8) | sectrk;
        io->hkba = hk_info.buf;
        io->hkspr = (hk_info.buf >> 16) | 022000; /* FMT22 bit */
        io->hkwc = -(nbytes / 2);       /* negative # of words */
        cs1 |= (hk_cs1[unit] | (((hk_info.buf >> 16) & 3) << 3) | 
            hk_cs1[unit]) | hk_info.cmd | 1;
        io->hkcs1 = cs1;                /* execute */
        while ((io->hkcs1 & 0200)==0);  /* wait for CRDY=1 */

        if ((io->hkcs1 & 0x8000)==0)    /* no error? yes return */
            return 0;

        if (io->hker & 0x80)            /* error, break */
            break;
    }

    hk_info.bad_cs1 = io->hkcs1;
    hk_info.bad_cs2 = io->hkcs2;
    hk_info.bad_ds = io->hkds;
    hk_info.bad_er = io->hker;
    hk_info.bad = ((io->hkda & 0x700) >> 8) * HKSECTRK +
        io->hkdc * HKSECTRK * HKHEAD +
        (io->hkda & 0x1f);
    return -1;
}

hk_printerror(err)
int err;
{
    printf("HK disk error: Unit=%d sector=%u\n\t       cs1=%4x cs2=%4x ds=%4x err=%4x\n",
        hk_info.unit, hk_info.bad, hk_info.bad_cs1, 
        hk_info.bad_cs2, hk_info.bad_ds, hk_info.bad_er);
    if (err == 2)
        printf("Cannot read bad sector file\n");
    else
        printf("No replace sector available\n");
}

hk_getreplrec(badblkno)
int badblkno;
{
    
    int repl, replidx;

    for (repl = 0; 
            (replidx = hk_bad_replidx(&hk_badblktbls[hk_info.unit], badblkno))< 0;
            badblkno = repl) {
        repl = hk_nblocks[hk_info.unit] + (7*HKSECTRK) - HKSECTRK - 1 -replidx;
        if (badblkno == repl)
            return 0;
    }
    return repl;
}

/* read replacement record for a given block */
int hk_readrepl(blk)
int blk;
{
    int repl, bread;
    if ((repl = hk_getreplrec(hk_info.bad)) < 0)
        return -1;                      /* no replacement found */
    
    hk_info.blk = repl;                 /* replacement sector */
    bread = hk_info.cnt < 512 ? hk_info.cnt : 512;
    
    if (hk_rwsec(bread) < 0)            /* failed to read/write? */
        return -1;
    
    hk_info.buf += bread;
    hk_info.cnt -= bread;
    return 0;
}

/* read badblktabl */
hk_readbadblk()
{
    int i;

    hk_info.buf = phys_offset(hk_badblktbls[hk_info.unit]);
    hk_info.cmd = 0x10;                  /* read cmd */
    
    for (i = 0; i < 5; i++) {
        hk_info.blk =
            hk_nblocks[hk_info.unit] + (7*HKSECTRK) - HKSECTRK + (i<<2);
        if (hk_rwsec(512)==0)           /* try to red sector */
            return 1;                   /* successful */
    }
    return 0;
}

int hk_bad_replidx(tp, blkno)
register struct hkbad *tp;
int blkno;
{
    register int i;

    for (i=0; i< tp->hk_count; i++) {
        if (blkno == tp->hk_badblk[i])
            return i;
    }
    return -1;
}
