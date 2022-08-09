#include "data.h"
#include <sys/sw.h>

/*
 * emulex/spectra RK06/07 controller
 */

static int sw_unit;
static int sw_b_cylin;
static struct sw_badblockinfo sw_badblkl;
static short sw_lsplh;
static short sw_lhplc;
static short sw_lcpldl;

extern short sw_eeprom_read();

int sw_open(bp)
register struct iobuf *bp;
{
    register struct device *io = SWADDR;
    register short res;
    register int parblk;
    int unit;
    int dummy[2];                       /* unused */
    int nowaitmsg;                      /* suppress wait on 2nd.. pass */

    dummy[0] = 0;                       /* unused */
    nowaitmsg = 0;                      

    sw_unit = bp->b_unit;
    unit = bp->b_unit;

    for (;;) {
        io->swcs2 = unit;               /* select unit */
        io->swcs1 = DCLR|GO;            /* drive clear */
        while ((io->swcs1 & CRDY)==0);  /* wait for ready */

        io->swof = FMT22;               /* 22 bit address mode */
        io->swcs1 = PACK|GO;            /* pack acknowledge */
        while ((io->swcs1 & CRDY)==0);  /* wait for ready */

        if ((io->swds & (MOL|VV)) == (MOL|VV))  /* is online */
            break;

        if (!noopenerr) {               /* print, if at all, wait msg only once */
            if (!nowaitmsg) {
                printf("Waiting for SW\n");
                nowaitmsg++;
            }
        } else {
            printf("SW: drive %d not ready\n"); /* BUG! no %d arg! */
            return -1;
        }
    }
    
    io->swof = FMT22;                   /* 22 bit address mode */
    io->swcs1 = PRESET|GO;              /* read-in-preset */
    while ((io->swcs1 & CRDY)==0);      /* wait for ready */

    res = sw_eeprom_read(BOOTPROMSTART); /* read from EEPROM */
    res <<= 8;
    res |= sw_eeprom_read(BOOTPROMSTART+1);
    
    if (res != SWMAGIC) {
        if (!noopenerr)
            printf("no spectra\n");
        return -1;
    }

    parblk = sw_eeprom_read(unit+PARBLK_LOC);   /* get parameters for unit */
    sw_lsplh = sw_eeprom_read(parblk+LSPLH_LOC);
    sw_lhplc = sw_eeprom_read(parblk+LHPLC_LOC);
    sw_lcpldl = sw_eeprom_read(parblk+LCPLDL_LOC);
    sw_lcpldl |= (sw_eeprom_read(parblk+LCPLDH_LOC)<<8);
    
    io->swcs2 = bp->b_unit;
    sw_readwrite(SWBADBLOCKOFFSET, phys_offset(&sw_badblkl), 512, 1);
    if (sw_badblkl.sw_magic != SWMAGIC)
        printf("SW not initialized\n");
    sw_readwrite(SWBADBLOCKOFFSET+1,    /* read second half of badblock table */
        phys_offset(&sw_badblkl.sw_log_bad[MAXBADSEC-1]), 512, 1);
    return 0;
}

int sw_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    register struct device *io = SWADDR;
    int sec, sector, dmaaddr, count, cnt;

    if (bp->b_unit != sw_unit)
        sw_open(bp);

    bpsecbuf_phys(bp);                  /* set b_dma */
    
    dmaaddr = bp->b_dma;
    sector = bp->b_sector;
    count = bp->b_count;
    while (count > 0) {
        io->swcs2 = bp->b_unit;
        if (!(io->swds & VV)) {         /* volume valid? */
            io->swof = SKPENB;
            io->swcs1 = PRESET|GO;
            while ((io->swcs1 & CRDY)==0); /* wait for ready */
        }

        if (sw_b_cylin) {               /* valid I/O? */
            cnt = count;
            sec = sector;
        } else {                        /* no, need replacement block */
            cnt = count > 512 ? 512 : count;
            sec = sw_getaltsec(sector);
        }
        
        if (sw_readwrite(sec, dmaaddr, cnt, rwcmd) == -1)
            return -1;                  /* I/O error */
        
        count -= cnt;                   /* advance to next sector */
        sector++;
        dmaaddr + = count;
    }
    return bp->b_count;
}

short sw_eeprom_read(addr)
int addr;
{
    register struct device *io = SWADDR;

    io->swwc = addr;
    io->swcs1 = RDPROM|GO;              /* read EEPROM */
    while ((io->swcs1 & CRDY)==0);      /* wait for ready */
    return io->swdb & 0xff;
}

int sw_getaltsec(sec)
register int sec;
{
    register daddr_t *p = sw_badblkl.sw_log_bad;
    register int i;
    
    if (sec==0) return sec;

    for (i=0; i < MAXBADSEC; i++) {
        if (sec == *p) {
            p += 127;                   /* points to sw_log_alter[i] */
            return *p;
        }
        p++;
    }
    
    return sec;                         /* no alternative sector */
}

int sw_readwrite(sec, buf, cnt, cmd)
register int sec;
register char *buf;
int cnt, cmd;
{
    register struct device *io = SWADDR;
    register short rblks, rcyl, rtrk;
    int rwcmd;

    sec += 2;                           /* skip over badblock sectors */
    rcyl = sec / (sw_lsplh * sw_lhplc); /* cylinder */
    rblks = sec % (sw_lsplh * sw_lhplc); /* sector in cylinder */
    rtrk = rblks / sw_lsplh;            /* head (track) */
    rblks = rblks % sw_lsplh;           /* sector in track */

    if (sw_b_cylin)
        io->swof = SKPENB|OFFDIR;       /* towards spindle */
    else
        io->swof = SKPENB;              /* away rom spindle */
    io->swdc = rcyl;
    io->swda = (rtrk << 8) + rblks;
    io->swba = (short)buf;
    io->swbae= ((int)buf) >> 16;
    io->swwc = -(cnt>>1);               /* negative size in words */
    if (cmd & 0x10000) {                /* 0x10000 = r/w with header and data format */
        cmd &= ~0x10000;
        rwcmd = cmd==1 ? RHDR : WHDR;
    } else {
        rwcmd = cmd==1 ? RCOM : WCOM;
    }
    
    io->swcs1 = (((((int)buf) >> 16) & 3) << 8) | GO | rwcmd;    /* execute */
    while ((io->swcs1 & CRDY)==0);      /* wait for ready */
    if (io->swcs1 & TRE) {              /* error? */
        printf("SW disk error: sector=%ld\ncs1=%4x cs2=%4x ds=%4x er1=%4x er2=%4x\n",
            sec, io->swcs1, io->swcs2, io->swds, io->swer1, io->swer2);
        return -1;
    }
    return 0;
}
