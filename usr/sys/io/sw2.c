/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.2  Aug 19 1986 /usr/sys/io/sw2.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/buf.h"
#include "sys/ino.h"
#include "sys/inode.h"
#include "sys/mount.h"
#include "sys/conf.h"
#include "sys/proc.h"
#include "sys/seg.h"
#include "sys/var.h"
#include "sys/fssizes.h"
#include "sys/sw2.h"

struct buf sw2utab[NSW];
struct buf sw2cbuf;
struct buf sw2rbuf;

static short sw2timeorun = 0;
static short sw2timo = -1;
int sw2timeostat = 0;

static short sw2fmt[NSW];
static struct sw2_badblockinfo sw2_badblockinfo[NSW];
static struct sw2_sechdr sw2_hdr[NSW];

struct buf sw2tab;

/* partition data */
extern struct fs_sizes sw_sizes[];

extern sw2timeo();
extern short fuword();

sw2delay() {
    int i;
    for (i=0; i < 5; i++);
}

/* rw=1 is read, rw=0 is write */
ushort sw2_eepromio(adr, data, rw)
ushort adr, data;
{
    register struct device *dp = SWADDR;
    register short i;
    register ushort ret;

    spldisk();
    for (i=0; sw2tab.b_bcount != 0; ) {
        if (i++ > 50) {
            u.u_error = EAGAIN;
            spl0();
            return -1;
        }
        delay(1);
    }

    if (rw == 1) {
        dp->sw2wc = adr;
        dp->sw2cs1 = RDPROM|GO;
        while ((dp->sw2cs1 & CRDY) == 0);
        ret = dp->sw2db & 255;
    } else {
        dp->sw2wc = adr;
        dp->sw2db = data & 255;
        dp->sw2cs1 = WTPROM|GO;
        while ((dp->sw2cs1 & CRDY) == 0);
        dp->sw2cs1 = WTPROM|GO;
        while ((dp->sw2cs1 & CRDY) == 0);
        ret = 0;
    }
    
    if ((dp->sw2er1 & ILF) != 0) {
        dp->sw2cs2 = CCLR;
        while ((dp->sw2cs1 & CRDY) == 0);
        ret = -1;
        u.u_error = EIO;
    }
    spl0();
    return ret;
}

/* verbose != 0 will report drive size */
sw2_driveinit(drv, verbose)
short drv, verbose;
{
    register struct device *dp = SWADDR;

    sw2_r[drv].ntry = NUMTRY;
    sw2_r[drv].stat = 0;

    spldisk();
    dp->sw2cs2 = drv;
    dp->sw2cs1 = DCLR|GO;
    while((dp->sw2cs1 & CRDY) == 0);

    dp->sw2of |= FMT22;
    dp->sw2cs1 = PACK|GO;
    while((dp->sw2cs1 & CRDY) == 0);

    if ((dp->sw2ds & (MOL|VV)) != (MOL|VV)) {
        spl0();
        return -1;
    }

    sw2_r[drv].stat |= SW_INIT;
    spl0();

    if (!verbose)
        return 0;

    sw2_badblkio(drv, 1);
    if (sw2_badblockinfo[drv].sw2_magic != SWMAGIC)
        printf("SW2 init: WARNING wrong initialisation on drive %d\n",
            drv);
    else
        printf("SW2 disk drive %d has %d MB capacity\n", drv,
            (int)sw2_r[drv].nblks * (int)sw2_r[drv].nhead * 
            (int)sw2_r[drv].ncyl / 2000);
    return 0;
}


short sw2_controllerinit()
{
    register struct device *dp = SWADDR;
    register short par, drv, h;
    short res;
    
    if (fsword(SWADDR) == -1) {
        u.u_error = ENXIO;
        return -1;
    }

    dp->sw2cs2 = CCLR;
    while ((dp->sw2cs1 & CRDY) == 0);
    
    dp->sw2of = FMT22;
    res = sw2_eepromio(BOOTPROMSTART, 0, 1);
    if (res == -1) {
        for (drv=0; drv < NSW; drv++) {
            dp->sw2cs2 = drv;
            SWSECTRK(drv) = RM_NSEC;
            SWCYL(drv)    = RM_NCYLS;
            switch (dp->sw2dt & 255) {
            case 20:
            case 21:
                SWTRKCYL(drv) = RM_2TRK;
                break;
            case 23:
                SWTRKCYL(drv) = RM_5TRK;
                break;
            default:
                SWTRKCYL(drv) = 0;
            }
        }
        /* BUG: returns here without a valid return value! */
    } else {
        res <<= 8;
        res |= sw2_eepromio(BOOTPROMSTART+1, 0, 1);
        if (res != SWMAGIC) {
            printf("SW2 init: warning! wrong eeprom initialisation\n");
            return -1;
        }
        for (drv=0; drv < NSW; drv++) {
            par = sw2_eepromio(drv+PARBLK_LOC, 0, 1);
            SWSECTRK(drv) = sw2_eepromio(par+LSPLH_LOC,    0, 1);
            SWTRKCYL(drv) = sw2_eepromio(par+LHPLC_LOC,  0, 1);
            SWCYL(drv)    = sw2_eepromio(par+LCPLDL_LOC, 0, 1);
            h =             sw2_eepromio(par+LCPLDH_LOC, 0, 1);
            h <<= 8;
            SWCYL(drv)   |=  h;
        }
        return 0;
    }
}

sw2init()
{
    register short drv;

    if (sw2_controllerinit() >= 0) {
        for (drv=0; drv < NSW; drv++)
            sw2_driveinit(drv, 1);
    }
    u.u_error = 0;
}

sw2strategy(bp)
register struct buf *bp;
{
    register struct buf *bp2;
    register struct device *dp = SWADDR;
    register cnt;
    register blkno;
    register ushort dev = physical(bp->b_dev);
    short lun;
    int maxblks;
    
    if (dev < 2 && (sw2_r[dev].stat & SW_INIT)==0)
        sw2_driveinit(dev, (short)((sw2_r[dev].stat & SW_SYSIO) != 0));

    lun = fsys(bp->b_dev);

    cnt = bp->b_bcount;
    if ((bp->b_flags & SWHDIO)==0)
        cnt = (cnt + (SWSECSIZ-1)) / SWSECSIZ;
    else
        cnt = (cnt + (SWHDRSIZ-1)) / SWHDRSIZ;

    maxblks = SW_SECTOT(dev) - (SW_SPARE + SWBADBLOCKOFFSET);
    blkno = bp->b_blkno;
    if (dev >= 2 || (sw2_r[dev].stat & SW_INIT) == 0 ||
        ((sw2_r[dev].stat & SW_SYSIO) == 0 && 
         (blkno < 0 ||
          ((sw2_r[dev].stat & SW_SPECIAL) == 0 &&
           ((blkno+cnt) > sw_sizes[lun].nblocks || (blkno+cnt) > maxblks))
         )
        )
       ) {
        u.u_error = ENXIO;
        bp->b_flags |= B_ERROR;
        iodone(bp);
        return;
    }

    blkno += SWBADBLOCKOFFSET;
    if (sw2_r[dev].stat & SW_SYSIO)
        bp->b_cylin = CYLADDR(blkno, dev);
    else
        bp->b_cylin = CYLADDR(blkno + sw_sizes[lun].offset, dev);

    bp2 = &sw2utab[dev];
    spldisk();

      newsort(bp2, bp, sw_sizes);
      if (bp2->b_bcount == 0) {
          sw2ustart(dev);
          if (sw2tab.b_bcount == 0)
              sw2start();
      }
    spl0();
}

sw2ustart(dev)
register ushort dev;
{
    register struct device *dp = SWADDR;
    register struct buf *bp;
    register struct buf *np;
    register short trksec;

    if (dev >= 2)
        return;

    dp->sw2cs2 = dev;
    dp->sw2as = (1 << dev);
    if ((dp->sw2ds & (MOL|DPR|VV)) != (MOL|DPR|VV))
        sw2_r[dev].stat &= ~SW_INIT;

    bp = &sw2utab[dev];
    if (sw2fmt[dev]) {
        wakeup(&sw2fmt[dev]);
        bp->b_bcount = 0;
        sw2fmt[dev] = 0;
    }
    
    np = bp->av_forw;
    if (np == 0)
        return;

    if (bp->b_bcount == 0) {
        bp->b_bcount++;
        if (sw2_r[dev].stat & SW_SYSIO)
            sw2_r[dev].sn = np->b_blkno + SWBADBLOCKOFFSET;
        else
            sw2_r[dev].sn = np->b_blkno + 
                sw_sizes[fsys(np->b_dev)].offset + SWBADBLOCKOFFSET;
        
        trksec = TRKSEC(sw2_r[dev].sn, dev);
        sw2_r[dev].addr = np->b_paddr;
        sw2_r[dev].rblks = trksec % SWSECTRK(dev);
        sw2_r[dev].rtrk  = trksec / SWSECTRK(dev);
        sw2_r[dev].nbtrans = np->b_bcount;
        if ((np->b_flags & SWHDIO)==0) {
            if (np->b_flags & B_READ)
                sw2_r[dev].command = RCOM;
            else
                sw2_r[dev].command = WCOM;
        } else {
            if (np->b_flags & B_READ)
                sw2_r[dev].command = RHDR;
            else
                sw2_r[dev].command = WHDR;
        }
        sw2_r[dev].rcyl = np->b_cylin;
    }
    
    bp->b_forw = 0;
    if (sw2tab.av_forw == 0)
        sw2tab.av_forw = bp;
    else
        sw2tab.av_back->b_forw = bp;
    
    sw2tab.av_back = bp;
    spltimer();
    sw2timo = 3;
}

sw2start()
{
    register struct device *dp = SWADDR;
    register struct buf *bp;
    register struct buf *np;
    register ushort dev;
    register ushort cmd;

loop:
    bp = sw2tab.av_forw;
    if (bp == 0) return;
            
    np = bp->av_forw;
    if (np == 0) {
        sw2tab.av_forw = bp->b_forw;
        goto loop;
    } else {
        sw2tab.b_bcount++;
        dev = physical(np->b_dev);
        dp->sw2cs2 = dev;

        if ((dp->sw2ds & (MOL|DPR|VV)) != (MOL|DPR|VV)) {
            printf("sw2start: drive %d off-line - error\n", dev);
            sw2_r[dev].stat = 0;
            sw2tab.b_bcount = 0;
            sw2tab.b_cylin = 0;
            bp->av_forw = np->av_forw;
            np->b_flags |= B_ERROR;
            iodone(np);
            goto loop;
        }
    }

    if (sw2_r[dev].stat & SW_SPECIAL)
        dp->sw2of |= ECI;
    else
        dp->sw2of &= ~ECI;

    if (sw2tab.b_cylin >= 8) {
        if ((sw2tab.b_cylin & 1) != 0)
            dp->sw2of |= (FMT22|OFFDIR);
        else
            dp->sw2of &= ~OFFDIR;

        dp->sw2cs1 = OFFSET|GO;
        while ((dp->sw2ds & PIP) != 0);
    }

    dp->sw2dc = sw2_r[dev].rcyl;
    dp->sw2da = (sw2_r[dev].rtrk << 8) | sw2_r[dev].rblks;
    dp->sw2ba  = sw2_r[dev].addr;
    dp->sw2bae = sw2_r[dev].addr >> 16;
    dp->sw2wc = -(sw2_r[dev].nbtrans >> 1);
    cmd = (((sw2_r[dev].addr >> 16) & 03) << 8) | /*bits 9-8 of swcs1 */
            sw2_r[dev].command | (IE|GO);
    dp->sw2cs1 = cmd;
}

sw2intr()
{
    register struct device *dp = SWADDR;
    register struct buf *bp;
    register struct buf *np;
    register ushort dev;
    short sw2as;
    ushort sw2er2; /* set but unused */
    int altsec = 0;
    int errsec;
    ushort lsec;

    spldisk();
    sw2timo = 0;
    sw2as = dp->sw2as & 0xff;
     
    if (sw2tab.b_bcount != 0) {
        bp = sw2tab.av_forw;
        np = bp->av_forw;
        dev = physical(np->b_dev);
        dp->sw2cs2 = dev;

        sw2er2 = dp->sw2er2;
        if (dp->sw2cs1 & TRE) {
            while ((dp->sw2ds & DRY)==0);
            if (sw2tab.b_cylin == 0)
                sw2error(np, dev, 0);
            errsec = ((dp->sw2dc * (int)sw2_r[dev].nhead) + 
                    ((dp->sw2da>>8) & 0xff)) *
                (int)sw2_r[dev].nblks + (dp->sw2da & 0xff) - 1;
                
            altsec = sw2_getaltsec(dev, errsec);
            
            if ((dp->sw2er1 & (WLE|IAE)) ||
                (dp->sw2cs2 & NEM) ||
                ((sw2_r[dev].ntry==0 && (np->b_flags & SWHDIO)==0) ?
                    altsec==0 :
                     sw2tab.b_cylin++ >= sw2_r[dev].ntry) )
                np->b_flags |= B_ERROR;
            else
                sw2tab.b_bcount = 0;

            dp->sw2cs1 = DCLR|GO;
            while ((dp->sw2cs1 & DRY)==0);
        }
        
        if (altsec != 0 && (np->b_flags & SWHDIO)==0) {
            sw2tab.b_bcount = 0;
            sw2tab.b_cylin = 0;
            sw2_r[dev].stat |= SW_ALTIO;
            lsec = errsec - sw2_r[dev].sn;
            if ((((ushort)(lsec + 1)) * SWSECSIZ) < sw2_r[dev].nbtrans) {
                sw2_r[dev].bleft = sw2_r[dev].nbtrans - (lsec + 1) * SWSECSIZ;
                sw2_r[dev].nbtrans = SWSECSIZ;
            } else {
                sw2_r[dev].bleft = 0;
                sw2_r[dev].nbtrans -= lsec * SWSECSIZ;
            }

            sw2_r[dev].addr = lsec * SWSECSIZ + sw2_r[dev].addr;
            sw2_r[dev].saddr = sw2_r[dev].addr + SWSECSIZ;
            sw2_r[dev].sblk = errsec+1;
            
            altsec += 2;
            sw2_r[dev].rcyl = CYLADDR(altsec, dev);
            altsec = TRKSEC(altsec, dev);
            sw2_r[dev].rblks = altsec % sw2_r[dev].nblks;
            sw2_r[dev].rtrk = altsec / sw2_r[dev].nblks;
        }

        if (sw2tab.b_bcount != 0) {
            if (sw2tab.b_cylin != 0 || (np->b_flags & B_ERROR)) {
                dp->sw2cs1 = RTC|GO;
                while ((dp->sw2cs1 & 128)==0);
                sw2tab.b_cylin++;
                sw2error(np, dev, (short)sw2tab.b_cylin);
            }
            sw2tab.b_bcount = 0;
            sw2tab.b_cylin = 0;
            if (sw2_r[dev].stat & SW_ALTIO) {
                sw2_r[dev].stat &= ~SW_ALTIO;
                if ((np->b_flags & B_ERROR)==0 && sw2_r[dev].bleft != 0) {
                    sw2_r[dev].sn = sw2_r[dev].sblk;
                    sw2_r[dev].addr = sw2_r[dev].saddr;
                    sw2_r[dev].nbtrans = sw2_r[dev].bleft;
                    sw2_r[dev].rcyl = CYLADDR(sw2_r[dev].sblk, dev);
                    altsec = TRKSEC(sw2_r[dev].sblk, dev);
                    sw2_r[dev].rblks = altsec % sw2_r[dev].nblks;
                    sw2_r[dev].rtrk = altsec / sw2_r[dev].nblks;
                    goto retry;
                }
            }

            sw2tab.av_forw = bp->b_forw;
            bp->b_bcount = 0;
            bp->b_cylin = 0;
            bp->av_forw = np->av_forw;
            if (np->b_flags & B_ERROR)
                np->b_cylin = np->b_bcount;
            else
                np->b_cylin = 0;
            
            iodone(np);
            if (bp->av_forw)
                sw2ustart(dev);
        }
        sw2as &= ~(1<<dev);
    } else {
        for (dev=0; dev < NSW; dev++) {
            if (sw2fmt[dev] != 0)
                sw2as |= (1 << dev);
        }
    }

retry:
    for (dev=0; dev < NSW; dev++) {
        if ((sw2as & (1 << dev)) != 0 &&
            ((sw2_r[dev].stat & 1) || sw2fmt[dev]))
          sw2ustart(dev);
    }

    sw2start();
}


sw2error(bp, dev, ntry)
register struct buf *bp;
register ushort dev;
register short ntry;
{
    register struct device *dp = SWADDR;

    if (ntry-- == 0) {
        sw2_r[dev].cs1 = dp->sw2cs1;
        sw2_r[dev].cs2 = dp->sw2cs2;
        sw2_r[dev].ds  = dp->sw2ds;
        sw2_r[dev].er1 = dp->sw2er1;
        sw2_r[dev].er2 = dp->sw2er2;
        sw2_r[dev].wc  = dp->sw2wc;
        sw2_r[dev].ba  = dp->sw2ba;
        sw2_r[dev].da  = dp->sw2da;
        sw2_r[dev].dc  = dp->sw2dc;
        sw2_r[dev].bae = dp->sw2bae;
        sw2_r[dev].badsn = (dp->sw2dc * (int)sw2_r[dev].nhead + 
            ((dp->sw2da >> 8) & 0xff)) * (int)sw2_r[dev].nblks + 
            (dp->sw2da & 0xff) - 1;
        return;
    }

    if ((bp->b_flags & B_ERROR) != 0 || ntry >= sw2_r[dev].ntry)
        printf("\nsw2: fatal ");
    else
        printf("\nsw2: recoverable ");

    printf("SW2 disk error: drive=%d  physical sector=%ld (+SWBADBLOCKOFFSET)\n",
        dev, sw2_r[dev].badsn-2);
    printf("Error Count = %d\n", ntry);

    if ((sw2_r[dev].ds & (MOL|DPR|VV)) != (MOL|DPR|VV))
        printf("SW2: drive is offline\n");

    if (sw2_r[dev].er1 & WLE)
        printf("SW2: drive is write protected\n");

    if (sw2_r[dev].er1 & (HCRC|HCE))
        printf("SW2: could not read header of sector\n");

    if ((sw2_r[dev].er1 & (DCK|DTE|ECH)) || (sw2_r[dev].er2 & BSE))
        printf("SW2: sector is bad\n");

    printf("cs1=%4x cs2=%4x ds=%4x er1=%4x er2=%4x\n",
        sw2_r[dev].cs1, sw2_r[dev].cs2, sw2_r[dev].ds, 
        sw2_r[dev].er1, sw2_r[dev].er2);
}

sw2_iohdr(devno, flags, func)
short devno;
{
    register short dev = physical(devno);
    register struct buf *bp = &sw2cbuf;

    spldisk();
      while (bp->b_flags & B_BUSY) {
          bp->b_flags |= B_WANTED;
          sleep(bp, PRIBIO+1);
      }
    spl0();
    
    bp->b_flags = flags | (SWHDIO|B_BUSY|B_PHYS);
    bp->b_dev = devno;
    bp->b_paddr = logtophys(&sw2_hdr[dev]);
    bp->b_bcount = SWHDRSIZ;
    bp->b_error = 0;
    bp->b_blkno = sw2_hdr[dev].un_sw2.sw2_sec;
    if (func == SW_HDRVAL) {
        sw2_hdr[dev].un_sw2.sw2_blkhdr.sw2_flgcyl = 
            CYLADDR(bp->b_blkno+SWBADBLOCKOFFSET, dev);
        sw2_hdr[dev].un_sw2.sw2_blkhdr.sw2_flgcyl |= 0150000; /* which flags? */

#define TRSCD (TRKSEC(bp->b_blkno+SWBADBLOCKOFFSET, dev) / sw2_r[dev].nblks)
#define TRSCM (TRKSEC(bp->b_blkno+SWBADBLOCKOFFSET, dev) % sw2_r[dev].nblks)
        sw2_hdr[dev].un_sw2.sw2_blkhdr.sw2_trksec = 
            ((TRSCD & 0xff) << 8) | (TRSCM & 0xFF);
    } else if (func == SW_HDRINVAL) {
        sw2_hdr[dev].un_sw2.sw2_blkhdr.sw2_flgcyl = 0;
        sw2_hdr[dev].un_sw2.sw2_blkhdr.sw2_trksec = 0;
    }

    spldisk();
      sw2strategy(bp);
      while ((bp->b_flags & B_DONE)==0)
          sleep(bp, PRIBIO);
    spl0();
    
    if ((bp->b_flags & B_ERROR) && (u.u_error=bp->b_error)==0)
        u.u_error = EIO;

    if (bp->b_flags & B_WANTED)
        wakeup(bp);

    bp->b_flags &= ~(B_WANTED|B_BUSY);
}

sw2open(devno)
short devno;
{
    register struct device *dp = SWADDR;
    ushort dev = physical(devno);

    if (fsword(SWADDR) == -1 || dev >= 2) {
        u.u_error = ENXIO;
        return;
    }

    spldisk();
      dp->sw2cs2 = dev;
      while ((dp->sw2cs1 & CRDY)==0);

      if ((dp->sw2ds & (MOL|DPR|VV)) != (MOL|DPR|VV))
          sw2_r[dev].stat = 0;
    spl0();

    if ((sw2_r[dev].stat & SW_INIT)==0) {
        sw2_driveinit(dev, 1);
        if (u.u_error != 0 && suser()) {
            u.u_error = 0;
            printf("sw2open: Warning drive %d  not ready\n", dev);
        }
    }

    if (sw2timeorun == 0) {
        sw2timeorun++;
        timeout(sw2timeo, &sw2timo, hz);
    }
}

sw2ioctl(devno, func, arg)
register short devno;
register caddr_t arg;
{
    register struct device *dp = SWADDR;
    register ushort dev;
    struct buf *sw2up;
    register struct sw2ctl *ap1 = (struct sw2ctl*)arg;
    struct sw2_badblockinfo *ap2 = (struct sw2_badblockinfo*)arg;
    struct sw2_sechdr *ap3 = (struct sw2_sechdr*)arg;
    struct sw2_conf *ap4 = (struct sw2_conf*)arg;
    int i;

    if (suser()==0)
        return;

    dev = physical(devno);
    switch (func) {
    case SW_FORMAT:
        spldisk();
          sw2up = &sw2utab[dev];
          while (sw2up->b_bcount)
              sleep(sw2up->av_forw, PRIBIO);
        
          sw2up->b_bcount++;

          dp->sw2cs2 = dev;
          dp->sw2cs1 = DCLR|GO;
          while ((dp->sw2cs1 & CRDY)==0);

          dp->sw2cs2 = dev;
          dp->sw2cs1 = PACK|GO;
          while ((dp->sw2cs1 & CRDY)==0);

          dp->sw2cs2 = dev;
          dp->sw2cs1 = PRESET|GO;
          while ((dp->sw2cs1 & CRDY)==0);

          dp->sw2as = 1 << dev;
          if ((dp->sw2ds & (MOL|DPR|VV)) != (MOL|DPR|VV)) {
            u.u_error = EIO;
            sw2up->b_bcount = 0;
            spl0();
            return;
          }

          dp->sw2of = FMT22;
          dp->sw2cs1 = (IE|FORMAT|GO);
          sw2fmt[dev]++;
          sleep(&sw2fmt[dev], PRIBIO);

          dp->sw2cs2 = dev;
          dp->sw2cs1 = (DCLR|GO);
          while ((dp->sw2cs1 & CRDY)==0);
          
          sw2up->b_bcount = 0;
        spl0();
        break;

    case SW_REEPROM:
        suword(&ap1->sw2_data, sw2_eepromio(fuword(ap1), 0, 1));
        break;
    
    case SW_WEEPROM:
        sw2_eepromio(fuword(ap1), fuword(&ap1->sw2_data), 0);
        break;

    case SW_RSTPROM:
        spldisk();
          ssword(dp, RSPROM|GO);
          printf("SW2: reinitializing Controller\n");
          for (i=0; i < 20000; i++);
          while ((dp->sw2cs1 & CRDY)==0);
          dp->sw2cs2 |= CCLR;
          dp->sw2of = FMT22;
        spl0();
        
        for (dev=0; dev < NSW; dev++)
            sw2_r[dev].stat = 0;
        break;

    case SW_READBAD:
        copyout(&sw2_badblockinfo[dev], ap2, sizeof(struct sw2_badblockinfo));
        break;
        
    case SW_WRITEBAD:
        if (copyin(ap2, &sw2_badblockinfo[dev], 
                sizeof(struct sw2_badblockinfo)) < 0) {
            u.u_error = EIO;
            return;
        }
        sw2_badblkio(dev, B_WRITE);
        break;
        
    case SW_CLEAR:
        spldisk();
          dp->sw2cs2 = CCLR;
        spl0();
        break;

    case SW_HDRVAL:
    case SW_HDRINVAL:
        if (copyin(ap3, &sw2_hdr[dev], sizeof(struct sw2_sechdr)) < 0) {
            u.u_error = EIO;
            return;
        }
        sw2_iohdr(devno, B_WRITE, func);
        break;
        
    case SW_READHDR:
        if (copyin(ap3, &sw2_hdr[dev], sizeof(struct sw2_sechdr)) < 0) {
            u.u_error = EIO;
            return;
        }
        sw2_iohdr(devno, B_READ, func);
        copyout(&sw2_hdr[dev], ap3, sizeof(struct sw2_sechdr));
        break;
    
    case SW_REINIT:
        sw2_driveinit(dev, 1);
        break;

    case SW_CONTRINIT:
        sw2_controllerinit();
        break;
        
    case SW_NORETRY_AND_ECC:
        sw2_r[dev].stat |= SW_SPECIAL;
        sw2_r[dev].ntry = 0;
        break;

    case SW_GETCONF:
        copyout(&sw2_r[dev], ap4, sizeof(struct sw2_conf));
        break;
        
    default:
        u.u_error = EINVAL;
        break;
    }
}

/* 0=write, 1=read */
sw2_badblkio(dev, rw)
short dev;
{
    register struct buf *bp = &sw2cbuf;

    spldisk();
      while (bp->b_flags & B_BUSY) {
          bp->b_flags |= B_WANTED;
          sleep(bp, PRIBIO+1);
      }
    spl0();

    bp->b_flags = rw | (B_BUSY|B_PHYS);
    bp->b_dev = dev << 4;
    bp->b_paddr = logtophys(&sw2_badblockinfo[dev]);
    bp->b_blkno = -SWBADBLOCKOFFSET;
    bp->b_bcount = sizeof(struct sw2_badblockinfo);
    bp->b_error = 0;
    spldisk();
      sw2_r[dev].stat |= SW_SYSIO;
      sw2strategy(bp);
      sw2_r[dev].stat &= ~SW_SYSIO;
      while ((bp->b_flags & B_DONE)==0)
          sleep(bp, PRIBIO);
    spl0();
    
    if (bp->b_flags & B_ERROR && (u.u_error=bp->b_error)==0)
        u.u_error = EIO;

    if (bp->b_flags & B_WANTED)
        wakeup(bp);

    bp->b_flags &= ~(B_WANTED|B_BUSY);
}


sw2_getaltsec(dev, sec)
ushort dev;
register sec;
{
    register struct sw2_badblockinfo *bbp = &sw2_badblockinfo[dev];
    register short i;

    if (sec != 0) {
        for (i=0; i < MAXBADSEC; i++) {
            if ((sec-SWBADBLOCKOFFSET) == bbp->sw2_log_bad[i])
                return bbp->sw2_log_alter[i];
        }
    }
    return 0;
}

/* C function */
sw2read(dev)
{
    physio(sw2strategy, &sw2rbuf, dev, 1);
}

/* C function */
sw2write(dev)
{
    physio(sw2strategy, &sw2rbuf, dev, 0);
}

sw2timeo()
{
    if (sw2timo >= 0)
        sw2timo--;

    if (sw2timo == 0 && sw2tab.b_bcount) {
        printf("SW: interrupt timeout\n");
        sw2timeostat++;
        sw2intr();
    }

    timeout(sw2timeo, &sw2timo, hz);
}
