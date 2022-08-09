/* PCS specific */
/* note the objfile jas some unexplained slack at the beginning of
 * the data segment, so compile won't reproduce the exact byte
 * structure in disassembler IDA. However, the code itself is correct.
 * Might be bit rot */
static char *_Version = "@(#) RELEASE:  1.5  Feb 05 1987 /usr/sys/munet/master.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/filsys.h>
#include <sys/reg.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/swap.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/conf.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/bpdata.h>
#include <sys/munet/diskless.h>
#include <fcntl.h>
#include "rbq.h"

struct buf *rootbp;
struct mount rootmount;
int dlnactv;

extern struct filsys *rootfp;
extern short my_version;

int adddlnode(reqno)
register short reqno;
{
    register struct dltable *dp = &dltable[reqno];
    register int ret;

    if (devisused(dp, dp->dl_dev))
        return -1;
    
    if (dp->dl_flags & DL_CONNECTED) {
        dlswapfree(reqno);
        ret = 1;
    } else {
        dlnactv++;
        ret = 0;
    }
    
    dp->dl_rootdev = dp->dl_dev;
    dp->dl_swapdev = dp->dl_swdev;
    dp->dl_flags = DL_CONNECTED;
    dp->dl_wbufbase = -1;
    if (rootfp==0 || dlnactv==1) {
        if (rcomsup(dp->dl_rootdev) < 0)
            ret = -1;
    }
    return ret;
}

int devisused(dp, dev)
register struct dltable *dp;
register dev_t dev;
{
    register struct mount *mp;
    
    for (mp = mount; mp < (struct mount*)v.ve_mount ; mp++) {
        if (!mp->m_flags || mp->m_bufp == 0)
            continue;
        if (mp->m_dev != dev && (dev | Fs2BLK) != mp->m_dev)
            continue;
        return -1;
    }
    return 0;
}

rmdlnode(reqno)
register short reqno;
{
    dltable[reqno].dl_flags |= DL_PROCESSING;
    dltable[reqno].dl_cmd = DLCLEAR;
    wakeup((caddr_t)&dltable[reqno]);
}

clrdlnode(reqno)
register short reqno;
{
    register struct dltable *dp = &dltable[reqno];

    dlswapfree(reqno);
    dp->dl_addr = noname;
    dp->dl_rootdev = NODEV;
    dp->dl_swapdev = NODEV;
    dp->dl_flags = 0;
    dp->dl_seqno = 0;
    dp->dl_cmd = 0;
    dp->dl_bufid = 0;
    dp->dl_lastin = 0;
    dp->dl_error = 0;
    dp->dl_ipaddr = 0;
    dp->dl_swdev = 0;
    if (--dlnactv == 0)
        comrootdev = NODEV;
}

dlswapfree(reqno)
register int reqno;
{
    register long *smp;
    register int dummy;
    register int npgs;
    dbd_t dbd;
    int i, k;
    long swplo;

    dbd.dbd_swpi = comswapsmi;
    smp = dltable[reqno].dlswapmap;
    if (smp == 0)
        return;

    npgs = (swaptab[comswapsmi].st_npgs + 31) >> 5;
    swplo = swaptab[comswapsmi].st_swplo;
    
    for (i = 0; i < npgs; i++) {
        if (smp[i]) {
            for (k = 0; k < 32; k++) {
                if ((smp[i] & (1<<k))) {
                    dbd.dbd_blkno = ((i << 5) + k) << (swplo+3);
                    swfree1(&dbd);
                }
            }
        }
        smp[i] = 0;
    }
}

reqproc()
{
    register int *unused1; /* dummy */
    register struct dltable *dp;
    register int haspkt, pktsz;

    int noerr;
    short reqno;
    int unused2;  /* dummy */

    noerr = 1;
    reqno = 0;
    dlnactv = 0;

retry:
    spldisk();

    if (noerr == 1) {
        while (req_q == 0)
            sleep((caddr_t)&req_q, PSWP+1);
        for (;;) {
            if (req_q & (1 << reqno))
                break;
            reqno++;
            if (reqno >= maxdlnactv)
                reqno = 0;
        }
        
        req_q &= ~(1<<reqno);
        spl0();
    } else {
        while (!(dltable[reqno].dl_flags & DL_PROCESSING))
            sleep((caddr_t)&dltable[reqno], PSWP+2);
        spl0();
    }
    
    dp = &dltable[reqno];
    u.u_error = 0;
    switch (dp->dl_cmd) {
    case DLCLEAR:
        {
            register long npgs;
            
            if (comrootdev != NODEV) {
                iflush(comrootdev);
                bflush(comrootdev);
                wcomsup(comrootdev);
            }
            spldisk();
            clrdlnode(reqno);
            spl0();
        
            npgs = swaptab[comswapsmi].st_npgs;
            npgs += 8191;
            npgs >>= 13;
            if (dltable[reqno].dlswapmap)
                sptfree(dltable[reqno].dlswapmap, npgs, 1);
            dltable[reqno].dlswapmap = 0;
            exit(0);
            break;
        }
    case RMTRBLK:
    case DLMPOLL:
    case DLMREAD:
    case DLIREAD:
        {
            register int node = uifindnode3(dp->dl_ipaddr);
            switch (uinode[node].nd_vers) {
            case 0:
            case 1:
            case 3:
                pktsz = 1024 + FR_HEADER_SIZE - 2;
                break;
            case 4:
                if (dp->dl_pageio == 1)
                    pktsz = sizeof(struct dlpacket);
                else
                    pktsz = 1024 + FR_HEADER_SIZE - 2;
                break;
            default:
                printf("reqproc: unknown node version type\n");
                break;
            }
            break;
        }
    case DLBOOT:
        pktsz = FR_HEADER_SIZE + 28;
        break;
    case DLALLOC:
        pktsz = 1024 + FR_HEADER_SIZE - 2;
        break;
    default:
        pktsz = FR_HEADER_SIZE - 2;
        break;
    }
    
    haspkt = 1;
    switch (dp->dl_cmd) {
    case SWALLOC:
    case SWALLOCCAT:
    case SWALLOCCL:
    case SWFREE:
    case DLALLOC:
    case DLFREE:
    case DLIALLOC:
    case DLIFREE:
    case DLWCSUP:
    case DLIREAD:
    case DLIUPDAT:
    case SWFREE_M32:
    case DLSHUTDOWN:
        if (dp->dl_flags & DL_RETRY) {
            reqsend(dp, pktsz);
            goto retry;
        }
        break;
    }
    
    switch (dp->dl_cmd) {
    case SWALLOC:
        {
            register int dlsize;
            int i;
            int dlret;
            int map;
            use_t *usep;
            int k;

            dp->dl_ret = swapfind(&swaptab[comswapsmi], dp->dl_size);
            if (dp->dl_ret == 0) {
                swapclup();
                dp->dl_ret = swapfind(&swaptab[comswapsmi], dp->dl_size);
            }
            if (dp->dl_ret) {
                dlsize = dp->dl_size;
                dlret = dp->dl_ret - 1;
                usep = &swaptab[comswapsmi].st_ucnt[dlret];
                for (i= 0; i < dlsize; i++)
                    *usep++ = 1;
                i = dlret >> 5;
                if (dlret & 31) {
                    map = 0;
                    for (k = dlret & 31; k < 32 && dlsize-- != 0; k++)
                        map |= (1<<k);
                    if (dp->dlswapmap[i] & map)
                        printf("master: inconsistent swapmap");
                    dp->dlswapmap[i++] |= map;
                }
                while (dlsize >= 32) {
                    if (dp->dlswapmap[i])
                        printf("master: inconsistent swapmap");
                    dp->dlswapmap[i++] = -1;
                    dlsize -= 32;
                }
                if (dlsize) {
                    map = 0;
                    while (dlsize-- > 0)
                        map |= (1<<dlsize);
                    if (dp->dlswapmap[i] & map)
                        printf("master: inconsistent swapmap");
                    dp->dlswapmap[i] |= map;
                }
            }
            break;
        }

    case SWFREE_M32:
        {
            register int dlsize;
            dbd_t *dbd;
            long map, blk, swplo;
            
            dbd = (dbd_t*)dp->dl_wbuffer;
            dlsize = dp->dl_size;
            swplo = swaptab[comswapsmi].st_swplo;
            while (dlsize-- > 0) {
                dbd->dbd_swpi = comswapsmi;
                swfree1(dbd);
                blk = (dbd->dbd_blkno - swplo) >> 3;
                map = 1 << (blk & 31);
                blk >>= 5;
                if ((dp->dlswapmap[blk] & map) == 0)
                    printf("master: releasing free swap space");
                dp->dlswapmap[blk] &= ~map;
                dbd++;
            }
            dp->dl_ret = 0;
            break;
        }

    case SWFREE:
        break;

    case SWALLOCCL:
        break;
    
    case SWALLOCCAT:
        break;


    case RMTRBLK:
    case DLMREAD:
        {
            register int sz;
            register struct buf *bp;
            caddr_t addr;
            daddr_t base;
            
            if (dp->dl_pageio) {
                if (BUFBASE(dp->dl_blkno) != dp->dl_wbufbase) {
                    dp->dl_wbufbase = BUFBASE(dp->dl_blkno);
                    addr = (caddr_t)dp->dl_wbuffer;
                    sz = 4096;
                    base = dp->dl_wbufbase;
                    splhi();
                    while ((bp = pfreelist.av_forw)==0) {
                        pfreelist.b_flags |= B_WANTED;
                        sleep((caddr_t)&pfreelist, PRIBIO+1);
                    }
                    pfreelist.av_forw = bp->av_forw;
                    bp->b_proc = u.u_procp;
                    bp->b_flags = (B_PHYS|B_BUSY|B_READ);
                    bp->b_dev = dp->dl_dev;
                    bp->b_un.b_addr = addr;
                    bp->b_paddr = logtophys(addr);
                    bp->b_blkno = FsLTOP(dp->dl_dev, base);
                    bp->b_bcount = sz;
                    bp->b_error = 0;
                    (*bdevsw[bmajor(bp->b_dev)].d_strategy)(bp);
                    clrcache();
                    while (!(bp->b_flags & B_DONE))
                        sleep((caddr_t)bp, PRIBIO);
                    if (bp->b_flags & B_ERROR) {
                        if ((u.u_error = bp->b_error) == 0)
                            u.u_error = EIO;
                    }
                    splhi();
                    bp->av_forw = pfreelist.av_forw;
                    pfreelist.av_forw = bp;
                    if (pfreelist.b_flags & B_WANTED) {
                        pfreelist.b_flags &= ~B_WANTED;
                        wakeup((caddr_t)&pfreelist);
                    }
                    spl0();
                }
            } else {
                bp = bread(dp->dl_dev, dp->dl_blkno);
                bcopy(bp->b_un.b_addr, dp->dl_wbuffer, FsBSIZE(dp->dl_dev));
                brelse(bp);
                dp->dl_wbufbase = -1;
            }
            break;
        }
    case RMTWBLK:
        {
            register struct buf *bp;
            
            if (dp->dl_pageio == 0) {
                dp->dl_wbufbase = -1;
                bp = getblk(dp->dl_dev, dp->dl_blkno);
                bcopy(dp->dl_wbuffer, bp->b_un.b_addr, FsBSIZE(dp->dl_dev));
                bdwrite(bp);
            } else if (BUFOFF(dp->dl_blkno) == 3 || dp->dl_pageio == 2) {
                splhi();
                while ((bp = pfreelist.av_forw)==0) {
                    pfreelist.b_flags |= B_WANTED;
                    sleep((caddr_t)&pfreelist, PRIBIO+1);
                }
                pfreelist.av_forw = bp->av_forw;
                bp->b_proc = u.u_procp;
                bp->b_flags = (B_PHYS|B_BUSY|B_WRITE);
                bp->b_dev = dp->dl_dev;
                bp->b_un.b_addr = dp->dl_wbuffer;
                bp->b_paddr = logtophys(dp->dl_wbuffer);
                if (dp->dl_pageio == 1) {
                    bp->b_blkno = FsLTOP(dp->dl_dev, dp->dl_wbufbase);
                    bp->b_bcount = 4096;
                } else {
                    bp->b_blkno = FsLTOP(dp->dl_dev, dp->dl_blkno);
                    bp->b_bcount = FsBSIZE(dp->dl_dev);
                    dp->dl_wbufbase = -1;
                }
                
                bp->b_error = 0;
                (*bdevsw[bmajor(bp->b_dev)].d_strategy)(bp);
                clrcache();

                splhi();
                while (!(bp->b_flags & B_DONE))
                    sleep((caddr_t)bp, PRIBIO);
                if (bp->b_flags & B_ERROR) {
                    if ((u.u_error = bp->b_error)==0)
                        u.u_error = EIO;
                }
                bp->av_forw = pfreelist.av_forw;
                pfreelist.av_forw = bp;
                if (pfreelist.b_flags & B_WANTED) {
                    pfreelist.b_flags &= ~B_WANTED;
                    wakeup((caddr_t)&pfreelist);
                }
                spl0();
            }
            break;
        }

    case DLINIT:
        {
            register int ret;
            long npgs;

            if ((ret = adddlnode(reqno)) < 0)
                u.u_error = EBUSY;
            else {
                dp->dl_ret = reqno;
                if (ret == 0) {
                    noerr = 0;
                    ret = newproc(1);
                    dltable[reqno].dlswapmap = 0;
                    if (ret == 1) {
                        npgs = swaptab[comswapsmi].st_npgs;
                        npgs += 8191;
                        npgs >>= 13;
                        u.u_procp->p_flag |= (SLOAD|SSYS);
                        u.u_procp->p_ppid = 1;
                        if ((dltable[reqno].dlswapmap = (long*)sptalloc(npgs)) == 0) {
                            clrdlnode(reqno);
                            exit(0);
                        }
                        goto retry;
                    }
                    noerr = 1;
                    if (ret == -1) {
                        spldisk();
                        clrdlnode(reqno);
                        dp->dl_flags |= DL_TEMP;
                        spl0();
                    }
                }
            }
            break;
        }

    case DLMPOLL:
    case DLBOOT:
        {
            register int i;
            for (i=0; i < 64; i++) {
                if (UISAME(&dp->dl_addr, &bpdata[i].bp_etname))
                    break;
            }
            if (i >= 64)
                haspkt = 0;
            else if (bpdata[i].bp_rootdev != NODEV) {
                if (comrootdev == NODEV)
                    rcomsup(bpdata[i].bp_rootdev);
                bpdata[i].bp_rootdev = dp->dl_rootdev = comrootdev;
                if (dp->dl_cmd == DLBOOT)
                    bcopy(bpdata[i].bootfile, dp->dl_wbuffer, BFC);
                else
                    bcopy(&bpdata[i], dp->dl_wbuffer, sizeof(struct bpdata));
            } else
                dp->dl_rootdev = NODEV;
            break;
        }

    case DLALLOC:
        {
            long *blks = (long*)dp->dl_wbuffer;
            register int sz;
            
            for (sz = dp->dl_size, dp->dl_size = 0; --sz >= 0; dp->dl_size++, ++blks) {
                *blks = doalloc(dp->dl_dev, rootfp);
                if (*blks == 0) break;
            }
            break;
        }

    case DLFREE:
        {
            long *blks = (long*)dp->dl_wbuffer;
            register int sz;
            
            for (sz = dp->dl_size; --sz >= 0; ) {
                dp->dl_blkno = *blks; blks++;
                dofree(dp->dl_dev, dp->dl_blkno, rootfp);
            }
            break;
        }

    case DLIALLOC:
        {
            struct inode *ip;
            ino_t ino;
loop:
            ino = doialloc(dp->dl_dev, rootfp);
            if (ino == 0) {
                dp->dl_blkno = 0;
                break;
            }
            ip = iget(&rootmount, ino);
            if (ip == 0) {
                dp->dl_blkno = 0;
                u.u_error = ENFILE;
                break;
            }
            
            if (ip->i_mode == 0) {
                ip->i_mode = 1;
                ip->i_nlink = 1;
                ip->i_flag |= ICHG;
                if (rootfp->s_tinode)
                    rootfp->s_tinode--;
                rootfp->s_fmod = 1;
                if (rootmount.m_count <= 0)
                    rootmount.m_count = 1;
                iput(ip);
                dp->dl_blkno = ino;
                break;
            }

            if (rootmount.m_count <= 0)
                rootmount.m_count = 1;
            iput(ip);
            goto loop;
        }

    case DLSHUTDOWN:
        dp->dl_lastin = time - 88;      /* why this 88? */
        /*FALLTHRU*/

    case DLWCSUP:
        {
            long *blks = (long*)dp->dl_wbuffer;
            register int sz;
    
            for (sz = dp->dl_size; --sz >= 0; ) {
                dp->dl_blkno = *blks; blks++;
                dofree(dp->dl_dev, dp->dl_blkno, rootfp);
            }
            wcomsup(dp->dl_dev);
            dp->dl_ret = rootfp->s_tinode;
            dp->dl_blkno = rootfp->s_tfree;
            break;
        }

    case DLIREAD:
        {
            struct dinode *addr;
            struct buf *bp;
            ushort inum;

            dp->dl_wbufbase = -1;
            inum = dp->dl_blkno;
            bp = bread(dp->dl_dev, FsITOD(dp->dl_dev, inum));
            if (u.u_error)
                dp->dl_blkno = 0;
            else {
                addr = bp->b_un.b_dino;
                addr += FsITOO(dp->dl_dev, inum);
                dp->dl_blkno = inum;
                bcopy(addr, dp->dl_wbuffer, sizeof(struct dinode));
            }
            brelse(bp);
            break;
        }

    case DLIUPDAT:
        {
            register int *unused;
            struct dinode *dip;
            int inum;
            int *ap;        /* actually the argument structure passed:
                             * struct { time_t atime, mtime;
                             *          int iflags;
                             *          struct dinode dinode; }
                             */
            int atime, mtime;
            uint iflags;
            dev_t dev;
            struct buf* bp;
            
            dp->dl_wbufbase = -1;
            inum = dp->dl_blkno;
            dev = dp->dl_dev;
            bp = bread(dev, FsITOD(dev, inum));
            if (bp->b_flags & B_ERROR) {
                brelse(bp);
                break;
            }
            dip = bp->b_un.b_dino;
            dip += FsITOO(dev, inum);
            ap = (int*)dp->dl_wbuffer;
            atime = *ap++;
            mtime = *ap++;
            iflags = *(uint*)ap++;
            bcopy(ap, dip, sizeof(daddr_t)*NADDR);
            if (iflags & IACC)
                dip->di_atime = atime;
            if (iflags & IUPD)
                dip->di_mtime = mtime;
            if (iflags & ICHG)
                dip->di_ctime = time;
            bdwrite(bp);
            dip = (struct dinode*)ap;
            if (dip->di_nlink <= 0 && dip->di_mode == 0)
                doifree(inum, rootfp);
            break;
        }
    default:
        haspkt = 0;
        break;
    }

    if (haspkt) {
        dp->dl_error = u.u_error;
        u.u_error = 0;
        reqsend(dp, pktsz);
    } else
        dp->dl_flags = 0;
    goto retry;
}

reqsend(dp, sz)
register struct dltable *dp;
unsigned int sz;
{
    ushort bufid;
    struct dlpacket *dlp;

    dlp = (struct dlpacket*)buf_get((dp->dl_bufid & 0xff00) >> 8, sz, 2, &bufid);
    dlp->e_dest = dp->dl_addr;
    dlp->rt_type = RTTYPE;
    dlp->rt_command = dp->dl_cmd;
    dlp->rt_seqno = dp->dl_seqno;

    switch (dp->dl_cmd) {
    case DLINIT:
        dlp->rt_blkno = time;
        dlp->rt_dev = swapdev;
        dlp->rt_return = dp->dl_ret;
        break;

    case SWALLOC:
    case SWALLOCCAT:
    case SWALLOCCL:
    case SWFREE:
    case SWFREE_M32:
        dlp->rt_start = dp->dl_start;
        dlp->rt_return = dp->dl_ret;
        dlp->rt_size = dp->dl_size;
        break;
    
    case DLBOOT:
        bcopy(dp->dl_wbuffer, dlp->rt_data, 30);
        break;

    case DLMPOLL:
        bcopy(dp->dl_wbuffer, dlp->rt_data, 72);
        dlp->rt_command = DLMASTER;
        dlp->rt_return = my_version;
        break;

    case RMTRBLK:
    case DLMREAD:
        if (dp->dl_error == 0) {
            if (dp->dl_pageio) {
                if (sz >= 4096 && dp->dl_pageio==1) {
                    dlp->rt_size = 4096;
                    buf_copy(bufid, dlp->rt_data, dp->dl_wbuffer, 4096, 1, 0);
                } else {
                    dlp->rt_size = FsBSIZE(dp->dl_dev);
                    bcopy(dp->dl_wbuffer + (dp->dl_blkno & 3) * FsBSIZE(dp->dl_dev), 
                        dlp->rt_data, FsBSIZE(dp->dl_dev));
                }
            } else {
                dlp->rt_size = FsBSIZE(dp->dl_dev);
                bcopy(dp->dl_wbuffer, dlp->rt_data, FsBSIZE(dp->dl_dev));
            }
        }
        break;

    case RMTWBLK:
        dlp->rt_size = dp->dl_size;
        /*FALLTHRU*/

    case DLFREE:
    case DLIFREE:
    case DLIUPDAT:
        break;

    case DLWCSUP:
    case DLSHUTDOWN:
        dlp->rt_blkno = dp->dl_blkno;
        dlp->rt_return = dp->dl_ret;
        break;

    case DLALLOC:
        dlp->rt_size = dp->dl_size;
        bcopy(dp->dl_wbuffer, dlp->rt_data, dp->dl_size * sizeof(int));
        break;
    
    case DLIREAD:
        bcopy(dp->dl_wbuffer, dlp->rt_data, sizeof(struct dinode));
        /*FALLTHRU*/

    case DLIALLOC:
        dlp->rt_blkno = dp->dl_blkno;
        break;

    default:
        goto error;
    }       

    dlp->rt_error = dp->dl_error;
    St_write("reqsend_NFSD1", 7, 1, dlp->rt_command, 0x0f, RTTYPE);
    St_write("reqsend_RPCD1", 8, 1, dlp->rt_type, 0x0f);
    if (uiwrites(bufid, dp->dl_ipaddr, ETHERPUP_IPTYPE))
        St_write("reqsend_RPCD2\0", 8, 1, -1, 0x14);
    
    dp->dl_flags &= ~(DL_PROCESSING|DL_RETRY|DL_TEMP);
    if (dp->dl_cmd == DLSHUTDOWN)
        rmdlnode(dp - dltable);
    return;

error:
    dp->dl_flags &= ~(DL_PROCESSING|DL_RETRY|DL_TEMP);
    buf_rel(bufid);
}

int rcomsup(dev)
dev_t dev;
{
    struct inode rinode;
    
    if (rootbp == 0)
        rootbp = geteblk();

    rootfp = rootbp->b_un.b_filsys;
    rootmount.m_flags = MINUSE;
    rootmount.m_dev = dev;
    rootmount.m_bufp = rootbp;
    rootmount.m_count = 0;
    rootmount.m_mount = 0;
    rootmount.m_omnt = 0;
    rinode.i_mode = IFBLK;
    rinode.i_rdev = dev;
    rinode.i_mount = &rootmount;
    u.u_offset = 512;
    u.u_count = sizeof(struct filsys);
    u.u_base = (caddr_t)rootfp;
    u.u_segflg = 1;
    readi(&rinode);
    if (u.u_error) {
        brelse(rootbp);
        rootfp = 0;
        return -1;
    } else {
        if (rootfp->s_magic != FsMAGIC)
            rootfp->s_type = Fs1b;
        
        comrootdev = dev;
        if (rootfp->s_type == Fs2b)
            comrootdev |= Fs2BLK;
        rootfp->s_ilock = 0;
        rootfp->s_flock = 0;
        rootfp->s_ninode = 0;
        rootfp->s_inode[0] = 0;
        rootfp->s_ronly = 0;
        return 0;
    }
}

wcomsup(dev)
dev_t dev;
{
    struct inode rinode;
    if (!rootfp->s_fmod)
        return;

    rinode.i_mode = IFBLK;
    rinode.i_rdev = dev;
    rootfp->s_fmod = 0;
    rootfp->s_time = time;
    rootfp->s_state = FsOKAY - rootfp->s_time;
    u.u_error = 0;
    u.u_offset = 512;
    u.u_count = sizeof(struct filsys);;
    u.u_base = (caddr_t)rootfp;
    u.u_segflg = 1;
    u.u_fmode = O_SYNC|O_RDWR;
    writei(&rinode);
    return 0;
}
