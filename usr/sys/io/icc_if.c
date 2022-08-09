/* PCS specific */
static char* _Version = "@(#) RELEASE:  2.0  Oct 20 1986 /usr/sys/io/icc_if.c";

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
#include "sys/icc/ifctrl.h"

static int icc = 0; /* dev# of icc to serve */

#define IF_WAITLOCK 0x08
#define IF_LOCK     0x40
extern int if_flag;

union request if_request;
union reply if_reply;
short if_initflg;

/* note this is also defined as global in icc_is.c and icc_iw.c */
char mfblk[MFBLKSIZE];
paddr_t mfblk_paddr;


if_lock()
{
    while (if_flag & IF_WAITLOCK) {
        if_flag |= IF_LOCK;
        sleep(&if_flag, 1);
    }
    if_flag |= IF_WAITLOCK;
}

if_rele()
{
    if (if_flag & IF_LOCK) {
        wakeup(&if_flag);
        if_flag &= ~IF_LOCK;
    }
    if_flag = 0;
}

if_init()
{
    /* only floppy at icc0 */
    if (icc_initflg[0]==0)
        return;

    mfblk_paddr = logtophys(mfblk);

    if_request.ioctl_cmd.cmd = SC_DOINIT;
    if_request.ioctl_cmd.dev = IF_ID;
    if_request.ioctl_cmd.rxaddr = logtophys(&if_reply);
    if_reply.ok_reply.code = SCSI_SHORT_REPLY;
    Rpc(icc, SCSI_IF, SCSI_SYNC, &if_request, WAIT);
    if (if_reply.ok_reply.code == SCSI_OK) {
        printf("ICC floppy initialized\n");
        if_initflg++;
    }
}

if_open(dev, mode)
{
    if ((dev >> 4) > 1 || icc_initflg[0]==0) {
        u.u_error = ENXIO;
        return;
    }

    if_lock();
    if_request.rw_cmd.cmd = SC_OPEN;
    if_request.rw_cmd.dev = dev + IF_ID;
    if_request.rw_cmd.bp = 0;
    Rpc(icc, SCSI_IF, SCSI_SYNC, &if_request, WAIT);
    if (if_getsta(&if_reply))
        u.u_error = ENXIO;
    if_rele();
}

if_strategy(bp)
register struct buf *bp;
{
    if_lock();
    bp->av_forw = 0;
    if_request.rw_cmd.bp = bp;
    if_request.rw_cmd.dev = minor(bp->b_dev) + IF_ID;
    if_request.rw_cmd.bno = bp->b_blkno;
    if_request.rw_cmd.count = bp->b_bcount;
    if_request.rw_cmd.paddr = bp->b_paddr;
    if_request.rw_cmd.cmd = (bp->b_flags & B_READ) ? SC_READ : SC_WRITE;
    Rpc(icc, SCSI_IF, SCSI_ASYNC, &if_request, WAIT);
    if_rele();
}

if_getsta(rp)
union reply *rp;
{
    register struct buf *bp = rp->ok_reply.bp;
    short ret = 0;
    
    if (rp->ok_reply.code == SCSI_OK)
        goto done;

    if (rp->err_reply.code == SCSI_ERROR) {
        ret++;
        printf("\nIF error on floppy %d: ", rp->err_reply.disk);
        printf("code: 0x%x = ", rp->err_reply.error_code);
        printf("'%s' ", rp->err_reply.message);
        if (rp->err_reply.blkno >= 0)
            printf("block: %ld", rp->err_reply.blkno);
        printf("\n");
    } else {
        if (rp->err_reply.code == 1)
            ret++;
        else
            printf("bad if_reply %d\n", rp->err_reply.code);
    }
    
done:
    if (bp) {
        if (ret) {
            bp->b_flags |= B_ERROR;
            bp->b_resid = bp->b_bcount;
        } else
            bp->b_resid = 0;
        iodone(bp);
    }
    return ret;
}

if_read(dev)
dev_t dev;
{
    physio(if_strategy, NULL, dev, /*read*/1);
}

if_write(dev)
dev_t dev;
{
    physio(if_strategy, NULL, dev, /*write*/0);
}

if_ioctl(dev, cmd, arg)
{
    short val;
    if (cmd != IFCTRL) {
        u.u_error = ENXIO;
        return;
    }
    
    if (copyin(arg, &val, sizeof(short)) != 0) {
        u.u_error = EFAULT;
        return;
    }

    if_lock();

    if_request.ioctl_cmd.cmd = SC_IOCTL;
    if_request.ioctl_cmd.dev = dev + IF_ID;
    if_request.ioctl_cmd.ioctl_cmd = val;
    if_request.ioctl_cmd.rxaddr = mfblk_paddr;
    Rpc(icc, SCSI_IF, SCSI_SYNC, &if_request, WAIT);

    if (if_getsta(&if_reply))
        u.u_error = EIO;

    if (val == IF_ERRREPORT) {
        if (copyout(mfblk, arg, MFBLKSIZE) != 0)
            u.u_error = EFAULT;
    }
    if_rele();
}

if_intr(dev, up)
register UNIX_MESSAGE *up;
{
    if (up->id[1] == 1)
        if_getsta((union reply*)&up->data[0]);
    else if (up->id[1] == 2)
        if_getsta(&if_reply);
    else
        printf("if_intr: strange id[1]\n");
}
