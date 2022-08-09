/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.5  May 07 1987 /usr/sys/munet/dlnode.c ";

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
#include "rbq.h"

int dlswapaccount = 0;
extern short my_version;
extern int rmtopen();

#define CACHESZ 256

struct buf *diskless_getbuf()
{
    register struct buf *bp;
    int s = splhi();

    while ((bp = pfreelist.av_forw) == 0) {
        pfreelist.b_flags |= B_WANTED;
        sleep((caddr_t*)&pfreelist, NZERO+1);
    }
    pfreelist.av_forw = bp->av_forw;
    bp->b_flags = B_SWAP|B_BUSY;
    
    splx(s);
    bp->b_proc = u.u_procp;
    return bp;
}

diskless_relbuf(bp)
struct buf *bp;
{
    int s = splhi();

    bp->av_forw = pfreelist.av_forw;
    pfreelist.av_forw = bp;
    if (pfreelist.b_flags & B_WANTED) {
        pfreelist.b_flags &= ~B_WANTED;
        wakeup((caddr_t)&pfreelist);
    }
    
    splx(s);
}

dsklinit()
{
    register int i, sz;                 /* sz unused */
    caddr_t bp;

    rem_version = my_version;

    /* new payload = 4096, old payload = 1024, dlheader = 58 bytes */
    sz = rem_version==4 ? sizeof(struct dlpacket) : sizeof (struct dlpacket)-3072;

    for (i=0; i < nlandev; i++) {
        if (!ldevsw[i].lan_error) {
            if ((bp = buf_get(makedev(i, 0), sizeof(struct dlpacket), 
                    sizeof(struct ethpacket), &ldevsw[i].lan_bufid)) == 0)
                printf("dsklinit:buffer alloc error\n");
        }
    }
}

dlnodeinit()
{
    register struct buf *dp;
    register struct bpdata *bp;
    
    if (master)
        return;

    dp = diskless_getbuf();
    master_id = bcname;
    master_ip = -1;
    dp->b_swfunc = DLMPOLL;
    dp->b_dev = rootdev;
    rmtstrategy(dp);
    iowait(dp);

    if (dp->b_flags & B_ERROR) {
        switch (dp->b_error) {
        case ENXIO:
            panic("dlmpoll: no answer from master node");
        default:
            panic("root device on master is unavailable");
        }
    }
    
    bp = &bpdata[0];                    /* diskless client has only on struct */
    utsname.ipname = bp->bp_ipname;
    bcopy(bp->bp_nodename, utsname.nodename, 9);
    rmtrootdev = rmtpipedev = bp->bp_rootdev;
    if (rem_version > dp->b_dev)
        rem_version = dp->b_dev;
    if (bdevsw[bmajor(swapdev)].d_open == rmtopen) {
        rmtswapdev = bp->bp_swapdev | Fs2BLK;
        nswap = bp->bp_nswap;
        availsmem = nswap >> 3;
        swplo = bp->bp_swplo;
        if (bp->bp_swapdev == -1)
            ownswap = 0;
        else
            ownswap = 1;
    } else
        ownswap = 1;

    dp->b_swfunc = DLINIT;
    dp->b_dev = rootdev;
    rmtstrategy(dp);
    iowait(dp);
    if (dp->b_flags & B_ERROR) {
        switch (dp->b_error) {
        case ENXIO:
            panic("dlinit: no answer from master node");
        case EAGAIN:
            panic("resources of master low (processes)");
        default:
            panic("root device on master is unavailable");
        }
    }
    
    diskless_relbuf(dp);
    if (bdevsw[bmajor(rootdev)].d_open == rmtopen)
        rootdev |= rmtrootdev & Fs2BLK;

    printf("\nCADMUS Workstation: Node = '%9s'\n\tMaster address = %4x %4x %4x\n",
        utsname.nodename, master_id.hi, master_id.mi, master_id.lo);

    printf("\tRoot: ");
    if (bdevsw[bmajor(rootdev)].d_open == rmtopen)
        printf("remote %2d %2d\n", major(rmtrootdev), minor(rmtrootdev));
    else
        printf("local  %2d %2d\n", major(rootdev), minor(rootdev));

    printf("\tSwap: ");
    if (bdevsw[bmajor(swapdev)].d_open == rmtopen) {
        if (ownswap)
            printf("remote %2d %2d", major(rmtswapdev), minor(rmtswapdev));
        else
            printf("remote, shared with master");
    } else
        printf("local  %2d %2d", major(swapdev), minor(swapdev));
    
    printf(", nswap = %ld, swplo = %ld\n", nswap, swplo);
}

dbd_t swcache[CACHESZ];
short swcachelevel = 0;
short swcachestate;

diskless_swflush()
{
    register struct buf *bp;
    
    if (swcachelevel==0 || swcachestate)
        return;
        
    swcachestate++;
    bp = geteblk();
    bp->b_flags |= B_SWAP|B_BUSY;
    bp->b_swfunc = SWFREE_M32;
    bp->b_blkno = swcachelevel;
    bp->b_dev = swapdev;
    bcopy(swcache, bp->b_un.b_addr, swcachelevel<<2);
    if (swcachestate & 2)
        wakeup((caddr_t)&swcachestate);
    swcachelevel = 0;
    swcachestate = 0;
    rmtstrategy(bp);
    iowait(bp);
    bp->b_dev = -1;
    brelse(bp);
}

diskless_swalloc(nblk)
int nblk;
{
    register struct buf *bp;
    register int i;

    if (nblk == 0)
        return 0;

    if (nblk == 1 && swcachelevel > 0 && swcachestate==0) {
        dlswapaccount += nblk;
        swcachelevel--;
        return ((swcache[swcachelevel].dbd_blkno - 
                 swaptab[comswapsmi].st_swplo) >> 3) + 1;
    } else if (dlswapaccount > CACHESZ && swcachelevel > 10)
        diskless_swflush();

    bp = diskless_getbuf();
    bp->b_swfunc = SWALLOC;
    bp->b_swsize = nblk;
    bp->b_dev = swapdev;
    rmtstrategy(bp);
    iowait(bp);
    i = bp->b_dev;
    diskless_relbuf(bp);
    if (i)
        dlswapaccount += nblk;
    return i;
}

diskless_swfree1(dp)
dbd_t *dp;
{
    dlswapaccount--;
    while (swcachestate) {
        swcachestate |= 2;
        sleep((caddr_t)&swcachestate, PSWP+1);
    }
    
    swcache[swcachelevel] = *dp;
    swcachelevel++;
    if (swcachelevel >= CACHESZ)
        diskless_swflush();
}

int rocache[50];
short rocachelevel = 0;
short rocachestate = 0;

diskless_roflush(cmd)
int cmd;
{
    register struct buf *bp;
    register struct filsys *fsp;
    int *rop;
    int rocnt;

    if (cmd != DLFREE || rocachelevel > 10) {
        while (rocachestate)
            sleep((caddr_t)&rocachestate, PSWP+1);
        if (cmd != DLFREE || rocachelevel > 10) {
            rocachestate = 1;
            bp = geteblk();
            bp->b_flags |= B_SWAP|B_BUSY;
            bp->b_swfunc = cmd;
            if (cmd == DLFREE) {
                rop = &rocache[10];
                rocnt = rocachelevel - 10;
                rocachelevel = 10;
            } else {
                rop = &rocache[0];
                rocnt = rocachelevel;
                rocachelevel = 0;
            }
            bp->b_blkno = rocnt;
            bp->b_dev = rootdev;
            bcopy(rop, bp->b_un.b_addr, rocnt<<2);
            wakeup((caddr_t)&rocachestate);
            rocachestate = 0;
            rmtstrategy(bp);
            iowait(bp);
            if (cmd == DLWCSUP) {
                fsp = mount[0].m_bufp->b_un.b_filsys;
                fsp->s_tfree = bp->b_blkno;
                fsp->s_tinode = bp->b_swret;
            }
            bp->b_swret = -1;
            brelse(bp);
        }
    }
}

daddr_t dlalloc()
{
    register struct buf *bp;
    register int ret;
    int dummy1, dummy2;                 /* relics from former versions */

    if (rocachelevel > 0 && rocachestate==0) {
        rocachelevel--;
        return rocache[rocachelevel];
    }

    while (rocachestate)
        sleep((caddr_t)&rocachestate, PSWP+1);
    if (rocachelevel > 0) {
        rocachelevel--;
        return rocache[rocachelevel];
    }
    rocachestate = 1;
    bp = geteblk();
    bp->b_flags |= B_SWAP|B_BUSY;
    bp->b_swfunc = DLALLOC;
    bp->b_blkno = 10;
    bp->b_dev = rootdev;
    rmtstrategy(bp);
    iowait(bp);
    if (bp->b_blkno > 0) {
        bcopy(bp->b_un.b_addr, &rocache[rocachelevel], bp->b_blkno<<2);
        rocachelevel += bp->b_blkno;

        rocachelevel--;
        ret = rocache[rocachelevel];
    } else
        ret = 0;

    brelse(bp);
    wakeup((caddr_t)&rocachestate);
    rocachestate = 0;
    return ret;
}

dlfree(blk)
int blk;
{
    register struct buf *bp, *hp;
    register int blkno = FsLTOP(rootdev, blk);

redo:
    spl0();
    
    hp = bhash(rootdev,blkno);
    if (hp==0)
        panic("devtab");
    
    for (bp = hp->b_forw; bp != hp; bp = bp->b_forw) {
        if (blkno != bp->b_blkno || bp->b_dev != rootdev || (bp->b_flags & B_STALE))
            continue;
        spldisk();
        if (bp->b_flags & B_BUSY) {
            bp->b_flags |= B_WANTED;
            syswait.iowait++;
            sleep((caddr_t)bp, PRIBIO+1);
            syswait.iowait = 0;
            goto redo;
        }
        spl0();
        bp->b_flags &= ~(B_STALE|B_DELWRI|B_ASYNC|B_ERROR);
        break;
    }
    do {
        while (rocachestate)
            sleep((caddr_t)&rocachestate, PSWP+1);

        while (rocachelevel >= 40)
            diskless_roflush(DLFREE);
    } while (rocachestate);

    rocache[rocachelevel] = blk;
    rocachelevel++;
}

ino_t dlialloc()
{
    register struct buf *bp = diskless_getbuf();
    ino_t inum;
    
    bp->b_swfunc = DLIALLOC;
    bp->b_dev = rootdev;
    rmtstrategy(bp);
    iowait(bp);
    
    inum = bp->b_blkno;
    diskless_relbuf(bp);
    return inum;
}

dlwcsup()
{
    diskless_roflush(panicstr ? DLSHUTDOWN : DLWCSUP);
}

short cacheino;
time_t cacheta, cachetm, cachetc;

struct inode *dliread(ip)
register struct inode *ip;
{
    register struct buf *bp;
    register struct dinode *dip;
    register int i;
    char *iaddr, *diaddr;
    
    bp = geteblk();
    bp->b_flags |= B_SWAP|B_BUSY;
    bp->b_swfunc = DLIREAD;
    bp->b_blkno = ip->i_number;
    bp->b_dev = rootdev;
    rmtstrategy(bp);
    iowait(bp);
    if (bp->b_blkno == 0) {
        iput(ip);
        u.u_error = EIO;
        ip = 0;
    } else {
        dip = bp->b_un.b_dino;
        ip->i_mode = dip->di_mode;
        ip->i_nlink = dip->di_nlink;
        ip->i_uid = dip->di_uid;
        ip->i_gid = dip->di_gid;
        ip->i_size = dip->di_size;
        iaddr = (char*)ip->i_addr;
        diaddr = dip->di_addr;
        for (i=0; i < 13; i++) {
            *iaddr++ = 0;
            *iaddr++ = *diaddr++;
            *iaddr++ = *diaddr++;
            *iaddr++ = *diaddr++;
        }
        if ((ip->i_mode & IFMT)==IFIFO) {
            ip->i_frptr = ip->i_fwptr = 0;
            ip->i_frcnt = ip->i_fwcnt = 0;
        }
        cacheino = ip->i_number;
        cacheta = dip->di_atime;
        cachetm = dip->di_mtime;
        cachetc = dip->di_ctime;
    }
    
    bp->b_dev = -1;
    brelse(bp);
    return ip;
}

dliupdat(dip, ip, ta, tm)
register struct dinode *dip;
register struct inode *ip;
time_t *ta, *tm;
{
    register struct buf *bp = geteblk();
    int *dp;
    
    bp->b_flags |= B_SWAP|B_BUSY;
    bp->b_swfunc = DLIUPDAT;
    bp->b_blkno = ip->i_number;
    bp->b_dev = rootdev;
    dp = bp->b_un.b_words;
    *dp++ = *ta;
    *dp++ = *tm;
    *dp++ = ip->i_flag;
    ip->i_flag &= ~(ISYN|ICHG|IACC|IUPD);
    bcopy(dip, dp, sizeof(struct dinode));
    rmtstrategy(bp);
    iowait(bp);
    bp->b_dev = -1;
    brelse(bp);
    if (cacheino == ip->i_number)
        cacheino = -1;
}

dlstat(inum, tm)
ino_t inum;
time_t *tm; /* 3 elements atime,mtime,ctime */
{
    register struct buf *bp;
    register struct dinode *dip;

    if (inum == cacheino) {
        *tm++ = cacheta;
        *tm++ = cachetm;
        *tm = cachetc;
    } else {
        bp = geteblk();
        bp->b_flags |= B_SWAP|B_BUSY;
        bp->b_swfunc = DLIREAD;
        bp->b_blkno = inum;
        bp->b_dev = rootdev;
        rmtstrategy(bp);
        iowait(bp);
        dip = bp->b_un.b_dino;
        *tm++ = dip->di_atime;
        *tm++ = dip->di_mtime;
        *tm = dip->di_ctime;

        bp->b_dev = -1;
        bp->b_flags |= ISYN;
        brelse(bp);
    }
}

int onmaster(ip)
register struct inode *ip;
{
    register ino_t *inop = minolist;

    if (master || ip->i_dev != rootdev)
        return 0;

    while (inop < &minolist[10]) {
        if (ip->i_number == *inop++)
            return 1;
    }
    return 0;
}

int masterdev()
{
    register int node = uifindnode3(master_ip);
    return node >= 0 ? uinode[node].nd_dev : -1;
}