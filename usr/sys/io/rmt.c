/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.5  Feb 05 1987 /usr/sys/io/rmt.c";

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
#include "sys/munet/munet.h"
#include "sys/munet/mnbuf.h"
#include "sys/munet/diskless.h"
#include "sys/munet/bpdata.h"

char rem_version;
struct buf rmttab;

extern caddr_t buf_siz();
extern rmtintr(), rmtdone();

rmtopen(/*argsignored*/)
{}

rmtclose(/*argsignored*/)
{}

rmtstrategy(bp)
struct buf *bp;
{
    if ((bp->b_flags & B_SWAP)==0 && bp->b_dev != rootdev && bp->b_dev != swapdev) {
        printf("rmtstrategy: illegal device code %x\n", bp->b_dev);
        u.u_error = ENXIO;
        iodone(bp);
        return;
    }

    bp->b_dlindex = 0;
    bp->b_flags &= ~(B_DONE|B_ERROR);
    bp->b_actf = 0;
    
    spldisk();
    
    if (rmttab.b_actf == 0)
        rmttab.b_actf = bp;
    else
        rmttab.b_actl->b_actf = bp;
    rmttab.b_actl = bp;

    if ((rmttab.b_flags & B_BUSY)==0) {
        rmttab.b_flags |= B_BUSY;
        rmttab.b_active = 0;
        rmttab.b_errcnt = 0;
        rmttab.b_seqno++;
        if (rmttab.b_seqno > 9999)
            rmttab.b_seqno = 1;
        rmtstart(bp);
    }
    spl0();
    return;
}

rmtstart(bp)
register struct buf *bp;
{
    register struct dlpacket *dlp;
    register struct ldevsw *lp;
    register dev_t dev;
    register sz;
    caddr_t tmp;
    int bufsiz;

    dev = bp->b_dev == rootdev ? rmtrootdev : rmtswapdev;
        
    if ((bp->b_flags & (B_SWAP|B_READ))==0 ||
        bp->b_swfunc == DLIUPDAT ||
        bp->b_swfunc == DLFREE ||
        bp->b_swfunc == DLALLOC ||
        bp->b_swfunc == DLWCSUP ||
        bp->b_swfunc == DLSHUTDOWN ||
        bp->b_swfunc == SWFREE_M32) {
            if ((bp->b_flags & B_CTRL) && rem_version==4)
                bufsiz = sizeof(struct dlpacket);
            else
                bufsiz = sizeof(struct dlpacket)
                    - (UIMAXDATA_NEW_FRAG - UIMAXDATA_OLD);
    } else
        bufsiz = sizeof(struct dlpacket) - UIMAXDATA_NEW_FRAG;

    lp = &ldevsw[0];
    
    tmp = buf_siz(lp->lan_bufid, bufsiz);
    dlp = (struct dlpacket*)tmp;

    dlp->e_dest = master_id;
    dlp->rt_type = RTTYPE;
    dlp->rt_index = rmttab.b_dlindex;
    dlp->rt_seqno = rmttab.b_seqno;
    if (bp->b_flags & B_SWAP) {
        dlp->rt_command = bp->b_swfunc;
        switch (bp->b_swfunc) {
        case SWALLOC:
            dlp->rt_size = bp->b_swsize;
            break;
        case SWFREE_M32:
            dlp->rt_size = bp->b_blkno;
            bcopy(bp->b_un.b_addr, dlp->rt_data, bp->b_blkno << 2);
            dlp->rt_dev = dev;
            break;
        case SWALLOCCAT:
        case SWFREE:
            dlp->rt_size = bp->b_swsize;
            dlp->rt_start = bp->b_swstart;
            break;
        case SWALLOCCL:
            dlp->rt_size = bp->b_swsize;
            break;
        case DLINIT:
            dlp->rt_dev = dev;
            dlp->rt_size = rmtswapdev;
            break;
        case DLMPOLL:
            break;
        case DLIALLOC:
            dlp->rt_dev = dev;
            break;
        case DLALLOC:
            dlp->rt_size = bp->b_blkno;
            dlp->rt_dev = dev;
            break;
        case DLWCSUP:
        case DLSHUTDOWN:
            dlp->rt_dev = dev;
            dlp->rt_size = bp->b_blkno;
            bcopy(bp->b_un.b_addr, dlp->rt_data, bp->b_blkno << 2);
            break;
        case DLIUPDAT:
            /* 76=? Shouldn't this be sizeof(struct dinode)? */
            bcopy(bp->b_un.b_addr, dlp->rt_data, 76); 
            /*FALLTHRU*/
        case DLIFREE:
        case DLIREAD:
            dlp->rt_dev = dev;
            dlp->rt_blkno = bp->b_blkno;
            break;
        case DLFREE:
            dlp->rt_dev = dev;
            dlp->rt_size = bp->b_blkno;
            bcopy(bp->b_un.b_addr, dlp->rt_data, bp->b_blkno << 2);
            break;
        default:
            printf("rmtstart: bad swap cmd = %d\n", bp->b_swfunc);
        }
    } else {
        dlp->rt_dev = dev;
        dlp->rt_blkno = 
        FsPTOL(bp->b_dev, bp->b_blkno) +
        (rmttab.b_active >> FsBSHIFT(bp->b_dev))
            ;
        dlp->rt_pageio = (bp->b_flags & B_PAGEIO) ? 1 :
            ((dev==rmtswapdev && 
             ((swplo+1)<<3)<=dlp->rt_blkno) != 0 ? 2 : 0);
        
        sz = rem_version==4 ? UIMAXDATA_NEW_FRAG :
                FsBSIZE(bp->b_dev);
        
        if (sz > bp->b_bcount)
            sz = bp->b_bcount;

        dlp->rt_size = sz;
        if (bp->b_flags & B_READ)
            dlp->rt_command = RMTRBLK;
        else {
            dlp->rt_command = RMTWBLK;
            buf_copy(lp->lan_bufid, dlp->rt_data, 
                     bp->b_un.b_addr+rmttab.b_active,
                     sz, 1, 0);
        }
    }
    
    rmttab.b_flags &= ~B_DONE;
    St_write("rmtstart_NFSD2", 7, 1, dlp->rt_command,
             15, RTTYPE);
    St_write("rmtstart_RPCD3", 8, 1, dlp->rt_type,
             15);
    timeout(rmtintr, rmttab.b_seqno, hz*6);
    fr_trans(lp->lan_bufid, master_ip, 0x800, rmtdone);
}

rmtdone()
{
    return 0;
}

rmtintr(seqno, bufid)
ushort bufid;
{
    register struct buf *bp;
    register sz;
    register s = spldisk();
    bufque *qp;
    struct dlpacket *dlp;

    bp = rmttab.b_actf;
    if (bp==0) {
        splx(s);
        return;
    }

    if (seqno) {
        if (!(rmttab.b_flags&B_DONE) && seqno==rmttab.b_seqno) {
            if (rmttab.b_errcnt++ >= 6) {
                bp->b_flags |= B_ERROR;
                bp->b_error = ENXIO;
                printf("rmt: no answer from master\n");
                St_write("rmtintr_RPCD5", 8, 2, -1, 20);
                goto done;
            }
            St_write("rmtintr_RPCD4", 8, 1, -1, 19);
            rmtstart(bp);
        }
        splx(s);
        return;
    }

    qp = buf_pointer(bufid);
    dlp = (struct dlpacket*)qp->bq_pkptr;
    rmttab.b_flags |= B_DONE;
    cancelto(rmtintr, rmttab.b_seqno);
    St_write("rmtintr_NFSD3", 7, 2, dlp->rt_command, 15, RTTYPE);
    switch (dlp->rt_command) {
    case SWALLOC:
    case SWALLOCCAT:
        bp->b_dev = dlp->rt_return;
        break;
    case SWALLOCCL:
        bp->b_dev = dlp->rt_return;
        bp->b_bcount = dlp->rt_size;
        break;
    case DLINIT:
        rmttab.b_dlindex = dlp->rt_return;
        time = dlp->rt_blkno;
        if (haveclock)
            write_clock();
        swapdev |= FsLRG(dlp->rt_dev);
        break;
    case DLMASTER:
        bp->b_dev = dlp->rt_return;
        master_id = dlp->e_source;
        master_ip = dlp->ip_srcaddr;
        bcopy(dlp->rt_data, bpdata, sizeof(struct bpdata));
        break;
    case SWFREE:
    case SWFREE_M32:
        break;
    case RMTRBLK:
    case RMTWBLK:
        sz = dlp->rt_size;
        if (dlp->rt_command==RMTRBLK)
            buf_copy(bufid, dlp->rt_data,
                &bp->b_un.b_addr[rmttab.b_active],
                sz, 1, 1);
        break;
    case DLALLOC:
        bp->b_blkno = dlp->rt_size;
        bcopy(dlp->rt_data, bp->b_un.b_addr, bp->b_blkno<<2);
        break;
    case DLIALLOC:
        bp->b_blkno = dlp->rt_blkno;
        break;
    case DLFREE:
    case DLIFREE:
    case DLIUPDAT:
        break;
    case DLIREAD:
        bcopy(dlp->rt_data, bp->b_un.b_dino, sizeof(struct dinode));
        /*FALLTHRU*/
    case DLWCSUP:
    case DLSHUTDOWN:
        bp->b_blkno = dlp->rt_blkno;
        bp->b_dev = dlp->rt_return;
        break;
    default:
        printf("rmtintr: bad cmd = %d\n", dlp->rt_command);
    }
    if ((bp->b_error = dlp->rt_error) != 0) {
        bp->b_flags |= B_ERROR;
        St_write("rmtintr_RPCD8", 8, 2, -1, 20);
    }
done:
    if ((bp->b_flags & B_ERROR) ||
          (bp->b_flags & B_SWAP) ||
          (rmttab.b_active += sz) >= bp->b_active) {
        bp->b_errcnt = bp->b_active - rmttab.b_active;
        rmttab.b_actf = bp->b_actf;
        iodone(bp);
        if (rmttab.b_actf==0) {
            rmttab.b_actl = 0;
            rmttab.b_flags &= ~B_BUSY;
            rmttab.b_seqno++;
            if (rmttab.b_seqno > 9999)
                rmttab.b_seqno = 1;
        }
    }
    
    if (rmttab.b_actf) {
        if (rmttab.b_actf != bp)
            rmttab.b_bcount = 0;
        rmttab.b_errcnt = 0;
        rmttab.b_seqno++;
        if (rmttab.b_seqno > 9999)
            rmttab.b_seqno = 1;
        rmtstart(rmttab.b_actf);
    }
    splx(s);
}
