/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.2  Aug 19 1986 /usr/sys/io/sw.c ";

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
#include "sys/sw.h"

struct buf swutab[NSW];
struct buf swcbuf;
struct buf swrbuf;

static short swtimeorun = 0;
static short swtimo = -1;
int swtimeostat = 0;

static short swfmt[NSW];
static struct sw_badblockinfo sw_badblockinfo[NSW];
static struct sw_sechdr sw_hdr[NSW];

struct buf swtab;

/* partition data */
extern struct fs_sizes sw_sizes[];

extern swtimeo();
extern short fuword();

swdelay() {
    int i;
    for (i=0; i < 5; i++);
}

/* rw=1 is read, rw=0 is write */
ushort sw_eepromio(adr, data, rw)
ushort adr, data;
{
    register struct device *dp = SWADDR;
    register short i;
    register ushort ret;

    spldisk();
    for (i=0; swtab.b_bcount != 0; ) {
        if (i++ > 50) {
            u.u_error = EAGAIN;
            spl0();
            return -1;
        }
        delay(1);
    }

    if (rw == 1) {
        dp->swwc = adr;
        dp->swcs1 = RDPROM|GO;
        while ((dp->swcs1 & CRDY) == 0);
        ret = dp->swdb & 255;
    } else {
        dp->swwc = adr;
        dp->swdb = data & 255;
        dp->swcs1 = WTPROM|GO;
        while ((dp->swcs1 & CRDY) == 0);
        dp->swcs1 = WTPROM|GO;
        while ((dp->swcs1 & CRDY) == 0);
        ret = 0;
    }
    
    if ((dp->swer1 & ILF) != 0) {
        dp->swcs2 = CCLR;
        while ((dp->swcs1 & CRDY) == 0);
        ret = -1;
        u.u_error = EIO;
    }
    spl0();
    return ret;
}

/* verbose != 0 will report drive size */
sw_driveinit(drv, verbose)
short drv, verbose;
{
    register struct device *dp = SWADDR;

    sw_r[drv].ntry = NUMTRY;
    sw_r[drv].stat = 0;

    spldisk();
    dp->swcs2 = drv;
    dp->swcs1 = DCLR|GO;
    while((dp->swcs1 & CRDY) == 0);

    dp->swof |= FMT22;
    dp->swcs1 = PACK|GO;
    while((dp->swcs1 & CRDY) == 0);

    if ((dp->swds & (MOL|VV)) != (MOL|VV)) {
        spl0();
        return -1;
    }

    sw_r[drv].stat |= SW_INIT;
    spl0();

    if (!verbose)
        return 0;

    sw_badblkio(drv, 1);
    if (sw_badblockinfo[drv].sw_magic != SWMAGIC)
        printf("SW init: WARNING wrong initialisation on drive %d\n",
            drv);
    else
        printf("SW disk drive %d has %d MB capacity\n", drv,
            (int)sw_r[drv].nblks * (int)sw_r[drv].nhead * 
            (int)sw_r[drv].ncyl / 2000);
    return 0;
}


short sw_controllerinit()
{
    register struct device *dp = SWADDR;
    register short par, drv, h;
    short res;
    
    if (fsword(SWADDR) == -1) {
        u.u_error = ENXIO;
        return -1;
    }

    dp->swcs2 = CCLR;
    while ((dp->swcs1 & CRDY) == 0);
    
    dp->swof = FMT22;
    res = sw_eepromio(BOOTPROMSTART, 0, 1);
    if (res == -1) {
        for (drv=0; drv < NSW; drv++) {
            dp->swcs2 = drv;
            SWSECTRK(drv) = RM_NSEC;
            SWCYL(drv)    = RM_NCYLS;
            switch (dp->swdt & 255) {
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
        res |= sw_eepromio(BOOTPROMSTART+1, 0, 1);
        if (res != SWMAGIC) {
            printf("SW init: warning! wrong eeprom initialisation\n");
            return -1;
        }
        for (drv=0; drv < NSW; drv++) {
            par = sw_eepromio(drv+PARBLK_LOC, 0, 1);
            SWSECTRK(drv) = sw_eepromio(par+LSPLH_LOC,  0, 1);
            SWTRKCYL(drv) = sw_eepromio(par+LHPLC_LOC,  0, 1);
            SWCYL(drv)    = sw_eepromio(par+LCPLDL_LOC, 0, 1);
            h =             sw_eepromio(par+LCPLDH_LOC, 0, 1);
            h <<= 8;
            SWCYL(drv)   |=  h;
        }
        return 0;
    }
}

swinit()
{
    register short drv;

    if (sw_controllerinit() >= 0) {
        for (drv=0; drv < 2; drv++)
            sw_driveinit(drv, 1);
    }
    u.u_error = 0;
}

swstrategy(bp)
register struct buf *bp;
{
    register struct buf *bp2;
    register struct device *dp = SWADDR;
    register cnt;
    register blkno;
    register ushort dev = physical(bp->b_dev);
    short lun;
    int maxblks;
    
    if (dev < 2 && (sw_r[dev].stat & SW_INIT)==0)
        sw_driveinit(dev, (short)((sw_r[dev].stat & SW_SYSIO) != 0));

    lun = fsys(bp->b_dev);

    cnt = bp->b_bcount;
    if ((bp->b_flags & SWHDIO)==0)
        cnt = (cnt + (SWSECSIZ-1)) / SWSECSIZ;
    else
        cnt = (cnt + (SWHDRSIZ-1)) / SWHDRSIZ;

    maxblks = SW_SECTOT(dev) - (SW_SPARE + SWBADBLOCKOFFSET);
    blkno = bp->b_blkno;
    if (dev >= 2 || (sw_r[dev].stat & SW_INIT) == 0 ||
        ((sw_r[dev].stat & SW_SYSIO) == 0 && 
         (blkno < 0 ||
          ((sw_r[dev].stat & SW_SPECIAL) == 0 &&
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
    if (sw_r[dev].stat & SW_SYSIO)
        bp->b_cylin = CYLADDR(blkno, dev);
    else
        bp->b_cylin = CYLADDR(blkno + sw_sizes[lun].offset, dev);

    bp2 = &swutab[dev];
    spldisk();

      newsort(bp2, bp, sw_sizes);
      if (bp2->b_bcount == 0) {
          swustart(dev);
          if (swtab.b_bcount == 0)
              swstart();
      }
    spl0();
}

swustart(dev)
register ushort dev;
{
    register struct device *dp = SWADDR;
    register struct buf *bp;
    register struct buf *np;
    register short trksec;

    if (dev >= 2)
        return;

    dp->swcs2 = dev;
    dp->swas = (1 << dev);
    if ((dp->swds & (MOL|DPR|VV)) != (MOL|DPR|VV))
        sw_r[dev].stat &= ~SW_INIT;

    bp = &swutab[dev];
    if (swfmt[dev]) {
        wakeup(&swfmt[dev]);
        bp->b_bcount = 0;
        swfmt[dev] = 0;
    }
    
    np = bp->av_forw;
    if (np == 0)
        return;

    if (bp->b_bcount == 0) {
        bp->b_bcount++;
        if (sw_r[dev].stat & SW_SYSIO)
            sw_r[dev].sn = np->b_blkno + SWBADBLOCKOFFSET;
        else
            sw_r[dev].sn = np->b_blkno + 
                sw_sizes[fsys(np->b_dev)].offset + SWBADBLOCKOFFSET;
        
        trksec = TRKSEC(sw_r[dev].sn, dev);
        sw_r[dev].addr = np->b_paddr;
        sw_r[dev].rblks = trksec % SWSECTRK(dev);
        sw_r[dev].rtrk  = trksec / SWSECTRK(dev);
        sw_r[dev].nbtrans = np->b_bcount;
        if ((np->b_flags & SWHDIO)==0) {
            if (np->b_flags & B_READ)
                sw_r[dev].command = RCOM;
            else
                sw_r[dev].command = WCOM;
        } else {
            if (np->b_flags & B_READ)
                sw_r[dev].command = RHDR;
            else
                sw_r[dev].command = WHDR;
        }
        sw_r[dev].rcyl = np->b_cylin;
    }
    
    bp->b_forw = 0;
    if (swtab.av_forw == 0)
        swtab.av_forw = bp;
    else
        swtab.av_back->b_forw = bp;
    
    swtab.av_back = bp;
    spltimer();
    swtimo = 3;
}

swstart()
{
    register struct device *dp = SWADDR;
    register struct buf *bp;
    register struct buf *np;
    register ushort dev;
    register ushort cmd;

loop:
    bp = swtab.av_forw;
    if (bp == 0) return;
            
    np = bp->av_forw;
    if (np == 0) {
        swtab.av_forw = bp->b_forw;
        goto loop;
    } else {
        swtab.b_bcount++;
        dev = physical(np->b_dev);
        dp->swcs2 = dev;

        if ((dp->swds & (MOL|DPR|VV)) != (MOL|DPR|VV)) {
            printf("swstart: drive %d off-line - error\n", dev);
            sw_r[dev].stat = 0;
            swtab.b_bcount = 0;
            swtab.b_cylin = 0;
            bp->av_forw = np->av_forw;
            np->b_flags |= B_ERROR;
            iodone(np);
            goto loop;
        }
    }

    if (sw_r[dev].stat & SW_SPECIAL)
        dp->swof |= ECI;
    else
        dp->swof &= ~ECI;

    if (swtab.b_cylin >= 8) {
        if ((swtab.b_cylin & 1) != 0)
            dp->swof |= (FMT22|OFFDIR);
        else
            dp->swof &= ~OFFDIR;

        dp->swcs1 = OFFSET|GO;
        while ((dp->swds & PIP) != 0);
    }

    dp->swdc = sw_r[dev].rcyl;
    dp->swda = (sw_r[dev].rtrk << 8) | sw_r[dev].rblks;
    dp->swba  = sw_r[dev].addr;
    dp->swbae = sw_r[dev].addr >> 16;
    dp->swwc = -(sw_r[dev].nbtrans >> 1);
    cmd = (((sw_r[dev].addr >> 16) & 03) << 8) | /*bits 9-8 of swcs1 */
            sw_r[dev].command | (IE|GO);
    dp->swcs1 = cmd;
}

swintr()
{
    register struct device *dp = SWADDR;
    register struct buf *bp;
    register struct buf *np;
    register ushort dev;
    short swas;
    ushort swer2; /* set but unused */
    int altsec = 0;
    int errsec;
    ushort lsec;

    spldisk();
    swtimo = 0;
    swas = dp->swas & 0xff;
     
    if (swtab.b_bcount != 0) {
        bp = swtab.av_forw;
        np = bp->av_forw;
        dev = physical(np->b_dev);
        dp->swcs2 = dev;

        swer2 = dp->swer2;
        if (dp->swcs1 & TRE) {
            while ((dp->swds & DRY)==0);
            if (swtab.b_cylin == 0)
                swerror(np, dev, 0);
            errsec = ((dp->swdc * (int)sw_r[dev].nhead) + 
                    ((dp->swda>>8) & 0xff)) *
                (int)sw_r[dev].nblks + (dp->swda & 0xff) - 1;
                
            altsec = sw_getaltsec(dev, errsec);
            
            if ((dp->swer1 & (WLE|IAE)) ||
                (dp->swcs2 & NEM) ||
                ((sw_r[dev].ntry==0 && (np->b_flags & SWHDIO)==0) ?
                    altsec==0 :
                     swtab.b_cylin++ >= sw_r[dev].ntry) )
                np->b_flags |= B_ERROR;
            else
                swtab.b_bcount = 0;

            dp->swcs1 = DCLR|GO;
            while ((dp->swcs1 & DRY)==0);
        }
        
        if (altsec != 0 && (np->b_flags & SWHDIO)==0) {
            swtab.b_bcount = 0;
            swtab.b_cylin = 0;
            sw_r[dev].stat |= SW_ALTIO;
            lsec = errsec - sw_r[dev].sn;
            if ((((ushort)(lsec + 1)) * SWSECSIZ) < sw_r[dev].nbtrans) {
                sw_r[dev].bleft = sw_r[dev].nbtrans - (lsec + 1) * SWSECSIZ;
                sw_r[dev].nbtrans = SWSECSIZ;
            } else {
                sw_r[dev].bleft = 0;
                sw_r[dev].nbtrans -= lsec * SWSECSIZ;
            }

            sw_r[dev].addr = lsec * SWSECSIZ + sw_r[dev].addr;
            sw_r[dev].saddr = sw_r[dev].addr + SWSECSIZ;
            sw_r[dev].sblk = errsec+1;
            
            altsec += 2;
            sw_r[dev].rcyl = CYLADDR(altsec, dev);
            altsec = TRKSEC(altsec, dev);
            sw_r[dev].rblks = altsec % sw_r[dev].nblks;
            sw_r[dev].rtrk = altsec / sw_r[dev].nblks;
        }

        if (swtab.b_bcount != 0) {
            if (swtab.b_cylin != 0 || (np->b_flags & B_ERROR)) {
                dp->swcs1 = RTC|GO;
                while ((dp->swcs1 & 128)==0);
                swtab.b_cylin++;
                swerror(np, dev, (short)swtab.b_cylin);
            }
            swtab.b_bcount = 0;
            swtab.b_cylin = 0;
            if (sw_r[dev].stat & SW_ALTIO) {
                sw_r[dev].stat &= ~SW_ALTIO;
                if ((np->b_flags & B_ERROR)==0 && sw_r[dev].bleft != 0) {
                    sw_r[dev].sn = sw_r[dev].sblk;
                    sw_r[dev].addr = sw_r[dev].saddr;
                    sw_r[dev].nbtrans = sw_r[dev].bleft;
                    sw_r[dev].rcyl = CYLADDR(sw_r[dev].sblk, dev);
                    altsec = TRKSEC(sw_r[dev].sblk, dev);
                    sw_r[dev].rblks = altsec % sw_r[dev].nblks;
                    sw_r[dev].rtrk = altsec / sw_r[dev].nblks;
                    goto retry;
                }
            }

            swtab.av_forw = bp->b_forw;
            bp->b_bcount = 0;
            bp->b_cylin = 0;
            bp->av_forw = np->av_forw;
            if (np->b_flags & B_ERROR)
                np->b_cylin = np->b_bcount;
            else
                np->b_cylin = 0;
            
            iodone(np);
            if (bp->av_forw)
                swustart(dev);
        }
        swas &= ~(1<<dev);
    } else {
        for (dev=0; dev < NSW; dev++) {
            if (swfmt[dev] != 0)
                swas |= (1 << dev);
        }
    }

retry:
    for (dev=0; dev < NSW; dev++) {
        if ((swas & (1 << dev)) != 0 &&
            ((sw_r[dev].stat & 1) || swfmt[dev]))
          swustart(dev);
    }

    swstart();
}


swerror(bp, dev, ntry)
register struct buf *bp;
register ushort dev;
register short ntry;
{
    register struct device *dp = SWADDR;

    if (ntry-- == 0) {
        sw_r[dev].cs1 = dp->swcs1;
        sw_r[dev].cs2 = dp->swcs2;
        sw_r[dev].ds  = dp->swds;
        sw_r[dev].er1 = dp->swer1;
        sw_r[dev].er2 = dp->swer2;
        sw_r[dev].wc  = dp->swwc;
        sw_r[dev].ba  = dp->swba;
        sw_r[dev].da  = dp->swda;
        sw_r[dev].dc  = dp->swdc;
        sw_r[dev].bae = dp->swbae;
        sw_r[dev].badsn = (dp->swdc * (int)sw_r[dev].nhead + 
            ((dp->swda >> 8) & 0xff)) * (int)sw_r[dev].nblks + 
            (dp->swda & 0xff) - 1;
        return;
    }

    if ((bp->b_flags & B_ERROR) != 0 || ntry >= sw_r[dev].ntry)
        printf("\nsw: fatal ");
    else
        printf("\nsw: recoverable ");

    printf("SW disk error: drive=%d  physical sector=%ld (+SWBADBLOCKOFFSET)\n",
        dev, sw_r[dev].badsn-2);
    printf("Error Count = %d\n", ntry);

    if ((sw_r[dev].ds & (MOL|DPR|VV)) != (MOL|DPR|VV))
        printf("SW: drive is offline\n");

    if (sw_r[dev].er1 & WLE)
        printf("SW: drive is write protected\n");

    if (sw_r[dev].er1 & (HCRC|HCE))
        printf("SW: could not read header of sector\n");

    if ((sw_r[dev].er1 & (DCK|DTE|ECH)) || (sw_r[dev].er2 & BSE))
        printf("SW: sector is bad\n");

    printf("cs1=%4x cs2=%4x ds=%4x er1=%4x er2=%4x\n",
        sw_r[dev].cs1, sw_r[dev].cs2, sw_r[dev].ds, 
        sw_r[dev].er1, sw_r[dev].er2);
}

sw_iohdr(devno, flags, func)
short devno;
{
    register short dev = physical(devno);
    register struct buf *bp = &swcbuf;

    spldisk();
      while (bp->b_flags & B_BUSY) {
          bp->b_flags |= B_WANTED;
          sleep(bp, PRIBIO+1);
      }
    spl0();
    
    bp->b_flags = flags | (SWHDIO|B_BUSY|B_PHYS);
    bp->b_dev = devno;
    bp->b_paddr = logtophys(&sw_hdr[dev]);
    bp->b_bcount = SWHDRSIZ;
    bp->b_error = 0;
    bp->b_blkno = sw_hdr[dev].un_sw.sw_sec;
    if (func == SW_HDRVAL) {
        sw_hdr[dev].un_sw.sw_blkhdr.sw_flgcyl = 
            CYLADDR(bp->b_blkno+SWBADBLOCKOFFSET, dev);
        sw_hdr[dev].un_sw.sw_blkhdr.sw_flgcyl |= 0150000; /* which flags? */

#define TRSCD (TRKSEC(bp->b_blkno+SWBADBLOCKOFFSET, dev) / sw_r[dev].nblks)
#define TRSCM (TRKSEC(bp->b_blkno+SWBADBLOCKOFFSET, dev) % sw_r[dev].nblks)
        sw_hdr[dev].un_sw.sw_blkhdr.sw_trksec = 
            ((TRSCD & 0xff) << 8) | (TRSCM & 0xFF);
    } else if (func == SW_HDRINVAL) {
        sw_hdr[dev].un_sw.sw_blkhdr.sw_flgcyl = 0;
        sw_hdr[dev].un_sw.sw_blkhdr.sw_trksec = 0;
    }

    spldisk();
      swstrategy(bp);
      while ((bp->b_flags & B_DONE)==0)
          sleep(bp, PRIBIO);
    spl0();
    
    if ((bp->b_flags & B_ERROR) && (u.u_error=bp->b_error)==0)
        u.u_error = EIO;

    if (bp->b_flags & B_WANTED)
        wakeup(bp);

    bp->b_flags &= ~(B_WANTED|B_BUSY);
}

swopen(devno)
short devno;
{
    register struct device *dp = SWADDR;
    ushort dev = physical(devno);

    if (fsword(SWADDR) == -1 || dev >= 2) {
        u.u_error = ENXIO;
        return;
    }

    spldisk();
      dp->swcs2 = dev;
      while ((dp->swcs1 & CRDY)==0);

      if ((dp->swds & (MOL|DPR|VV)) != (MOL|DPR|VV))
          sw_r[dev].stat = 0;
    spl0();

    if ((sw_r[dev].stat & SW_INIT)==0) {
        sw_driveinit(dev, 1);
        if (u.u_error != 0 && suser()) {
            u.u_error = 0;
            printf("swopen: Warning drive %d  not ready\n", dev);
        }
    }

    if (swtimeorun == 0) {
        swtimeorun++;
        timeout(swtimeo, &swtimo, hz);
    }
}

swioctl(devno, func, arg)
register short devno;
register caddr_t arg;
{
    register struct device *dp = SWADDR;
    register ushort dev;
    struct buf *swup;
    register struct swctl *ap1 = (struct swctl*)arg;
    struct sw_badblockinfo *ap2 = (struct sw_badblockinfo*)arg;
    struct sw_sechdr *ap3 = (struct sw_sechdr*)arg;
    struct sw_conf *ap4 = (struct sw_conf*)arg;
    int i;

    if (suser()==0)
        return;

    dev = physical(devno);
    switch (func) {
    case SW_FORMAT:
        spldisk();
          swup = &swutab[dev];
          while (swup->b_bcount)
              sleep(swup->av_forw, PRIBIO);
        
          swup->b_bcount++;

          dp->swcs2 = dev;
          dp->swcs1 = DCLR|GO;
          while ((dp->swcs1 & CRDY)==0);

          dp->swcs2 = dev;
          dp->swcs1 = PACK|GO;
          while ((dp->swcs1 & CRDY)==0);

          dp->swcs2 = dev;
          dp->swcs1 = PRESET|GO;
          while ((dp->swcs1 & CRDY)==0);

          dp->swas = 1 << dev;
          if ((dp->swds & (MOL|DPR|VV)) != (MOL|DPR|VV)) {
            u.u_error = EIO;
            swup->b_bcount = 0;
            spl0();
            return;
          }

          dp->swof = FMT22;
          dp->swcs1 = (IE|FORMAT|GO);
          swfmt[dev]++;
          sleep(&swfmt[dev], PRIBIO);

          dp->swcs2 = dev;
          dp->swcs1 = (DCLR|GO);
          while ((dp->swcs1 & CRDY)==0);
          
          swup->b_bcount = 0;
        spl0();
        break;

    case SW_REEPROM:
        suword(&ap1->sw_data, sw_eepromio(fuword(ap1), 0, 1));
        break;
    
    case SW_WEEPROM:
        sw_eepromio(fuword(ap1), fuword(&ap1->sw_data), 0);
        break;

    case SW_RSTPROM:
        spldisk();
          ssword(dp, RSPROM|GO);
          printf("SW: reinitializing Controller\n");
          for (i=0; i < 20000; i++);
          while ((dp->swcs1 & CRDY)==0);
          dp->swcs2 |= CCLR;
          dp->swof = FMT22;
        spl0();
        
        for (dev=0; dev < NSW; dev++)
            sw_r[dev].stat = 0;
        break;

    case SW_READBAD:
        copyout(&sw_badblockinfo[dev], ap2, sizeof(struct sw_badblockinfo));
        break;
        
    case SW_WRITEBAD:
        if (copyin(ap2, &sw_badblockinfo[dev], 
                sizeof(struct sw_badblockinfo)) < 0) {
            u.u_error = EIO;
            return;
        }
        sw_badblkio(dev, B_WRITE);
        break;
        
    case SW_CLEAR:
        spldisk();
          dp->swcs2 = CCLR;
        spl0();
        break;

    case SW_HDRVAL:
    case SW_HDRINVAL:
        if (copyin(ap3, &sw_hdr[dev], sizeof(struct sw_sechdr)) < 0) {
            u.u_error = EIO;
            return;
        }
        sw_iohdr(devno, B_WRITE, func);
        break;
        
    case SW_READHDR:
        if (copyin(ap3, &sw_hdr[dev], sizeof(struct sw_sechdr)) < 0) {
            u.u_error = EIO;
            return;
        }
        sw_iohdr(devno, B_READ, func);
        copyout(&sw_hdr[dev], ap3, sizeof(struct sw_sechdr));
        break;
    
    case SW_REINIT:
        sw_driveinit(dev, 1);
        break;

    case SW_CONTRINIT:
        sw_controllerinit();
        break;
        
    case SW_NORETRY_AND_ECC:
        sw_r[dev].stat |= SW_SPECIAL;
        sw_r[dev].ntry = 0;
        break;

    case SW_GETCONF:
        copyout(&sw_r[dev], ap4, sizeof(struct sw_conf));
        break;
        
    default:
        u.u_error = EINVAL;
        break;
    }
}

/* 0=write, 1=read */
sw_badblkio(dev, rw)
short dev;
{
    register struct buf *bp = &swcbuf;

    spldisk();
      while (bp->b_flags & B_BUSY) {
          bp->b_flags |= B_WANTED;
          sleep(bp, PRIBIO+1);
      }
    spl0();

    bp->b_flags = rw | (B_BUSY|B_PHYS);
    bp->b_dev = dev << 4;
    bp->b_paddr = logtophys(&sw_badblockinfo[dev]);
    bp->b_blkno = -SWBADBLOCKOFFSET;
    bp->b_bcount = sizeof(struct sw_badblockinfo);
    bp->b_error = 0;
    spldisk();
      sw_r[dev].stat |= SW_SYSIO;
      swstrategy(bp);
      sw_r[dev].stat &= ~SW_SYSIO;
      while ((bp->b_flags & B_DONE)==0)
          sleep(bp, PRIBIO);
    spl0();
    
    if (bp->b_flags & B_ERROR && (u.u_error=bp->b_error)==0)
        u.u_error = EIO;

    if (bp->b_flags & B_WANTED)
        wakeup(bp);

    bp->b_flags &= ~(B_WANTED|B_BUSY);
}


sw_getaltsec(dev, sec)
ushort dev;
register sec;
{
    register struct sw_badblockinfo *bbp = &sw_badblockinfo[dev];
    register short i;

    if (sec != 0) {
        for (i=0; i < MAXBADSEC; i++) {
            if ((sec-SWBADBLOCKOFFSET) == bbp->sw_log_bad[i])
                return bbp->sw_log_alter[i];
        }
    }
    return 0;
}

/* C function */
swread(dev)
{
    physio(swstrategy, &swrbuf, dev, 1);
}

/* C function */
swwrite(dev)
{
    physio(swstrategy, &swrbuf, dev, 0);
}

swtimeo()
{
    if (swtimo >= 0)
        swtimo--;

    if (swtimo == 0 && swtab.b_bcount) {
        printf("SW: interrupt timeout\n");
        swtimeostat++;
        swintr();
    }

    timeout(swtimeo, &swtimo, hz);
}
