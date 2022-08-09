/* PCS specific */
static char *_Version = "@(#) RELEASE:  3.2  Jan 29 1987 /usr/virt/src/dev/sdlc.c";

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
#include "sys/utsname.h"
#include "sys/icc/unix_icc.h"

/* opcodes to Icc_command */
#define SDLC_GETPARAM   1
#define SDLC_ENDPARAM   2
#define SDLC_READ       3
#define SDLC_ENDREAD    4
#define SDLC_WRITE      5
#define SDLC_ENDWRITE   6
#define SDLC_CLOSE      7
#define SDLC_GETSTATUS  9
#define SDLC_GETXID     10
#define SDLC_SETXID     11
#define SDLC_PROBE      0x88

/* SDLC wait flags */
#define SDLC_WAIT(flg) (((caddr_t)&icc)+flg)
#define W_READDONE      0
#define W_WRITEDONE     1
#define W_PARAMDONE     2
#define W_CLOSE         3
#define W_4             4
#define W_STATUSDONE    5
#define W_TIMEOUT       6
#define W_GETXIDDONE    7
#define W_SETXIDDONE    8

/* SDLC errors (101-...) */
#define SERR_NOERROR    0
#define SERR_FRAMESHORT 101
#define SERR_CFIELD     103
#define SERR_FRAMELONG  104
#define SERR_NRINFRAME  105
#define SERR_TXTIMEOUT  106
#define SERR_RXTIMEOUT  107
#define SERR_RXOVRUN    108
#define SERR_RXUNDRUN   109
#define SERR_CRC        110
#define SERR_ABORT      111
#define SERR_NOCARRIER  112
#define SERR_LINEERR    113
#define SERR_NOBUF      114
#define SERR_FRAMEEMPTY 115
#define SERR_FRAMELOST  121


#define SDLC_BUFSIZE    4100
struct sdlcbuf {
    short len;
    char data[SDLC_BUFSIZE];
} txbuf, rxbuf;

struct sdlc {
    short open;
    short doclose;
    short wroffset;
} sdlc;

struct xid {
    char xid[48];
} xid_read, xid_write;

static short sdlc_t_errors;
static short sdlc_r_errors;

#define SDLC_ERROR  0x80

struct icc {
    short wrstat;
    short wrerr;
    short rdstat;
    short rderr;
} icc;

struct iccdev {
    short hi;
    short mi;
    short lo;
    short cmd;
};

struct sdlcparams {
    char params[122];
} sdlc_params;

struct sdlcstatus {
    char status[48];
} sdlc_status;

static short sdlc_notrunning[] = { 1, 1, 1, 1 };
static short sdlc_read_sleep;
static short sdlc_write_sleep;
static short sdlc_read_done;
static short sdlc_write_done;
static short check_icc_sw;

extern no_icc();
static sdlctout(), Icc_command(), sdlc_error();

sdlcopen(dev)
{
    register short unit = minor(dev);

    if (unit < 0 || unit > 3) {
        u.u_error = ENODEV;
        return;
    }
    
    if (sdlc_notrunning[unit]) {
        u.u_error = ENODEV;
        return;
    }

    if (no_icc(unit)) {
        u.u_error = ENODEV;
        return;
    }

    if (sdlc.open == 0) {
        sdlc.open = 1;
        sdlc.doclose = 0;
        sdlc_r_errors = 0;
        sdlc_t_errors = 0;
        sdlc_read_sleep = 0;
        sdlc_write_sleep = 0;
        sdlc_read_done = 0;
        sdlc_write_done = 0;
    }
}

sdlcclose(dev)
{
    register short unit = minor(dev);
    
    if (sdlc_notrunning[unit]) {
        u.u_error = ENODEV;
        return;
    }
    
    if (sdlc.doclose == 1) {
        Icc_command(unit, SDLC_CLOSE, 0);
        sleep(SDLC_WAIT(W_CLOSE), PPIPE);
        sdlc.doclose = 0;
        sdlc.open = 0;
        sdlc_read_sleep = sdlc_write_sleep = 0;
    }
}

sdlcread(dev)
{
    register short i;
    register short rc; /*return from sleep */
    register short unit = minor(dev);
    int cpl;

    if (sdlc_notrunning[unit]) {
        u.u_error = ENODEV;
        return;
    }

    Icc_command(unit, SDLC_READ, &rxbuf);
    
    cpl = currpl();
    spltimer();

    sdlc_read_sleep = 1;
      while (!sdlc_read_done)
          rc = sleep(SDLC_WAIT(W_READDONE), PPIPE);
      sdlc_read_sleep = 0;
      sdlc_read_done = 0;
    splx(cpl);

    if (sdlc_r_errors) {
        sdlc_error(icc.rderr);
        sdlc_r_errors = 0;
        u.u_error = icc.rderr + 100;    /* rescale */
        icc.rderr = icc.rdstat = 0;
        Icc_command(unit, SDLC_ENDREAD, &rxbuf);
        return;
    }
    
    if (rc == 0) {
        i = rxbuf.len;
        i =  u.u_count > i ? i : u.u_count;
        if ((i & 1) != 0 && i < (short)u.u_count) {
            i++; u.u_count++;
        }
        iomove(&rxbuf.data[sdlc.wroffset], i, 1);
        sdlc.wroffset += i;
        if ((rxbuf.len -= i) > 0) {
            Icc_command(unit, SDLC_ENDREAD, &rxbuf);
            icc.rderr = icc.rdstat = 0;
            return;
        }
    } else
        u.u_error = EINTR;

    sdlc.wroffset = 0;
    Icc_command(unit, SDLC_ENDREAD, &rxbuf);
    icc.rderr = icc.rdstat = 0;
}

sdlcwrite(dev)
{
    register short i;
    register short rc;
    register short unit = minor(dev);
    int cpl;
    short wrerr;

    if (sdlc_notrunning[unit]) {
        u.u_error = ENODEV;
        return;
    }

    if (u.u_count > SDLC_BUFSIZE) {
        u.u_error = EIO;
        return;
    }

    i = u.u_count > SDLC_BUFSIZE ? SDLC_BUFSIZE : u.u_count;
    txbuf.len = i;
    if (i & 1) {
        i++;
        u.u_count++;
    }
    iomove(txbuf.data, i, 0);

    cpl = currpl();
    spltimer();
      Icc_command(unit, SDLC_WRITE, &txbuf);
    
      sdlc_write_sleep = 1;
      while (!sdlc_write_done)
          rc = sleep(SDLC_WAIT(W_WRITEDONE), PPIPE-4);
      sdlc_write_done = 0;
      sdlc_write_sleep = 0;
      wrerr = icc.wrerr;
      icc.wrerr = icc.wrstat = 0;
      Icc_command(unit, SDLC_ENDWRITE, &txbuf);
    splx(cpl);

    if (sdlc_t_errors) {
        sdlc_error(icc.wrerr); /* this was cleared above, so "unknown error 0" will be printed */
        sdlc_t_errors = 0;
        u.u_error = wrerr + 100;
    }
    
    if (rc)
        u.u_error = EINTR;
}

sdlcint()
{

    clrcache();
    
    sdlc_t_errors = icc.wrstat & SDLC_ERROR;
    sdlc_r_errors = icc.rdstat & SDLC_ERROR;
    icc.wrstat &= ~SDLC_ERROR;
    icc.rdstat &= ~SDLC_ERROR;

    if (icc.wrstat == SDLC_ENDPARAM) {
        wakeup(SDLC_WAIT(W_PARAMDONE));
        return;
    }
    if (icc.rdstat == SDLC_READ) {
        sdlc_read_done = 1;
        if (sdlc_read_sleep)
            wakeup(SDLC_WAIT(W_READDONE));
        return;
    }
    if (icc.wrstat == SDLC_WRITE) {
        sdlc_write_done = 1;
        if (sdlc_write_sleep)
            wakeup(SDLC_WAIT(W_WRITEDONE));
        return;
    }
    if (icc.wrstat == SDLC_CLOSE) {
        wakeup(SDLC_WAIT(W_CLOSE));
        return;
    }
    if (icc.wrstat == SDLC_GETPARAM) {
        wakeup(SDLC_WAIT(W_4));
        return;
    }
    if (icc.wrstat == SDLC_GETXID) {
        wakeup(SDLC_WAIT(W_GETXIDDONE));
        return;
    }
    if (icc.wrstat == SDLC_SETXID) {
        wakeup(SDLC_WAIT(W_SETXIDDONE));
        return;
    }
    if (icc.wrstat == SDLC_GETSTATUS) {
        wakeup(SDLC_WAIT(W_STATUSDONE));
        return;
    }
    printf("sdlc: Interrupt ignored\007\n");
}

sdlcfatal()
{
    sdlc_notrunning[0] = 1;
    sdlc_notrunning[1] = 1;
    sdlc_notrunning[2] = 1;
    sdlc_notrunning[3] = 1;

    printf(" ICC-device for SDLC is down. Please load down again\n");

    if (sdlc_write_sleep) {
        sdlc_t_errors = 16;
        icc.wrstat = SDLC_ERROR|SDLC_WRITE;
        icc.wrerr = 16;
        sdlc_write_done = 1;
        wakeup(SDLC_WAIT(W_WRITEDONE));
    }

    if (sdlc_read_sleep) {
        sdlc_r_errors = 16;
        icc.rdstat = SDLC_ERROR|SDLC_READ;
        icc.rderr = 16;
        sdlc_read_done = 1;
        wakeup(SDLC_WAIT(W_READDONE));
    }
}

sdlcioctl(dev, func, arg)
short dev, func;
{
    register short unit = minor(dev);
    
    if (sdlc_notrunning[unit]) {
        u.u_error = ENODEV;
        return;
    }
    
    switch (func) {
    case 7:
        break;

    case 1: /* send parameters to device */
        Icc_command(unit, SDLC_GETPARAM, &sdlc_params);
        timeout(sdlctout, 0, hz / 10);
        sleep(SDLC_WAIT(W_TIMEOUT), PPIPE);
        
        Icc_command(unit, SDLC_ENDPARAM, &icc);
        sleep(SDLC_WAIT(W_PARAMDONE), PPIPE);
        if (sdlc_t_errors) {
            sdlc_error(icc.wrerr);
            sdlc_t_errors = 0;
            u.u_error = icc.wrerr + 100;
            break;
        }
        sdlc.open = 1;
        sdlc.doclose = 0;
        break;

    case 2: /* read parameters from driver */
        if (copyout(&sdlc_params, arg, sizeof(struct sdlcparams)) != 0)
            u.u_error = EIO;
        break;

    case 3: /* store parameters into driver */
        if (copyin(arg, &sdlc_params, sizeof(struct sdlcparams)) != 0)
            u.u_error = EIO;
        break;

    case 4: /* read status */
        Icc_command(unit, SDLC_GETSTATUS, &sdlc_status);
        sleep(SDLC_WAIT(W_STATUSDONE), PPIPE);
        if (sdlc_t_errors) {
            sdlc_error(icc.wrerr);
            sdlc_t_errors = 0;
            u.u_error = icc.wrerr + 100;
        }
        if (copyout(&sdlc_status, arg, sizeof(struct sdlcstatus)) != 0)
            u.u_error = EIO;
        break;

    case 5:
        break;

    case 6: /* send CLOSE to device on closing */
        sdlc.doclose = 1;
        break;

    case 8: /* read xid from device */
        Icc_command(unit, SDLC_GETXID, &xid_read);
        sleep(SDLC_WAIT(W_GETXIDDONE), PPIPE);
        if (sdlc_t_errors) {
            sdlc_error(icc.wrerr);
            sdlc_t_errors = 0;
            u.u_error = icc.wrerr + 100;
        }
        if (copyout(&xid_read, arg, sizeof(struct xid)) != 0)
            u.u_error = EIO;
        break;

    case 9: /* set xid for device and send it */
        if (copyin(arg, &xid_write, sizeof(struct xid)) != 0) {
            u.u_error = EIO;
            return;
        }
        Icc_command(unit, SDLC_SETXID, &xid_write);
        sleep(SDLC_WAIT(W_SETXIDDONE), PPIPE);
        if (sdlc_t_errors) {
            sdlc_error(icc.wrerr);
            sdlc_t_errors = 0;
            u.u_error = icc.wrerr + 100;
        }
        break;

      default:
        u.u_error = EINVAL;
        break;
    }
}

static sdlc_error(errcode)
short errcode;
{
    printf("\nSDLC");
    switch (errcode) {
    case 1:
        printf(" - error : frame to short \n");
        break;
    case 3:
        printf(" - internal error : unknown C-field to transmit\n");
        break;
    case 4:
        printf(" - error : frame to long \n");
        break;
    case 5:
        printf(" - error : not expected NR in SDLC-frame\n");
        break;
    case 7:
        printf(" - error : receiver timeout\n");
        break;
    case 6:
        printf(" - error : transmitter timeout\n");
        break;
    case 8:
        printf(" - error : RX-overrun\n");
        break;
    case 9:
        printf(" - error : TX-underrun\n");
        break;
    case 10:
        printf(" - CRC error (???)\n");
        break;
    case 11:
        printf(" - error : ABORT-sequence received\n");
        break;
    case 12:
        printf(" - error : Carrier lost\n");
        break;
    case 13:
        printf(" - Line error\n");
        break;
    case 14:
        printf(" - error : no buffer free\n");
        break;
    case 15:
        printf(" - error : attempt to transmit an empty frame\n");
        break;
    case 21:
        printf(" - error : frame lost\n");
        break;
    default:
        printf(" - unknown error %d\n", errcode);
        break;
    }
}

static Icc_command(unit, cmd, arg)
short unit, cmd;
{
    register padr;
    struct iccdev *iccbase;

    if (arg)
        padr = logtophys(arg);
    else
        padr = 0;

    iccbase = (struct iccdev*)ICC0_BASE + unit;

    if (no_icc(unit)) {
        printf(" panic: no icc # %d\n", unit);
        return;
    }

    iccbase->hi  = (short)(padr >> 16) & 0xff;
    iccbase->mi  = (short)(padr >> 8)  & 0xff;
    iccbase->lo  = (short)padr  & 0xff;
    iccbase->cmd = cmd;
}

static sdlctout()
{
    wakeup(SDLC_WAIT(W_TIMEOUT));
}

sdlc_check(unit, flag)
{
    register short notfound;
    register short i;
    
    if (flag == 0) { /* abort pending I/O */
        sdlc_notrunning[unit] = 1;
        if (sdlc_write_sleep) {
            icc.wrstat = SDLC_ERROR|SDLC_WRITE;
            icc.wrerr = 19;
            sdlc_t_errors = 19;
            sdlc_write_done = 1;
            sdlc_write_sleep = 0;
            wakeup(SDLC_WAIT(W_WRITEDONE));
        }
        if (sdlc_read_sleep) {
            icc.rdstat = SDLC_ERROR|SDLC_READ;
            icc.rderr = 19;
            sdlc_r_errors = 19;
            sdlc_read_done = 1;
            sdlc_read_sleep = 0;
            wakeup(SDLC_WAIT(W_READDONE));
        }
        return;
    }

    /* check unit */
    timeout(sdlctout, 0, hz);
    sleep(SDLC_WAIT(W_TIMEOUT), PPIPE);
    
    check_icc_sw = 0;
    if (no_icc(unit)) {
        sdlc_notrunning[unit] = 1;
        return;
    } 

    Icc_command(unit, SDLC_PROBE, &check_icc_sw);
    
    notfound = 1;
    for (i=0; i < 100; i++) {
        delay(1);
        clrcache();
        if (check_icc_sw) {
            notfound = 0;
            break;
        }
    }
    
    sdlc_notrunning[unit] = (notfound || check_icc_sw != 1);
}

no_icc(unit)
{
    return ssword((struct iccdev*)ICC0_BASE+unit, 0) == -1;
}
