/* PCS specific */
static char* _Version = "@(#) RELEASE:  2.0  Oct 20 1986 /usr/sys/io/icc_is.c";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/page.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"

#include "sys/icc/types.h"
#include "sys/icc/pikitor.h"
#include "sys/icc/unix_icc.h"
#include "sys/icc/icc_scsi.h"
#include "sys/icc/isctrl.h"

static int icc = 0; /* dev# of icc to serve */

#define IS_WAITLOCK 0x08
#define IS_LOCK     0x40
extern int is_flag;

union request is_request;
union reply is_reply;
short is_initflg;

char mfblk[MFBLKSIZE];
paddr_t mfblk_paddr;

int is_isopen;
int is_eof;

extern int icc_initflg[];

is_lock()
{
    while (is_flag & IS_WAITLOCK) {
        is_flag |= IS_LOCK;
        sleep(&is_flag, 1);
    }
    is_flag |= IS_WAITLOCK;
}

is_rele()
{
    if (is_flag & IS_LOCK) {
        wakeup(&is_flag);
        is_flag &= ~IS_LOCK;
    }
    is_flag = 0;
}

is_init()
{
    /* only one streamer at icc0 */
    if (icc_initflg[0]==0)
        return;

    mfblk_paddr = logtophys(mfblk);
    
    is_request.ioctl_cmd.cmd = SC_DOINIT;
    is_request.ioctl_cmd.dev = IS_ID;
    is_request.ioctl_cmd.rxaddr = logtophys(&is_reply);
    is_reply.ok_reply.code = SCSI_SHORT_REPLY;
    Rpc(icc, SCSI_IS, SCSI_SYNC, &is_request, WAIT);
    if (is_reply.ok_reply.code == SCSI_OK) {
        printf("ICC streamer initialized\n");
        is_initflg++;
    }
}

is_open(dev, mode)
{
    int iccno = dev >> 4;
    if (iccno > 0 || is_isopen || icc_initflg[0]==0) {
        u.u_error = ENXIO;
        return;
    }
    is_eof = 0;
    is_lock();
    is_request.rw_cmd.cmd = SC_OPEN;
    is_request.rw_cmd.dev = dev + IS_ID;
    is_request.rw_cmd.count = mode & 2;
    is_request.rw_cmd.bp = 0;
    Rpc(icc, SCSI_IS, SCSI_SYNC, &is_request, WAIT);
    if (is_getsta(&is_reply))
        u.u_error = ENXIO;
    else
        is_isopen++;
    is_rele();
}

is_close(dev, mode)
{
    is_lock();
    is_request.rw_cmd.cmd = SC_CLOSE;
    is_request.rw_cmd.dev = dev + IS_ID;
    is_request.rw_cmd.bp = 0;
    is_request.rw_cmd.count = mode & 2;
    Rpc(icc, SCSI_IS, SCSI_SYNC, &is_request, WAIT);
    if (is_getsta(&is_reply))
        u.u_error = ENXIO;
    is_rele();
    is_isopen = 0;
}

is_strategy(bp)
register struct buf *bp;
{
    is_lock();
    bp->av_forw = 0;
    is_request.rw_cmd.bp = bp;
    is_request.rw_cmd.dev = minor(bp->b_dev) + IS_ID;
    is_request.rw_cmd.count = bp->b_bcount;
    is_request.rw_cmd.paddr = bp->b_paddr;
    is_request.rw_cmd.cmd = (bp->b_flags & B_READ) ? SC_READ : SC_WRITE;
    Rpc(icc, SCSI_IS, SCSI_SYNC, &is_request, WAIT);
    is_getsta(&is_reply);
    is_rele();
}

is_getsta(rp)
union reply *rp;
{
    register struct buf *bp = rp->ok_reply.bp;
    short ret = 0;

    if (bp)
        bp->b_resid = 0;
    
    if (rp->ok_reply.code == SCSI_OK)
        ;
    else if (rp->err_reply.code == SCSI_ERROR) {
        if (rp->err_reply.error_code == 0x1c && bp != 0) {
            bp->b_resid = rp->err_reply.blkno << 9;
            if (bp->b_resid != bp->b_bcount)
                is_eof++;
            goto done;
        }
        ret++;
        printf("\nIS streamer: ");
        printf("code: 0x%x = ", rp->err_reply.error_code);
        printf("'%s' ", rp->err_reply.message);
        if (rp->err_reply.blkno >= 0)
            printf("residue: %ld blocks", rp->err_reply.blkno);
        printf("\n");
    } else if (rp->err_reply.code == 1)
        ret++;
    else
        printf("bad is_reply %d\n", rp->err_reply.code);
    
    if (ret && bp != NULL)
        bp->b_flags |= B_ERROR;

done:
    if (bp != NULL)
        iodone(bp);
    
    return ret;
}

is_read(dev)
dev_t dev;
{
    if (is_eof) {
        is_eof = 0;
        return;
    }
    physio(is_strategy, NULL, dev, /*read*/1);
}

is_write(dev)
dev_t dev;
{
    physio(is_strategy, NULL, dev, /*write*/0);
}

is_ioctl(dev, cmd, arg)
dev_t dev;
short cmd;
{
    short val;

    if (cmd != IS_IOCTL) {
        u.u_error = EINVAL;
        return;
    }

    if (copyin(arg, &val, sizeof(short)) != 0) {
        u.u_error = EFAULT;
        return;
    }

    is_lock();
    
    is_request.ioctl_cmd.cmd = SC_IOCTL;
    is_request.ioctl_cmd.dev = dev + IS_ID;
    is_request.ioctl_cmd.ioctl_cmd = val;
    is_request.ioctl_cmd.rxaddr = mfblk_paddr;
    Rpc(icc, SCSI_IS, SCSI_SYNC, &is_request, WAIT);

    if (is_getsta(&is_reply))
        u.u_error = EIO;
    
    if (val == IS_ERRREPORT || val == IS_MODESENSE) {
        if (copyout(mfblk, arg, MFBLKSIZE) != 0)
            u.u_error = EFAULT;
    }
    is_rele();
}

is_intr()
{
    printf("is_intr should not be called!\n");
}
