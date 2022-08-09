/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.9c May 21 1987 /usr/sys/io/x25icc.c ";

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
#include "fcntl.h"

#include "sys/icc/types.h"
#include "sys/icc/unix_icc.h"

#include "sys/x25param.h"
#include "sys/x25icc.h"
#include "sys/x25user.h"
#include "sys/x25cmd.h"
#include "sys/x25kernel.h"
#include "sys/x25access.h"

#define CHAN(dev) (dev & 0x3f)

/* only one ICC supported, need to compile for more */
struct iccregs {
    ushort icc_cr0;
    ushort icc_cr1;
    ushort icc_cr2;
    ushort icc_int;
};

static struct iccregs *icc_base = (struct iccregs*)ICC0_BASE;
static struct iccregs *icc_regs;

struct device x25icc;
static struct device *icc = &x25icc;

static struct cbuf pfb;
static struct cbuf trc;
static char xmtbuf[256];
static char rcvbuf[256];
static char pf_buf[200];
static char tr_buf[1024];

char x25userversion[9] = "1.8";
short x25vector[];
ushort x25nchan;

struct cbuf *x25pbuf;
char* x25paddr;
int x25pinl;
int x25pvalid;

struct cbuf *x25tbuf;
char* x25taddr;

int x25tick_on;
int x25XMTgo;
int x25mifdone;
char x25mask[] = { 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80 };
int x25l2state;
int x25icc_nr = -1;
int x25flag;
struct pfbuf x25pfbuf;
#define RST_WAIT        0       /* wait for restart (self/other side) */
#define RST_REQ         1       /* sent restart req to other side */
#define RST_WAITACK     2       /* wait for ACK from other side */
#define RST_DONE        3       /* restart done */

#define RST_NOERR       0       /* do not set error in x25restart() */
#define RST_SETERR      1       /* set error in x25restart() */
#define RST_FORCE       1       /* force restart */

int x25restart_state;
int x25icc_intr;

#define ICCST_DOWN      0       /* waiting for download firmware */
#define ICCST_LOADED    1       /* has firmware, but circuit not yet on */
#define ICCST_UP        2       /* circuit is up */
#define ICCST_RUNNING   3       /* circuit is actively running */
int x25icc_state;

static char hdr[] = "R";

extern char *logtophys();

x25accept(cp, param)
register chanptr cp;
register struct svccall *param;
{
    struct svccall tmp;
    int cnt;

    if (cp->c_state != C_CRIN)
        u.u_error = EBADSTATE;
    
    x25getcmd(cp);
    x25chk_spcl(cp, 0);

    if (u.u_error != 0 || copyin(param, &tmp, sizeof(struct svccall)) != 0)
        return;

    cnt = x25encode(&tmp, cp->c_dbuf, 17);
    if (u.u_error == 0) {
        x25newchstate(cp, C_READY);
        cp->c_flag |= C_INCOMING;
        x25xput(cp, ACK|CALL, cnt);
    }
}

x25autoclear(cp, arg)
register chanptr cp;
{
    if (arg == 0x19 || cp == &x25chans[0])
        return 0;
    
    if ((cp->c_flag & C_OPEN)==0 || cp->c_state == C_IDLE) {
        if ((cp->c_flag & C_LISTEN) && arg == 1)
            return 0;
        
        cp->c_flag |= C_MUSTCLEAR;
        cp->c_flag &= ~C_MUSTRESET;
        if (arg == 9)
            x25newchstate(cp, C_CLIN);
        return 1;
    }
    return 0;
}

x25busy()
{
    register chanptr cp;
    
    for (cp = &x25chans[1]; cp < &(x25chans[x25nchan])+1; cp++) {
        if (cp->c_flag & C_OPEN) {
            u.u_error = EBUSY;
            return 1;
        }
    }
    return 0;
}

x25call(cp, svc)
register chanptr cp;
struct svccall *svc;
{
    struct svccall tmp;
    int cnt;

    if (cp->c_state != C_IDLE) {
        u.u_error = EBADSTATE;
        return;
    }

    x25getcmd(cp);
    x25chk_spcl(cp, 0);
    
    if (u.u_error)
        return;

    if (copyin(svc, &tmp, sizeof(struct svccall)) != 0)
        return;

    cnt = x25encode(&tmp, cp->c_dbuf, 1);
    if (u.u_error)
        return;

    if (strcmp(x25userversion, "1.8") != 0 && x25access(cp->c_dbuf, cnt) == 0)
        return;
    
    if (cp->c_channo == 0)
        x25mapout(cp);

    x25newchstate(cp, C_CROUT);
    x25xput(cp, CALL, cnt);
    
    if ((cp->c_uflag & C_CALLNWAIT) == 0)
        x25getcconf(cp, svc, &tmp);
}

x25chk_spcl(cp, param)
register chanptr cp;
{
    register flag = cp->c_flag;
    register state = cp->c_state;

    if (flag & C_LISTEN)
        return;

    if (((flag & (C_CLEARED|C_MUSTCLEAR)) || state==C_CLIN || state==C_CLOUT)
            && u.u_error == 0)
        u.u_error = ECLEAR;

    if (((flag & C_MUSTRESET) || state==C_RIN || state==C_ROUT)
            && u.u_error == 0 && param <= 8)
        u.u_error = ERESET;

    if ((flag & C_UEDATA) && u.u_error == 0 && param <= 4)
        u.u_error = EINTERRUPT;
    
    if (x25restart_state != RST_DONE && u.u_error == 0)
        u.u_error = ERESTART;
}

x25clear(cp, param)
register chanptr cp;
register char *param;
{
    char *buf = cp->c_dbuf;

    if (cp->c_state == C_IDLE) {
        u.u_error = ECLEAR;
        return;
    }

    x25getcmd(cp);
    x25chk_spcl(cp, CLEAR);
    if (u.u_error)
        return;

    cp->c_dcmd = 0;
    if (copyin(param, buf, 2) != 0)
        return;

    buf[0] = buf[1] = 0;
    x25newchstate(cp, C_CLOUT);
    x25xput(cp, CLEAR, 2);
    x25getclear(cp, 0);
}

x25close(dev)
{
    register chanptr cp = &x25chans[CHAN(dev)];
    char *buf;
    int s = currpl();

    if (x25flag & FL_PSYS)
        x25printf("%c %d %d %d %d\n", 'c', CHAN(dev), 0, 0, 0);

    buf = cp->c_dbuf;

    if (cp == &x25chans[0]) {
        cp->c_flag &= 0;
        x25acc_close();
        return;
    }

    x25getcmd(cp);
    x25chk_spcl(cp, 0);
    if (u.u_error != ERESTART) {
        
        switch (cp->c_state) {
        case C_IDLE:
            break;

        case C_CLIN:
            x25newchstate(cp, C_IDLE);
            x25xput(cp, ACK|CLEAR, 0);
            x25wait_idle(cp);
            break;

        default:
            buf[0] = buf[1] = 0;
            x25newchstate(cp, C_CLOUT);
            x25xput(cp, CLEAR, 2);
            /*FALLTHRU*/

        case C_CLOUT:
            x25wait_idle(cp);
            break;
        }
    }
    
    spltty();
    cp->c_flag &= (C_MUSTRESET|C_MUSTCLEAR);
    switch (cp->c_state) {
    case C_IDLE:
        break;

    case C_CLOUT:
        x25printf("x25close: Sent CLEAR, but didn't receive CLEARCONF.\n");
        break;
        
    default:
        x25printf("x25close: Couldn't properly clear the connection.\n");
        break;
    }

    if (cp->c_flag)
        x25printf("x25close: chan %d flag = %x.\n",
            cp - x25chans, cp->c_flag);

    x25newchstate(cp, C_IDLE);
    cp->c_uflag = 0;
    cp->c_pgrp = 0;
    splx(s);
}

x25decode(tgt, src, idx)
register struct svccall *tgt;
register char *src;
{
    char *callee = tgt->c_called_address;
    char *caller = tgt->c_calling_address;
    int nibble = 0;             /* toggle between high and low 4bits of byte */
    int crcnt = 0;
    int cecnt = 0;
    int i;
    int ch;
    int len;
    char* buf = src;

    tgt->c_facl = 0;
    tgt->c_cudl = 0;

#define GETNIBBLE() (nibble ? \
    (nibble=0, *buf++ & 0x0f) : (nibble=1, *buf >> 4) & 0x0f)

    if (idx) {
        crcnt = GETNIBBLE();
        cecnt = GETNIBBLE();
    }
    for (i = 0; i < cecnt; i++) {
        ch = GETNIBBLE() + '0';
        subyte(callee++, ch);
        if (u.u_error)
            return;
    }
    subyte(callee, 0);

    for (i=0; i < crcnt; i++) {
        ch = GETNIBBLE() + '0';
        subyte(caller++, ch);
        if (u.u_error)
            return;
    }
    subyte(caller, 0);

    if (nibble)
        buf++;
    
    if (u.u_error != 0 || buf >= &src[idx])
        return;

    len = *buf++ & 0x3f;
    tgt->c_facl = len;
    if (len && copyout(buf, tgt->c_facf, len))
        return;
    buf += len;
    
    len = idx - (buf - src);
    if (len < 0)
        len = 0;
    if (len > 16) len = 16;
    
    tgt->c_cudl = len;
    if (len && copyout(buf, tgt->c_cudf, len))
        return;
}

x25encode(src, tgt, cmd)
register struct svccall *src;
register char *tgt;
{
    char *callee = src->c_called_address;
    char *caller = src->c_calling_address;
    int nibble = 0;             /* toggle between high and low nibble */
    int cecnt = 0;              /* length of callee address */
    int crcnt = 0;              /* length of caller address */
    char* buf = &tgt[1];
    int ch;

#define PUTNIBBLE(ch) if (nibble==0)\
    { nibble=1; *buf = (ch & 0x0f) << 4; } else \
    { nibble=0; *buf++ |= (ch & 0x0f); }

    /* pack callee address into bytes */
    for (;;) {
        ch = fubyte(callee++);
        if (ch == -1) {
            u.u_error = EFAULT;
            return -1;
        }
        if (ch == 0) break;
        if (ch < '0' || ch > '9') {
            u.u_error = EINVAL;
            return -1;
        }
        PUTNIBBLE(ch);
        cecnt++;
        if (cecnt == 16) {
            u.u_error = EINVAL;
            return -1;
        }
    }

    /* pack caller address into bytes */
    for (;;) {
        ch = fubyte(caller++);
        if (ch == -1) {
            u.u_error = EFAULT;
            return -1;
        }
        if (ch == 0) break;
        if (ch < '0' || ch > '9') {
            u.u_error = EINVAL;
            return -1;
        }
        PUTNIBBLE(ch);
        crcnt++;
        if (crcnt == 16) {
            u.u_error = EINVAL;
            return -1;
        }
    }
    
    if (nibble)
        buf++;
    
    *tgt = (crcnt << 4) + cecnt;
    if (src->c_facl > 63 || src->c_cudl > 16) {
        u.u_error = EINVAL;
        return -1;
    }
    *buf++ = src->c_facl;

    if (src->c_facl != 0 && copyin(src->c_facf, buf, src->c_facl) != 0)
        return -1;
    buf += src->c_facl;

    if (src->c_cudl != 0 && copyin(src->c_cudf, buf, src->c_cudl) != 0)
        return -1;
    buf += src->c_cudl;

    if (cmd == (ACK|CALL) && 
        !cecnt && !crcnt && !src->c_facl && !src->c_cudl)
        return 0;
    
    return buf - tgt;
}

x25error(dev, err, wakeflg)
{
    register chanptr cp = &x25chans[dev];
    
    if (err == M_EL2DOWN) {
        if (x25l2state) {
            x25initchans(EL2DOWN);
            x25l2state = 0;
            x25newstate(ICCST_DOWN);
        }
        return;
    } else if (err == M_EL2FAIL) {
        x25initchans(ERESTART);
        x25chans[0].c_dcmd = x25chans[0].c_ucmd = 0;
        x25restart_state = RST_WAIT;
        x25restart(RST_NOERR, RST_FORCE);
        return;
    } else if (dev >= 1 && dev < (x25nchan+1)) {
        switch(err) {
        case 6:
            cp->c_flag |= C_MUSTCLEAR;
            cp->c_flag &= ~C_MUSTRESET;
            if (wakeflg)
                wakeup((caddr_t)cp);
            return;

        case 5:
            cp->c_flag |= C_MUSTRESET;
            if (wakeflg)
                wakeup((caddr_t)cp);
            return;
        }
    }
    x25printf("x25error: channel %d, errorcode %d from ICC.\n",
        dev, err);
}

x25getcallconf(cp, svc)
register chanptr cp;
struct svccall *svc;
{
    struct svccall tmp;
    
    if (cp->c_state != C_CROUT) {
        u.u_error = EBADSTATE;
        return;
    }
    
    if (copyin(svc, &tmp, sizeof(struct svccall)) != 0)
        return;

    x25getcconf(cp, svc, &tmp);
}

x25getcconf(cp, svcin, svcout)
register chanptr cp;
struct svccall *svcin, *svcout;
{
    int s = currpl();
    
    for (;;) {
        spltty();
        x25getcmd(cp);
        x25chk_spcl(cp, 0);
        if (u.u_error) {
            splx(s);
            return;
        }
        if (cp->c_state != C_CROUT)
            break;
        
        sleep((caddr_t)cp, 0x1b);
        splx(s);
    }
    
    splx(s);
    if (cp->c_state != C_READY) {
        x25printf("x25getcconf: bad state %d.\n", cp->c_state);
        u.u_error = EBADSTATE;
        return;
    }

    x25decode(svcout, cp->c_ubuf, cp->c_ulen);
    copyout(svcout, svcin, sizeof(struct svccall));
    cp->c_ucmd = 0;
    x25rcvbit(cp - x25chans);
}

x25getclear(cp, arg)
register chanptr cp;
register short *arg; 
{
    int s = currpl();

    if (cp->c_flag & C_CLEARED) {
        u.u_error = cp->c_clerror;
        cp->c_flag &= ~C_CLEARED;
        return;
    }
    if ((cp->c_flag & C_MUSTCLEAR) && cp->c_state == C_IDLE) {
        cp->c_flag &= ~(C_MUSTRESET|C_MUSTCLEAR);
        cp->c_dcmd = 0;
        if (cp->c_state == C_CLIN)
            x25newchstate(cp, C_IDLE);
        else
            x25newchstate(cp, C_CLOUT);

        cp->c_dbuf[0] = cp->c_dbuf[1] = 0;
        x25xput(cp, CLEAR, 2);
    }
    if (cp->c_state == C_CLIN) {
        x25newchstate(cp, C_IDLE);
        x25xput(cp, ACK|CLEAR, 0);
    } else {
        for (;;) {
            spltty();
            x25getcmd(cp);
            if (cp->c_state != C_CLOUT) {
                splx(s);
                break;
            }
            sleep((caddr_t)cp, 0x1a);
            splx(s);
        }
    }
    
    if (arg && copyout(&cp->c_lastc, arg, sizeof(short)))
        return;
}

x25getcmd(cp)
register chanptr cp;
{
    register int ucmd;
    
    if (x25icc_state != ICCST_RUNNING)
        x25initchans(EDOWNLOAD);
    ucmd = x25chans[0].c_ucmd;
    x25chans[0].c_ucmd = 0;

    if (ucmd == RESTART) {
        switch (x25restart_state) {
        case RST_WAIT:
        case RST_REQ:
        case RST_DONE:
            x25restart_state = RST_DONE;
            x25initchans(ERESTART);
            x25xput(&x25chans[0], ACK|RESTART, 0);
            break;
        case RST_WAITACK:
            x25restart_state = RST_DONE;
            break;
        }
    } else if (ucmd == (ACK|RESTART)) {
        switch (x25restart_state) {
        case RST_WAITACK:
            x25restart_state = RST_DONE;
            break;
        default:
            x25restart_state = RST_DONE;
            break;
        }
    } else if (ucmd != 0)
        x25printf("x25getcmd: Bad cmd %x on channel 0.\n", ucmd);
        
    if (ucmd != 0 || cp == &x25chans[0])
        return;

    if (cp->c_flag & C_UEACK)
        cp->c_flag &= ~(C_UEACK|C_DEDATA);

    if ((ucmd = cp->c_ucmd) == 0)
        return;

    if ((ucmd & CMDCODE)==RESET || (ucmd & CMDCODE)==CLEAR) {
        cp->c_flag &= ~(C_DBUSY|C_UEDATA|C_DEDATA);
        cp->c_flag &= ~C_MUSTRESET;
        if ((ucmd & CMDCODE)==CLEAR)
            cp->c_flag &= ~C_MUSTCLEAR;
    }
    
    if (ucmd == EDATA) {
        switch (cp->c_state) {
        case C_READY:
            cp->c_flag |= C_UEDATA;
            break;
        }
    } else if (ucmd == CALL) {
        switch (cp->c_state) {
        case C_IDLE:
            if (cp->c_flag & C_LISTEN)
                x25newchstate(cp, C_CRIN);
            else
                cp->c_flag |= C_MUSTCLEAR;
            break;
        default:
            cp->c_flag |= C_MUSTCLEAR;
            break;
        }
    } else if (ucmd == (ACK|CALL)) {
        switch (cp->c_state) {
        case C_CROUT:
            x25newchstate(cp, C_READY);
            break;
        }
    } else if (ucmd == RESET) {
        switch (cp->c_state) {
        case C_READY:
            x25newchstate(cp, C_RIN);
            break;
        case C_ROUT:
            x25newchstate(cp, C_READY);
            break;
        }
    } else if (ucmd == (ACK|RESET)) {
        switch (cp->c_state) {
        case C_ROUT:
            x25newchstate(cp, C_READY);
            break;
        }
    } else if (ucmd == CLEAR) {
        switch (cp->c_state) {
        case C_CLOUT:
            x25newchstate(cp, C_IDLE);
            break;
        default:
            x25newchstate(cp, C_CLIN);
            break;
        }
    } else if (ucmd == (ACK|CLEAR)) {
        switch (cp->c_state) {
        case C_CLOUT:
            x25newchstate(cp, C_IDLE);
            break;
        }
    } else
        x25printf("x25getcmd: Unknown cmd rcv'd: 0x%x.\n", ucmd);
    
    if (ucmd != (ACK|CALL))
        cp->c_ucmd = 0;
}

x25getinterrupt(cp, param)
register chanptr cp;
register char* param;
{
    int s = currpl();

    spltty();

    x25getcmd(cp);
    x25chk_spcl(cp, RESET);
    if (u.u_error || (cp->c_flag & C_UEDATA)==0) {
        splx(s);
        return;
    }
    
    splx(s);
    if (copyout(&cp->c_uedata, param, 1));  /* BUG? */
    cp->c_flag &= ~C_UEDATA;
    x25xput(cp, ACK|EDATA, 0);
}

x25getmesscont(cp)
register chanptr cp;
{
    if (cp->c_udata==0 || cp->c_upos==0 || cp->c_upos == cp->c_ulen) {
        if (!(cp->c_uflag & C_READMORE))
            return;
    }
    u.u_error = ENMESS;
}

x25getpf()
{
    int cout = x25pbuf->c_out;
    char ch;

    if (x25pbuf->c_in == cout)
        return;

    while (x25pbuf->c_in != cout) {
        ch = x25paddr[cout];
        if (x25pinl == 0)
            x25printf("ICC:  ");
        x25printf("%c", ch);
        x25pinl = ch != '\n';
        if (++cout == x25pbuf->c_len)
            cout = 0;
    }
    x25pbuf->c_out = cout;
}

x25getreset(cp, param)
register chanptr cp;
register char* param;
{
    int s = currpl();

    for (;;) {
        spltty();

        x25getcmd(cp);
        x25chk_spcl(cp, CLEAR);
        if (u.u_error) {
            splx(s);
            return;
        }

        if (cp->c_state == C_RIN) {
            x25newchstate(cp, C_READY);
            x25xput(cp, ACK|RESET, 0);
        }
        
        if ((cp->c_flag & C_MUSTRESET) && cp->c_state == C_READY) {
            cp->c_flag &= ~C_MUSTRESET;
            cp->c_dcmd = 0;
            x25newchstate(cp, C_ROUT);
            cp->c_dbuf[0] = cp->c_dbuf[1] = 0;
            x25xput(cp, RESET, 2);
        }

        if (cp->c_state != C_ROUT)
            break;

        sleep((caddr_t)cp, 0x1b);
        splx(s);
    }
    
    if (copyout(&cp->c_lastr, param, sizeof(struct svcreason)))
        return;
}

x25getrestart()
{
    register chanptr cp = &x25chans[0];
    int s = currpl();
    
    if (x25icc_state != ICCST_RUNNING) {
        u.u_error = EDOWNLOAD;
        return 0;
    }
    
    if (x25restart_state == RST_WAIT) {
        u.u_error = EBADSTATE;
        return 0;
    } else if (x25restart_state == RST_REQ) {
        x25restart_state = RST_DONE;
        x25xput(cp, ACK|RESTART, 0);
        return 1;
    }
    
    cp->c_timeo = 3;
        
    for (;;) {
        if (cp->c_timeo == -1)
            return 0;
            
        spltty();
        x25getcmd(cp);
        if (x25restart_state != RST_WAITACK)
            break;
            
        sleep((caddr_t)cp, 0x1b);
        splx(s);
    }
    splx(s);
    cp->c_timeo = -1;
    return 1;
}

x25gettrace()
{
    register chanptr cp = &x25chans[0];
    char ch;
    
    for (;;) {
        if (cp->c_udata == 12 && cp->c_ulen < u.u_count) {
            if (passc(RD0_ACCOUNT)); /* BUG ? */
            iomove(cp->c_ubuf, cp->c_ulen, 1);
            cp->c_udata = 0;
            x25rcvbit(cp - x25chans);
            return;
        }
        if (x25getaccess())
            return;

        if (x25pfbuf.pf_in != x25pfbuf.pf_out) {
            if (passc(RD0_PRINTF))
                return;
            while (x25pfbuf.pf_in != x25pfbuf.pf_out) {
                ch = x25pfbuf.pf_data[x25pfbuf.pf_out];
                if (++x25pfbuf.pf_out == 500)
                    x25pfbuf.pf_out = 0;
                if (passc(ch))
                    break;
            }
            return;
        }
        
        if (x25icc_state == ICCST_DOWN || x25icc_state == ICCST_LOADED) {
            u.u_error = EDOWNLOAD;
            return;
        }

        if (x25tbuf->c_in != x25tbuf->c_out) {
            if (passc(RD0_TRACE))
                return;
            while (x25tbuf->c_in != x25tbuf->c_out) {
                ch = x25taddr[x25tbuf->c_out];
                x25tbuf->c_out++;
                if (x25tbuf->c_out == x25tbuf->c_len)
                    x25tbuf->c_out = 0;
                if (passc(ch))
                    break;
            }
            return;
        }
        sleep((caddr_t)&x25tbuf, 0x1a);
    }
}

x25icc_command(iccp, addr, irq)
register struct iccregs *iccp;
caddr_t addr;
ushort irq;
{
    register int padr = 0;

    if (iccp < icc_base) {
        x25printf("x25icc_command: didn't know which ICC to use.\n");
        return;
    }

    if (addr)
        padr = (int)logtophys(addr);

    iccp->icc_cr0 = (padr >> 16) & 0xff;
    iccp->icc_cr1 = (padr >> 8)  & 0xff;
    iccp->icc_cr2 =  padr        & 0xff;
    iccp->icc_int = irq;
}

x25init(param)
register caddr_t param;
{
    struct conf *conf;
    uint maxchan;
    int s = currpl();
    chanptr cp;

    if (x25busy())
        return;

    /* needs to be in ICCST_LOADED */
    switch (x25icc_state) {
    case ICCST_DOWN:
    case ICCST_RUNNING:
        u.u_error = EDOWNLOAD;
        break;
    case ICCST_UP:
        u.u_error = EBADSTATE;
        break;
    }

    if (x25icc_nr < 0 && u.u_error == 0)
        u.u_error = EDOWNLOAD;
    
    if (u.u_error)
        return;
    
    if (x25setup() == 0)
        return;

    conf = (struct conf*)xmtbuf;
    if (copyin(param, conf, sizeof(struct conf)))
        return;

    maxchan = conf->cf_maxchan;
    if (maxchan > MAXCHAN || maxchan < 1) {
        u.u_error = EINVAL;
        return;
    }

    conf->cf_qvec = x25vector[x25icc_nr];
    x25newstate(ICCST_UP);
    x25tick((caddr_t)1);

    spltty();

    icc->xmt.csr = XINIT|XGO;
    icc->rcv.csr = 0;
    x25XMTgo = 1;
    x25setint();
    while ((icc->xmt.csr & XDONE)==0)
        sleep((caddr_t)&x25icc_state, 0x1b);

    splx(s);

    if (icc->xmt.csr & XERROR) {
        u.u_error = EIO;
        copyout((caddr_t)icc, param, 4); /* seems icc returns conf data */
        x25newstate(ICCST_DOWN);
    } else {
        x25newstate(ICCST_RUNNING);
        x25l2state = 1;
        x25nchan = maxchan;
        x25memclear(&x25chans[1], x25nchan * sizeof(struct svc));
        x25memclear(x25devnr, x25nchan+1);
        for (cp = &x25chans[0]; cp < &(x25chans[x25nchan])+1; cp++)
            cp->c_timeo = -1;
        x25rcvbits();
        x25setint();
    }
    
    icc->xmt.csr = 0;
}

x25initchans(clerr)
{
    register chanptr cp;

    x25wakeall();
    
    for (cp = &x25chans[1]; cp < &(x25chans[x25nchan])+1; cp++) {
        if (cp->c_state != C_IDLE) {
            if (cp->c_flag & C_OPEN) {
                cp->c_flag |= C_CLEARED;
                cp->c_clerror = clerr;
            }
            x25newchstate(cp, C_IDLE);
        }
    }
}

x25interrupt(cp, param)
register chanptr cp;
register caddr_t param;
{
    int s = currpl();

    for (;;) {
        spltty();

        x25getcmd(cp);
        x25chk_spcl(cp, 0);
        if (u.u_error == 0 && cp->c_state != C_READY)
            u.u_error = EBADSTATE;

        if (u.u_error) {
            splx(s);
            return;
        }
        
        if ((cp->c_flag & C_DEDATA)==0) {
            splx(s);
            break;
        }
        sleep((caddr_t)cp, 0x1b);
        splx(s);
    }
    
    if (copyin(param, &cp->c_dedata, 1))
        return;

    x25xput(cp, EDATA, 1);
    cp->c_flag |= C_DEDATA;
}

static int IntrActive;

x25intr(dev)
{
    register chanptr cp;
    int rcvchan = 0;
    int rcvflg = 0;
    int xmtcsr;
    int xmtchan;
    
    if (IntrActive)
        x25printf("x25intr: Recursive Interrupt!\n");
    IntrActive = 1;

    clrcache();
    x25tick((caddr_t)1);
    
    x25icc_intr &= ~2;

    if (x25icc_state != ICCST_DOWN) {
        xmtcsr = icc->xmt.csr;
        xmtchan = icc->xmt.chan;
    }
    
    if (icc->isdown)
        x25newstate(ICCST_DOWN);

    if (x25icc_state == ICCST_DOWN || x25icc_state == ICCST_LOADED)
        goto done;
    
    if ((xmtcsr & XDONE) || (icc->rcv.csr & XDONE))
        x25mifdone = 1;
    
    if (xmtcsr & XDONE) {
        if (x25XMTgo == 0)
            x25printf("x25intr: xmit interrupt, but no req.\n");
        x25XMTgo = 0;
    }
    
    if (x25icc_state == ICCST_UP)
        wakeup((caddr_t)&x25icc_state);
    else {
        if (xmtcsr & XWAIT) {
            if ((xmtcsr & (XWAIT|XDONE)) != (XWAIT|XDONE))
                x25printf("x25intr: bad csr %x.\n", xmtcsr);
        
            xmtchan = x25devnr[xmtchan];
            if (xmtchan >= 1 && xmtchan < (x25nchan+1)) {
                cp = &x25chans[xmtchan];
                cp->c_flag |= C_DBUSY;
            }
            xmtcsr &= ~XWAIT;
        }
    
        if (icc->rcv.csr & XDONE) {
            rcvchan = x25rint();
            rcvflg = 1;
            x25rcvbits();
        }

        if ((xmtcsr & XDONE) || rcvchan != 0) {
            if (xmtcsr & XERROR)
                x25error(icc->xmt.chan, icc->xmt.cmd, 1);

            if (x25start())
                rcvflg = 1;
            else if (xmtcsr & XDONE)
                icc->xmt.csr = 0;
        }
    
        if (rcvflg || (x25icc_intr & 1))
            x25setint();
    }
    icc->irp = 0;

done:
    IntrActive = 0;
}

x25ioctl(dev, func, param, mode)
{
    x25ioctl_(dev, func, param, mode);

    if (x25flag & FL_PSYS) {
        x25printf("%c %d %d %d %d\n",
            'i', CHAN(dev), func & 0xff, 0, u.u_error);
    }
}

x25ioctl_(dev, func, param, mode)
register dev;
{
    register chan = CHAN(dev);
    register chanptr cp = &x25chans[CHAN(dev)];

    switch (func) {
    case X25STAT:
      break;
    
    case X25DOWNLOAD:
    case X25LOADED:
    case X25INIT:
    case X25RESTART:
    case X25GETRESTART:
    case X25GETFLAG:
    case X25SETFLAG:
    case X25SETACCESS:
    case X25SETVERSION:
        if (chan != 0)
            u.u_error = EINVAL;
        break;
    
    default:
        if (chan == 0)
            u.u_error = EINVAL;
        break;
    }

    if (u.u_error)
        return;

    if (chan != 0)
        x25sigclr(cp->c_pgrp);

    switch (func) {
    case X25DOWNLOAD:
        if (x25icc_state != ICCST_DOWN)
            x25initchans(EDOWNLOAD);
        x25newstate(ICCST_DOWN);
        break;
    case X25LOADED:
        switch (x25icc_state) {
        case ICCST_DOWN:
            x25newstate(ICCST_LOADED);
            break;
        case ICCST_LOADED:
            break;
        default:
            u.u_error = EBADSTATE;
            break;
        }
        break;
    case X25INIT:
        x25init(param);
        break;
    case X25RESTART:
        x25restart(RST_SETERR, param); /*param=1 -> force restart */
        break;
    case X25GETRESTART:
        if (x25getrestart());
        break;
    case X25CALL:
        x25call(cp, param);
        break;
    case X25GETCALLCONF:
        x25getcallconf(cp, param);
        break;
    case X25LISTEN:
        x25listen(cp, param);
        break;
    case X25ACCEPT:
        x25accept(cp, param);
        break;
    case X25WRITEMESSCONT:
        cp->c_uflag |= C_WRITEMORE;
        break;
    case X25GETMESSCONT:
        x25getmesscont(cp);
        break;
    case X25WRITENMESS:
        cp->c_uflag &= ~C_WRITEQ;
        break;
    case X25READNMESS:
        cp->c_uflag &= ~C_READQ;
        break;
    case X25WRITEQMESS:
        cp->c_uflag |= C_WRITEQ;
        break;
    case X25READQMESS:
        cp->c_uflag |= C_READQ;
        break;
    case X25INTERRUPT:
        x25interrupt(cp, param);
        break;
    case X25GETINTERRUPT:
        x25getinterrupt(cp, param);
        break;
    case X25RESET:
        x25reset(cp, param);
        break;
    case X25GETRESET:
        x25getreset(cp, param);
        break;
    case X25CLEAR:
        x25clear(cp, param);
        break;
    case X25GETCLEAR:
        x25getclear(cp, param);
        break;
    case X25CALLNWAIT:
        cp->c_uflag |= C_CALLNWAIT;
        break;
    case X25CALLWAIT:
        cp->c_uflag &= ~C_CALLNWAIT;
        break;
    case X25READNWAIT:
        cp->c_uflag |= C_READNWAIT;
        break;
    case X25READWAIT:
        cp->c_uflag &= ~C_READNWAIT;
        break;
    case X25WRITENWAIT:
        cp->c_uflag |= C_WRITENWAIT;
        break;
    case X25WRITEWAIT:
        cp->c_uflag &= ~C_WRITENWAIT;
        break;
    case X25STAT:
        x25stat(param);
        break;
    case X25GETFLAG:
        copyout(&x25flag, param, sizeof(int));
        break;
    case X25SETFLAG:
        copyin(param, &x25flag, sizeof(int));
        break;
    case X25SETACCESS:
        x25setaccess(param, mode);
        break;
    case X25SETVERSION:
        copyin(param, x25userversion, 9);
        break;
    default:
        u.u_error = EINVAL;
        break;
    }
}

x25is_x25(iccno)
{
    static short version;
    register i;
    
    delay(hz / 2);

    version = 0;
    x25icc_command(&icc_base[iccno], &version, ICC_IDENT);
    
    for (i=0; i< 100; i++) {
        clrcache();
        if (version)
            break;
        
        delay(1);
    }
        
    if (i == 100)
        printf("x25is_x25: timeout on icc%d.\n", iccno);

    return version == 2;
}

x25listen(cp, param)
register chanptr cp;
register caddr_t param;
{
    struct svccall tmp;
    int s = currpl(); /*52*/
    
    if (cp->c_state != C_IDLE)
        u.u_error = EBADSTATE;
    
    if (copyin(param, &tmp, sizeof(struct svccall)))
        return;
    if (copyin(&tmp.c_cudf[0], &cp->c_protid, 1))
        return;

    cp->c_flag |= C_LISTEN;
    for (;;) {
        spltty();
        x25getcmd(cp);
        x25chk_spcl(cp, 0);
        if (x25icc_state==ICCST_DOWN && u.u_error==0)
            u.u_error = EDOWNLOAD;

        if ((cp->c_state == C_CROUT || cp->c_state == C_READY) &&
                u.u_error == 0)
            u.u_error = EBADSTATE;

        if (u.u_error) {
            cp->c_flag &= ~C_LISTEN;
            splx(s);
            return;
        }

        if (cp->c_state == C_CRIN)
            break;

        if (sleep(cp, 0x11b)) {
            cp->c_flag &= ~C_LISTEN;
            splx(s);
            if (u.u_error == 0)
                u.u_error = EINTR;
            return;
        }
        splx(s);
    }
        
    cp->c_flag &= ~C_LISTEN;
    x25decode(&tmp, cp->c_ubuf, cp->c_ulen);
    if (copyout(&tmp, param, sizeof(struct svccall)));
    x25rcvbit(cp - &x25chans[0]);
}

/* cmd=1 will accept either matching channel, or unused one */
x25mapin(chan, cmd, len)
register int chan;
{
    register chanptr cp;
    register int i;
    register int found = 0;
    char protid = 0;
    int nochan = 0;
    int empty = 0;
    int chan0 = 0;

    /* seek for the chan with matching protid -> found != 0
     * or the first empty one -> empty != 0
     */
    if (cmd == 1) {
        protid = x25protid(len);

        for (i=1; i < (x25nchan+1); i++) {
            cp = &x25chans[i];
            if (cp->c_channo) 
                continue;
            if (!(cp->c_flag & C_LISTEN))
                continue;

            if (protid == cp->c_protid) {
                found = i;
                break;
            }
            if (cp->c_protid==0 && empty==0)
                empty = i;
        }

        /* no match, but an empty channel */
        if (!found && empty)
            found = empty;

        /* we got a channel to use, report if tracing */
        if (found) {
            if (x25flag & FL_PMAP)
                x25printf("x25mapin: map call %d,%d to %d,%d.\n",
                    chan, protid & 0xff, found,
                    x25chans[found].c_protid & 0xff);
        } else if (x25flag & FL_PCLEAR) {
            x25printf("x25mapin: call on chan %d cleared (protid %d).\n",
                chan, protid & 0xff);
        }
        
        /* no match and no empty -> table full */
        if (!found)
            nochan = 1;
    }

    /* no channel found yet,
     * try to reuse a closed channel, or steal one without a
     * partner */
    if (!found) {
        /* seek reverse */
        for (i=x25nchan; i >= 1; i--) {
            cp = &x25chans[i];
            if (cp->c_channo)
                continue;
            if (!(cp->c_flag & C_OPEN)) {
                found = i;
                break;
            }
            if (chan0==0)
                chan0 = i;
        }       
        
        if (!found)
            found = chan0;

        if (found) {
            if (x25flag & FL_PMAP)
                x25printf("x25mapin: map %d to %d (cmd %d).\n",
                    chan, found, cmd);
        }
    }
    
    /* really nothing free */
    if (!found) {
        x25printf("x25mapin: BAD MAP TABLE.\n");
        return;
    }

    /* have one */
    cp = &x25chans[found];
    cp->c_channo = chan;
    cp->c_ochanno = chan;
    x25devnr[chan] = found;

    /* was a recycled one */
    if (nochan) {
        cp->c_flag |= C_MUSTCLEAR;
        cp->c_flag &= ~C_MUSTRESET;
    }
}

x25mapoff(cp)
register chanptr cp;
{
    register int chan = cp - x25chans;
    int s = currpl();

    if (x25flag & FL_PMAP)
        x25printf("x25mapoff: unmap svc%d.\n", chan);

    spltty();
    if (chan != x25devnr[cp->c_channo])
        x25printf("x25mapoff: BAD MAP TABLE.\n");

    x25devnr[cp->c_channo] = 0;
    cp->c_channo = 0;
    splx(s);
}

x25mapout(cp)
register chanptr cp;
{
    register int chan = cp - x25chans;
    register i;
    int s = currpl();

    spltty();

    for (i = x25nchan; i >= 1; i--) {
        if (x25devnr[i]==0) {
            cp->c_channo = i;
            cp->c_ochanno = i;
            x25devnr[i] = chan;
            splx(s);
            if (x25flag & FL_PMAP)
                x25printf("x25mapout: map svc%d to chan %d.\n",
                    chan, i);
            return;
        }
    }

    x25printf("x25mapout: BAD MAP TABLE.\n");
    splx(s);
}

x25memclear(s, size)
register caddr_t s;
register ushort size;
{
    if (size==0) {
        x25printf("x25memclear: size = %d.\n", size);
        return;
    }

    while (size-- != 0)
        *s++ = 0;
}

x25newchstate(cp, newst)
register chanptr cp;
{
    if (cp->c_state == newst)
        return;

    cp->c_state = newst;
    if (newst != C_IDLE) {
        cp->c_udata = cp->c_ddata = 0;
        cp->c_upos = 0;
        cp->c_flag &= ~(C_DBUSY|C_UEDATA|C_DEDATA);
    }
    if (newst == C_IDLE && cp->c_channo)
        x25mapoff(cp);

    if (newst == C_RIN || newst == C_ROUT)
        cp->c_flag &= ~C_MUSTRESET;

    if (newst == C_CLIN || newst == C_CLOUT)
        cp->c_flag &= ~(C_INCOMING|C_MUSTCLEAR|C_MUSTRESET);
}

x25newstate(newst)
{
    x25icc_state = newst;
    if (newst == ICCST_DOWN) {
        x25pvalid = 0;
        x25icc_intr = 0;
        x25XMTgo = 0;
        x25l2state = 0;
        x25restart_state = RST_WAIT;
        x25wakeall();
    }

    switch (newst) {
    case ICCST_DOWN:
    case ICCST_LOADED:
        x25flag = 0;
        x25pfbuf.pf_in = 0;
        x25pfbuf.pf_out = 0;
    }
}

x25open(dev, mode)
{
    x25open_(dev, mode);

    if (x25flag & FL_PSYS)
        x25printf("%c %d %d %d %d\n", 
            'o', CHAN(dev), 0, 0, u.u_error);
}

x25open_(dev, mode)
int dev;
{
    register chanptr cp;
    register chan = CHAN(dev);
    
    cp = &x25chans[CHAN(dev)];
    if (cp == x25chans) {
        cp->c_flag | = C_OPEN;
        return;
    }
    
    if (x25icc_state != ICCST_RUNNING || x25icc_nr < 0) {
        u.u_error = EDOWNLOAD;
        return;
    }
    
    if (ssword(icc_regs, 0) == -1) {
        u.u_error = EDOWNLOAD;
        return;
    }

    if (chan > x25nchan) {
        u.u_error = ENXIO;
        return;
    }
    
    x25getcmd(cp);
    if (x25restart_state != RST_DONE) {
        u.u_error = ERESTART;
        return;
    }

    if (cp->c_flag) {
        if ((mode & O_APPEND)==0)
            u.u_error = EBUSY;
        return;
    }

    cp->c_flag &= 0;
    cp->c_flag |= C_OPEN;
    cp->c_uflag = 0;
    cp->c_pgrp = u.u_procp->p_pgrp;
    cp->c_uid = u.u_uid;
}

typedef struct { int arg[8]; } va;
x25printf(fmt, args)
char* fmt;
va args;
{
    char buf[100]; /* 0x80 */
    int i;
    int sz; /*88*/
    int pfin; /*8c*/
    int pfout; /*90*/
    int pfsz; /*94*/
    int n;

    if (((sz = sprintf(buf, fmt, args))+1) > 100)
        panic("x25printf: buffer overflow.");
    if (!(x25flag & FL_PUSER)) {
        printf("%s", buf);
        return;
    }

    pfin = x25pfbuf.pf_in;
    pfout = x25pfbuf.pf_out;
    pfsz = 500;
    n = pfsz - 1 - pfin + pfout;
    if (n >= pfsz)
        n -= pfsz;
    if (sz >= n)
        sz = n - 1;
    
    for (i=0; i < sz; i++) {
        x25pfbuf.pf_data[pfin] = buf[i];
        pfin++;
        if (pfin == pfsz)
            pfin = 0;
    }
    x25pfbuf.pf_in = pfin;
}

static sprintf(buf, fmt, args)
char *buf;
register char *fmt;
int args;
{
    register int ch;
    register char* s;

    int *ap;
    char *bp = buf;
    
    ap = (int*)&args;

    for (;;) {
        while ((ch= *fmt++) != '%') {
            if (ch == 0) {
                *buf++ = '\0';
                return (buf - bp) - 1;
            }
            *buf++ = ch;
        }
        ch = *fmt++;
        switch (ch) {
        case 'D':
        case 'd':
        case 'u':
      lbl_2990:
            printn(&buf, *ap++);
            break;
        case 'O':
        case 'o':
      lbl_29a8:
            printx(&buf, *ap++, 3, 0x1fffffff, 7);
            break;
        case 'X':
        case 'x':
      lbl_29d0:
            printx(&buf, *ap++, 4, 0xfffffff, 15);
            break;
        case 'c':
            *buf++ = *ap++;
            break;
        case 's':
            s = (char*)*ap++;
            while ((ch = *s++) != '\0')
                *buf++ = ch;
            break;
        default:
            *buf++ = ch;
        }
    }
}

static printn(buf, n)
char** buf;
{
    register int i;

    if (n < 0) {
        *((*buf)++) = '-';
        n = - n;
    }
    
    if ((i = n/10) != 0)
        printn(buf, i);
    
    *((*buf)++) = "0123456789"[n % 10];
}

static printx(buf, n, bits, mask, cmask)
char **buf;
{
    register rem = bits;
    rem  = n >> rem;
    
    if ((rem &= mask) != 0)
        printx(buf, rem, bits, mask, cmask);
    
    *((*buf)++) = "0123456789ABCDEF"[n & cmask];
}

x25protid(len)
register int len;
{
    register idx;
    register int dlen;
    
    if (len==0)
        return 0;

    idx = rcvbuf[0];    /* packed size of addresses */

    idx = (idx & 0xf) + ((idx >> 4) & 0xf); /*unpack*/
    idx = (idx+1) >> 1; /* align */
    if (len <= (idx+1))
        return 0;

    dlen = rcvbuf[idx+1];
    dlen &= 0x3f;
    if (len <= (idx + dlen + 2))
        return 0;

    return rcvbuf[idx + dlen + 2];
}

x25rcvbit(chan)
register int chan;
{
    register byt, bit;
    char ena;
    
    chan = x25chans[chan].c_channo;
    byt = chan / 8;
    bit  = chan & 7;

    if (chan < 0 || chan >= (x25nchan+1)) {
        x25printf("x25rcvbit: bad chan %d.\n", chan);
        return;
    }

    ena = icc->rcvena[byt];
    if (ena & x25mask[bit])
        return;
    
    ena |= x25mask[bit];
    icc->rcvena[byt] = ena;
    x25setint();
}

x25rcvbits()
{
    register chanptr cp;
    register int i;
    register int byt, bit;
    char rcvena[16];

    bcopy(icc->rcvena, rcvena, 16);
    
    for (i=0; i < (x25nchan+1); i++) {
        cp = &x25chans[x25devnr[i]];
        byt = i / 8;
        bit = i & 7;
        if (cp->c_udata || (cp->c_ucmd & CMDCODE)==CALL)
            rcvena[byt] &= ~x25mask[bit];
        else
            rcvena[byt] |= x25mask[bit];
    }
    
    bcopy(rcvena, icc->rcvena, 16);
    icc->rcv.csr = XGO;
}

x25read(dev)
{
    ulong cntsav = u.u_count;
    int   chan = CHAN(dev);
    
    x25read_(dev);

    if ((x25flag & FL_PSYS) && chan)
        x25printf("%c %d %d %d %d\n",
            'r', chan, cntsav, cntsav-u.u_count, u.u_error);
}

x25read_(dev)
{
    register chanptr cp = &x25chans[CHAN(dev)];
    int sz;
    int ucmd;
    int s = currpl(); /*40*/

    if (cp == x25chans) {
        x25gettrace();
        return;
    }

    cp->c_pgrp = u.u_procp->p_pgrp;
    cp->c_uid = u.u_uid;
    x25sigclr(cp->c_pgrp);

    for (;;) {
        spltty();
        x25getcmd(cp);
        x25chk_spcl(cp, 0);
        if (u.u_error==0 && cp->c_state != C_READY)
            u.u_error = EBADSTATE;

        if (u.u_error) {
            splx(s);
            return;
        }
        
        if (cp->c_udata != 0)
            break;

        if (cp->c_uflag & C_READNWAIT) {
            splx(s);
            return;
        }
        
        sleep((caddr_t)cp, 0x1b);
    }

    ucmd = cp->c_udata & CMDCODE;
    if (cp->c_uflag & C_READQ) {
        if (ucmd != QDATA)
            u.u_error = ENMESS;
    } else if (ucmd != NDATA)
        u.u_error = EQMESS;

    if (u.u_error)
        return;

    sz = cp->c_ulen - cp->c_upos;
    if (sz > u.u_count)
        sz = u.u_count;

    iomove(&cp->c_ubuf[cp->c_upos], sz, 1);
    cp->c_upos += sz;
    if (cp->c_upos < cp->c_ulen)
        return;
    if (cp->c_udata & MOREDATA)
        cp->c_uflag |= C_READMORE;
    else
        cp->c_uflag &= ~C_READMORE;
    
    cp->c_udata = 0;
    x25rcvbit(cp - x25chans);
}

x25reset(cp, param)
register chanptr cp;
register caddr_t param;
{
    char *dbuf = cp->c_dbuf;

    x25getcmd(cp);
    x25chk_spcl(cp, RESET);

    if (u.u_error==0 && cp->c_state != C_READY)
        u.u_error = EBADSTATE;
    
    if (u.u_error)
        return;

    cp->c_dcmd = 0;
    if (copyin(param, dbuf, 2))
        return;

    dbuf[0] = dbuf[1] = 0;
    x25newchstate(cp, C_ROUT);
    x25xput(cp, RESET, 2);
}

x25reseticc(iccno, flag)
{
    if (x25icc_nr == iccno)
        x25newstate(ICCST_DOWN);

    if (flag && x25is_x25(iccno)) {
        x25icc_nr = iccno;
        icc_regs = &icc_base[x25icc_nr];
        if (x25icc_state != ICCST_DOWN)
            x25newstate(ICCST_DOWN);
    }
}

/* seterr=1 with set u.u_error
 * force=1 with enforce reset, if 0, will only restart, if not yet
 * initialized */
x25restart(seterr, force)
{
    register chanptr cp = x25chans;
    int i;

    if (x25icc_state != ICCST_RUNNING) {
        if (seterr)
            u.u_error = EDOWNLOAD;
        return;
    }

    x25getcmd(cp);
    switch (x25restart_state) {
    case RST_REQ:
    case RST_WAITACK:
        if (seterr)
            u.u_error = ERESTART;
        return;
    }

    if (u.u_error && !seterr)
        return;

    if (force || x25restart_state==RST_WAIT) {
        x25initchans(ERESTART);
        cp->c_dbuf[0] = cp->c_dbuf[1] = 0;
        x25restart_state = RST_WAITACK;
        x25xput(cp, RESTART, 2);
    }

    for (i = 2; ;) {
        if (!seterr)
            return;
        if (x25getrestart())
            return;
        if (i-- <= 0)
            break;
        x25restart_state = RST_WAITACK;
        x25xput(cp, RESTART, 2);
    }
    
    x25restart_state = RST_WAIT;
    if (seterr)
        u.u_error = EL2DOWN;
}

x25rint()
{
    register chanptr cp;
    register int chan = icc->rcv.chan;
    register int bc = icc->rcv.bc;
    int clflag = 0;
    int rcvcmd = icc->rcv.cmd;
    char* ubuf;
    
    if (chan < 0 || chan > x25nchan) {
        x25printf("x25rint: Bad channel number %d.\n", chan);
        return 0;
    }

    if (icc->rcv.csr & XERROR)
        rcvcmd = 0;

    if (chan && x25devnr[chan]==0)
        x25mapin(chan, rcvcmd, bc);
        
    cp = &x25chans[x25devnr[chan]];
    x25signal(cp);

    if (cp->c_flag & C_MUSTCLEAR) {
        clflag = 1;
        goto done;
    }

    if (icc->rcv.csr & XERROR) {
        switch (icc->rcv.cmd) {
        case RNRP:
            cp->c_flag |= C_MUSTCLEAR;
            cp->c_flag &= ~C_MUSTRESET;
            clflag = 1;
            goto done;
        case RRP:
            cp->c_flag |= C_MUSTRESET;
            clflag = 1;
            goto done;
        default:
            x25error(chan, icc->rcv.cmd, 0);
            return 0;
        }
    }
    
    if (x25restart_state != RST_DONE && 
            (rcvcmd & CMDCODE) != RESTART && rcvcmd != ACCOUNT)
        return 0;

    if (x25autoclear(cp, rcvcmd))
        return 1;

    if (rcvcmd == (ACK|CLEAR) && chan != 0 && !(cp->c_flag & C_OPEN)) {
        x25newchstate(cp, C_IDLE);
        return 0;
    }

    if (bc > 128) bc = 128;
    if (bc < 0) bc = 0;

    switch (rcvcmd & CMD) {
    case CALL:
    case NDATA:
    case QDATA:
    case ACCOUNT:
    case ACK|CALL:
        cp->c_ulen = bc;
        cp->c_upos = 0;
        ubuf = cp->c_ubuf;
        if ((rcvcmd & CMDCODE) == CALL)
            cp->c_ucmd = rcvcmd;
        else
            cp->c_udata = rcvcmd;
        break;
    case ACK|NDATA:
        if (cp->c_state == C_READY && !(cp->c_flag & C_DBUSY))
            x25printf("x25rint: NDATA+ACK ignored.\n");
        cp->c_flag &= ~C_DBUSY;
        clflag = 1;
        goto done;
    case EDATA:
        bc = 1;
        ubuf = &cp->c_uedata;
        cp->c_ucmd = rcvcmd;
        break;
    case ACK|EDATA:
        cp->c_flag |= C_UEACK;
        goto done;
    case RESET:
    case CLEAR:
    case RESTART:
        bc = 2;
        cp->c_ucmd = rcvcmd;
        if ((rcvcmd & CMD) == RESET)
            ubuf = (char*)&cp->c_lastr;
        else
            ubuf = (char*)&cp->c_lastc;
        break;
    case ACK|RESET:
    case ACK|CLEAR:
    case ACK|RESTART:
        bc = 0;
        cp->c_ucmd = rcvcmd;
        break;
    }
    
    bcopy(rcvbuf, ubuf, bc);
    
done:
    if ((rcvcmd & CMD) != RESTART) {
        if (rcvcmd == ACCOUNT)
            wakeup((caddr_t)&x25tbuf);
        else
            wakeup((caddr_t)cp);
    } else
        x25wakeall();
    
    return clflag;
}

x25setint()
{
    if (x25icc_intr & 2)
        x25icc_intr |= 1;
    else {
        icc_regs->icc_int = 1;
        x25icc_intr = 2;
    }
}

x25setup()
{
    x25memclear(icc, 0x40);
    
    icc->xmt.bar = logtophys(xmtbuf);
    icc->rcv.bar = logtophys(rcvbuf);

    pfb.c_len = 200;
    pfb.c_addr = logtophys(pf_buf);
    pfb.c_in = pfb.c_out = 0;
    icc->pfb = (struct cbuf*)logtophys(&pfb);

    trc.c_len = 1024;
    trc.c_addr = logtophys(tr_buf);
    trc.c_in = trc.c_out = 0;
    icc->trc = (struct cbuf*)logtophys(&trc);

    x25pbuf = &pfb;
    x25paddr = pf_buf;

    x25pinl = 0;
    x25pvalid = 1;

    x25tbuf = &trc;
    x25taddr = tr_buf;

    x25restart_state = RST_WAIT;
    x25icc_intr = 0;

    x25icc_command(icc_regs, icc, ICC_CONNECT);

    return 1;
}

x25sigclr(pgrp)
{
    register chanptr cp;
    
    for (cp = &x25chans[1]; cp < (&x25chans[x25nchan])+1; cp++) {
        if (pgrp == cp->c_pgrp)
            cp->c_flag &= ~C_SIGNALLED;
    }
}

x25signal(cp)
register chanptr cp;
{
    register chanptr cp2;
    
    if (cp == x25chans || cp->c_pgrp == 0)
        return;

    if ((cp->c_uflag & (C_WRITENWAIT|C_CALLNWAIT|C_READNWAIT)) == 0)
        return;
    
    if (cp->c_flag & C_SIGNALLED)
        return;

    for (cp2 = &x25chans[1]; cp2 < (&x25chans[x25nchan])+1; cp2++) {
        if (cp->c_pgrp == cp2->c_pgrp && (cp2->c_flag & C_SIGNALLED))
            return;
    }
    cp->c_flag |= C_SIGNALLED;
    signal(cp->c_pgrp, SIGUSR2);
}

x25start()
{   
    register chanptr cp;
    register char* bp;
    register int i;
    int s = currpl();

    spltty();
    if (x25XMTgo) {
        splx(s);
        return 0;
    }
    
    for (i=0; i < (x25nchan+1); i++) {
        cp = &x25chans[i];
        if (cp->c_dcmd) {
            bp = cp->c_dbuf;
            if (cp->c_dcmd == EDATA)
                bp = &cp->c_dedata;
            x25xmit(i, cp->c_dcmd, bp, cp->c_dlen);
            cp->c_dcmd = 0;
            break;
        }
        if (cp->c_flag & C_MUSTCLEAR) {
            if (!(cp->c_flag & C_OPEN) || 
                    cp->c_state == C_IDLE || cp->c_state == C_CLOUT) {
                cp->c_flag &= ~(C_MUSTCLEAR|C_MUSTRESET);
                switch (cp->c_state) {
                case C_CLIN:
                    x25newchstate(cp, C_IDLE);
                    break;
                default:
                    x25newchstate(cp, C_CLOUT);
                    break;
                }
                x25xmit(i, CLEAR, "\0\0", 2);
                break;
            }
        }

        if (cp->c_flag & C_MUSTRESET) {
            if (!(cp->c_flag & C_OPEN) || cp->c_state != C_READY) {
                cp->c_flag &= ~C_MUSTRESET;
                switch (cp->c_state) {
                case C_CLIN:
                case C_CLOUT:
                    break;
                case C_RIN:
                    x25newchstate(cp, C_READY);
                    x25xmit(i, RESET, "\0\0", 2);
                    break;
                case C_READY:
                    x25newchstate(cp, C_ROUT);
                    x25xmit(i, RESET, "\0\0", 2);
                    break;
                default:
                    break;
                }
                break;
            }
        }
        if (cp->c_flag & C_DEACK) {
            x25xmit(i, (ACK|EDATA), 0, 0);
            cp->c_flag &= ~C_DEACK;
            break;
        }
        
        if (cp->c_ddata && !(cp->c_flag & C_DBUSY)) {
            x25xmit(i, cp->c_ddata, cp->c_dbuf, cp->c_dlen);
            cp->c_ddata = 0;
            x25signal(cp);
            break;
        }       
    }
    
    splx(s);
    if (i == (x25nchan+1))
        return 0;
    
    wakeup((caddr_t)cp);
    return 1;
}

x25stat(param)
register char* param;
{
    static struct svcstat svcstat;
    register chanptr cp;
    register struct s_chan *schan;
    
    svcstat.s_hwinfo = 0;
    svcstat.s_l2state = 0;
    svcstat.s_l3state = 0;
    svcstat.s_nchan = x25nchan;

    if (x25icc_nr >= 0 && x25icc_state != ICCST_DOWN) {
        svcstat.s_hwinfo |= (x25icc_nr | S_IS_UP);
        svcstat.s_hwaddr = (char*)&icc_base[x25icc_nr];
    }       
        
    if (x25l2state && x25icc_state == ICCST_RUNNING)
        svcstat.s_l2state = 1;
        
    if (x25restart_state == RST_DONE)
        svcstat.s_l3state = 1;

    for (schan = svcstat.s_chan, cp = x25chans; 
            cp < (&x25chans[x25nchan])+1; schan++, cp++) {
        if (cp->c_flag & C_OPEN) {
            if (cp->c_flag & C_LISTEN) {
                schan->s_state = S_LISTEN;
                schan->s_protid = cp->c_protid;
                schan->s_pgrp = cp->c_pgrp;
            } else {
                switch (cp->c_state) {
                case C_READY:
                case C_RIN:
                case C_ROUT:
                    schan->s_state = S_CONNECTED;
                    schan->s_channo = cp->c_channo;
                    break;
                default:
                    schan->s_state = S_OPEN;
                    break;
                }
            }
        } else
            schan->s_state = S_NOTOPEN;
    }

    copyout(&svcstat, param, sizeof(struct svcstat));
}

x25tick(p)
register caddr_t p;
{
    register chanptr cp;
    int xmtcsr;
    
    if (p) {
        if (!x25tick_on)
            timeout((caddr_t)x25tick, 0, hz);
        x25tick_on = 1;
        return;
    }

    for (cp = x25chans; cp < (&x25chans[x25nchan])+1; cp++) {
        if (cp->c_timeo == 0)
            wakeup((caddr_t)cp);
        if (cp->c_timeo >= 0)
            cp->c_timeo--;
    }
    
    clrcache();

    if (!((xmtcsr = icc->xmt.csr) & XINIT) && 
        x25XMTgo && ++x25XMTgo >= 3 && x25mifdone) {
            x25printf("x25tick: xmit timeout.\n");
            x25XMTgo = 0;
            if ((xmtcsr & XDONE) || (icc->rcv.csr & XDONE)) {
                x25XMTgo = 1;
                x25intr(0);
            } else if (x25start())
                x25setint();
            x25mifdone = 0;
    }
    
    if (x25pvalid) {
        x25getpf();
        wakeup((caddr_t)&x25tbuf);
    }
    
    timeout((caddr_t)x25tick, 0, hz);
}

x25wait_idle(cp)
register chanptr cp;
{
    int s = currpl();

    cp->c_timeo = 4;

    for (;;) {
        spltty();
        x25getcmd(cp);
        if (cp->c_timeo < 0 || x25icc_state != ICCST_RUNNING ||
                (cp->c_state == C_IDLE && cp->c_dcmd==0)) {
            splx(s);
            break;
        }
        
        if (sleep((caddr_t)cp, 0x11a)) {
            if (u.u_error == 0)
                u.u_error = EINTR;
            splx(s);
            break;
        }
        
        splx(s);
    }
    
    cp->c_timeo = -1;
}

x25write(dev)
{
    int ucount = u.u_count;
    x25write_(dev);

    if (x25flag & FL_PSYS)
        x25printf("%c %d %d %d %d\n",
            'w', CHAN(dev), ucount, ucount-u.u_count, u.u_error);
}

x25write_(dev)
{
    register chanptr cp = &x25chans[CHAN(dev)];
    int sz;
    int cmd;
    int s = currpl();

    if (cp == x25chans) {
        u.u_error = EINVAL;
        return;
    }

    cp->c_pgrp = u.u_procp->p_pgrp;
    cp->c_uid = u.u_uid;
    
    x25sigclr(cp->c_pgrp);
    
    while (u.u_count != 0) {
        
        spltty();
        x25getcmd(cp);
        x25chk_spcl(cp, 0);
        if (cp->c_state != C_READY && u.u_error==0)
            u.u_error = EBADSTATE;

        if (u.u_error) {
            splx(s);
            return;
        }

        if ((cp->c_flag & C_DBUSY) || cp->c_ddata) {
            if (cp->c_uflag & C_WRITENWAIT) {
                splx(s);
                return;
            }
            sleep((caddr_t)cp, 0x1b);
            splx(s);
        } else {
            splx(s);
            cmd = NDATA;
            if (cp->c_uflag & C_WRITEQ)
                cmd = QDATA;
            if (u.u_count > 128)
                cmd |= MOREDATA;
            else if (u.u_count == 128) {
                if (cp->c_uflag & C_WRITEMORE) {
                    cmd |= MOREDATA;
                    cp->c_uflag &= ~ C_WRITEMORE;
                }
            } else
                cp->c_uflag &= ~C_WRITEMORE;
            sz = 128;
            if (sz > u.u_count)
                sz = u.u_count;
            if (iomove(cp->c_dbuf, sz, 0));
            if (u.u_error)
                return;
            cp->c_dlen = sz;
            cp->c_ddata = cmd;
            if (x25start())
                x25setint();
        }
    }
}

x25wakeall()
{
    register chanptr cp;
    
    wakeup((caddr_t)x25chans);
    
    for (cp = &x25chans[1]; cp < (&x25chans[x25nchan])+1; cp++) {
        if (cp->c_state != C_IDLE || (cp->c_flag & C_LISTEN))
            wakeup((caddr_t)cp);
    }
    
    wakeup((caddr_t)&x25tbuf);
}

x25xmit(chan, cmd, buf, len)
register char* buf;
{
    register int sz;
    register int ochanno;

    if (x25XMTgo)
        x25printf("x25xmit: xmit was busy.\n");
    
    switch (cmd) {
    case EDATA:
        sz = 1;
        break;
    case ACK|EDATA:
    case ACK|RESET:
    case ACK|CLEAR:
    case ACK|RESTART:
        sz = 0;
        break;
    case RESET:
    case CLEAR:
    case RESTART:
        sz = 2;
        break;
    default:
        sz = len;
        break;
    }

    ochanno = x25chans[chan].c_ochanno;
    if ((cmd & CMDCODE) != RESTART && ochanno==0)
        x25printf("x25xmit: cmd=%d, dev=%d, channo=0.\n",
            cmd, chan, ochanno);

    icc->xmt.chan = ochanno;
    bcopy(buf, xmtbuf, sz);
    icc->xmt.bc = sz;
    icc->xmt.cmd = cmd;
    icc->h1 = x25chans[chan].c_uid;
    icc->xmt.csr = XGO;
    x25XMTgo = 1;
}

x25xput(cp, cmd, len)
register chanptr cp;
{
    switch (cmd) {
    case NDATA:
    case QDATA:
        cp->c_dlen = len;
        cp->c_ddata = cmd;
        break;
    case CALL:
    case ACK|CALL:
        cp->c_dlen = len;
        cp->c_dcmd = cmd;
        break;
    case ACK|EDATA:
        cp->c_flag |= C_DEACK;
        break;
    default:
        cp->c_dcmd = cmd;
        break;
    }

    if (x25start())
        x25setint();
}

char x25acc_busy;
char x25acc_filled;
short x25acc_refnr;
struct x25access x25acc;

x25access(callp, len)
char* callp;
ushort len;
{
    while (x25acc_busy)
        sleep((caddr_t)&x25acc_busy, 0x1b);

    if ((x25chans[0].c_flag & C_OPEN)==0) {
        u.u_error = EACCES;
        return 0;
    }

    x25acc.acc_uid = u.u_uid;
    x25acc.acc_len = len;
    x25acc.acc_refnr = ++x25acc_refnr;
    strncpy(x25acc.acc_callp, callp, PACKETSIZE);
    x25acc.acc_allow = -1;
    x25acc_busy = x25acc_filled = 1;
    wakeup((caddr_t)&x25tbuf);

    while (x25acc.acc_allow < 0) {
        if (sleep((caddr_t)&x25acc, 0x11b)) {
            u.u_error = EINTR;
            break;
        }
        if (x25acc.acc_allow==0)
            u.u_error = EACCES;
    }
    x25acc_busy = 0;
    wakeup((caddr_t)&x25acc_busy);
    x25acc_filled = 0;
    return u.u_error==0;
}

x25getaccess()
{
    if (x25acc_filled == 0)
        return 0;
    
    if (u.u_count < sizeof(struct x25access)+sizeof(char[2]))
        u.u_error = EINVAL;
    else {
        iomove(hdr, 2, 1);
        iomove(&x25acc, sizeof(struct x25access), 1);
        x25acc_filled = 0;
    }
    return 1;
}

x25setaccess(buf, mode)
struct x25access *buf;
{
    struct x25access tmp;
    
    if (!(mode & 2)) {
        u.u_error = EACCES;
        return;
    }

    if (copyin(buf, &tmp, sizeof(struct x25access)))
        return;

    if (tmp.acc_refnr == x25acc.acc_refnr) {
        x25acc.acc_allow = tmp.acc_allow;
        wakeup((caddr_t)&x25acc);
    }
}

x25acc_close()
{
    x25acc.acc_allow = 0;
    wakeup((caddr_t)&x25acc);
}

static strncpy(s1, s2, n)
register char *s1, *s2;
register n;
{
    while (--n >= 0)
        *s1++ = *s2++;
}
