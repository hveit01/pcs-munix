/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.9  Aug 25 1987 /usr/sys/io/muxke2.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/console.h"

static char dh_xon[] =  { 0x11, 0x11, 0, 0 };
static char dh_xoff[] = { 0x13, 0x13, 0, 0 };
int muxkeko = 0;

static int          dh_xon_paddr;
static int          dh_xoff_paddr;

extern int          con_type;
short               dhsar[8]; /* silo int alarm */
char                *dm_addr[];
extern struct tty   dh_tty[];
extern char         *dh_addr[];
extern int          hz;
extern int          dh_timo[];


struct dhregs {
    short dhscr;
    short dhnrcr;
    short dhlpr;
    short dhcar;
    short dhbcr;
    short dhbar;
    short dhbrcr;
    short dhssr;
};
/* DHSCR bits */
#define SC_LSEL 0x000f
#define SC_MEMX 0x0030
#define SC_RXEN 0x0040
#define SC_RINT 0x0080
#define SC_CNXM 0x0100
#define SC_NXM  0x0400
#define SC_xxxx 0x0800 /*unknown function */
#define SC_SXEN 0x1000
#define SC_TXEN 0x2000
#define SC_SINT 0x4000
#define SC_TINT 0x8000
#define SC_INTEN (SC_TXEN|SC_SXEN|SC_RXEN)

/* DHNRCR bits (read) */
#define NR_NEXT 0x00ff
#define NR_LINE 0x0f00
#define NR_PE   0x1000
#define NR_FE   0x2000
#define NR_OE   0x4000
#define NR_RCV  0x8000
/* DHNRCR bits (write) */
#define NW_WADR 0x0000
#define NW_WTIM 0x0001
#define NW_WDEL 0x0003
#define NW_RADR 0x0005
#define NW_RBDR 0x0007
#define NW_RDEL 0x0009
#define NW_INIT 0x000b
#define NW_RTIM 0x000d
#define NW_MSB  0xff00
/* DHLPR bits */
#define LP_CSZ5 0x0000
#define LP_CSZ6 0x0001
#define LP_CSZ7 0x0002
#define LP_CSZ8 0x0003
#define LP_STP2 0x0004
#define LP_xxxx 0x0008
#define LP_PEN  0x0010
#define LP_PEVN 0x0020
#define LP_RSPD 0x03c0 /*mask*/
#define LP_TSPD 0x3c00 /*mask*/
#define LP_FDX  0x4000
#define LP_ECHO 0x8000

/* this is not a DM-11-BB, but it has some modem control */
struct dmregs {
    short dmcsr;
    short dmstat;
};
/* dmcsr bits */
#define DM_LINE 0x000f /*mask*/
#define DM_IACK 0x0080 /* interrupt ack */
#define DM_INIT 0x3060 /* initialization bits */
/* dmstat bits */
#define DM_CTS  0x0008 /* clear to send */
#define DM_2000 0x2000 /* ??? */
#define DM_RTS  0x4000 /* request to send */
#define DM_DCD  0x8000 /* carrier detected */


/* some handy macros */
#define UNIT(dev) (dev>>4)
#define TOUNIT(u) (u<<4)
#define LINE(dev) (dev & 0x0f)
#define SELLINE(iop, dev) ((char*)&iop->dhscr)[1] = LINE(dev) | SC_INTEN
#define SELLINE2(iop, dev, unit) ((char*)&iop->dhscr)[1] = LINE(dev) | TOUNIT(unit) | SC_INTEN
#define SETBIT(reg, bit) { tmp = reg; tmp |= bit; reg = tmp; }
#define CLRBIT(reg, bit) { tmp = reg; tmp &= ~bit; reg = tmp; }

/* PCS: 68000 specific: NOP instruction */
#define NOPDELAY()  0x4e71

extern dhproc(), dmctl(), ttrstrt();
extern logtophys();

dhopen(dev, flag, mode)
{
    register struct tty *tp;
    extern int dh_cnt;
    static int dh_opencnt;


    if (dh_cnt > 16) {
        if (fsword(dh_addr[1]) == -1)
            dh_cnt = 16;
    }

    if (dev >= dh_cnt || fsword(dh_addr[UNIT(dev)]) == -1) {
        printf("MUXKE2: attempt to open non-existent device. dev=%d\n", dev);
        u.u_error = ENXIO;
        return;
    }
    
    if (!dh_xon_paddr) {
        dh_xon_paddr = logtophys(dh_xon);
        dh_xoff_paddr = logtophys(dh_xoff);
    }
    
    tp = &dh_tty[dev];
    if ((tp->t_state & (WOPEN|ISOPEN)) == 0) {
        ttinit(tp);
        tp->t_proc = dhproc;
        dhparam(dev);
    }

    if (dh_opencnt++ == 0)
        dhxtimeo();
    
    spltty();

    if ((tp->t_cflag & CLOCAL)|| (dmctl(dev, TTXON) & TTXOFF))
        tp->t_state |= CARR_ON;
    else
        tp->t_state &= ~CARR_ON;
    
    if ((flag & 4)==0) {
        for (; (tp->t_state & CARR_ON)==0; ) {
            tp->t_state |= WOPEN;
            sleep(&tp->t_canq, NINTER-1);
        }
    }
    
    (*linesw[tp->t_line].l_open)(tp);
    spl0();
}

dhclose(dev)
{
    register struct tty *tp = &dh_tty[dev];
    
    (*linesw[tp->t_line].l_close)(tp);

    if ((tp->t_cflag & HUPCL) && (tp->t_state & ISOPEN)==0 &&
        (tp->t_cflag & CLOCAL)==0) {
            spltty();
            dmctl(dev, 0);
            spl0();
    }
}

dhread(dev)
{
    register struct tty *tp = &dh_tty[dev];
    (*linesw[tp->t_line].l_read)(tp);
}

dhwrite(dev)
{
    register struct tty *tp = &dh_tty[dev];
    (*linesw[tp->t_line].l_write)(tp);
}

dhioctl(dev, cmd, arg, mode)
register dev;
{
    if (cmd == TCCLR && suser()) {
        dhclr();
        return;
    }
    
    if (ttiocom(&dh_tty[dev], cmd, arg, mode) != 0)
        dhparam(dev);
}

dhparam(dev)
{
    register struct tty *tp = &dh_tty[dev];
    register struct dhregs *iop = (struct dhregs*)dh_addr[UNIT(dev)];
    register int cflag = tp->t_cflag;
    register int lpr = (cflag & CBAUD) << 10;
    register int lpr1;
    ushort tmp;

/*  lpr = lpr << 10; /* LPR transmit speed */
    if (cflag & CREAD) {
        lpr1 = (cflag & CBAUD) << 6; /* LPR receive speed */
        lpr |= lpr1;
    }
    if (cflag & CS6) lpr |= LP_CSZ6; /* char length bit0 */
    if (cflag & CS7) lpr |= LP_CSZ7; /* char length bit1 */
    if (cflag & PARENB) {
        lpr |= LP_PEN; /* parity enable */
        if ((cflag & PARODD)) lpr |= LP_PEVN; /* even parity */
    }
    if (cflag & CSTOPB) lpr |= LP_STP2; /* two stop bits */
    if ((cflag & CLOCAL)==0) lpr |= LP_xxxx; /* flag clocal==0, docu says: unused */
    
    spltty();
    
    CLRBIT(iop->dhscr, SC_LSEL); /* deselect line */
    NOPDELAY();
    SETBIT(iop->dhscr, (LINE(dev) | SC_INTEN));
                    /* select line, reenable ints */

    if (dev >= 16) {
        iop->dhnrcr = 0xe100|NW_WADR; /* set BASADR = 0xe100 */
        iop->dhnrcr = 0xe500|NW_WTIM; /* set TIMCON = 0xe5 */
    }
    iop->dhnrcr = 0x0100|NW_WDEL;     /* set DELCON = 1 */
    if ((cflag & CBAUD)==0) {
        if (tp->t_state & CARR_ON) {
            iop->dhbcr = 0xffff;
            if ((tp->t_cflag & CLOCAL)==0)
                dmctl(dev, 0);
        }
        spl0();
    } else {
        if (iop->dhlpr != lpr) {
            (*tp->t_proc)(tp, T_SUSPEND);
            while (tp->t_state & BUSY)
                delay(hz>>4);
            delay(hz >> 2);
            
            SELLINE(iop, dev);
            iop->dhlpr = lpr;
            if ((lpr & 8) &&    /* flag clocal==0 */
                (dmctlon(dev) & 0x8000)==0 &&
                 (tp->t_state & CARR_ON)) {
                if (tp->t_state & ISOPEN) {
                    signal(tp->t_pgrp, SIGHUP);
                    ttyflush(tp, 3);
                }
                tp->t_state &= ~CARR_ON;
            }
            
            delay(hz >> 2);
            (*tp->t_proc)(tp, 3);
        }
        spl0();
    }
}

dhrint(dev)
{
    register struct tty *tp;
    register struct dhregs *iop;
    register int rdata; /* received data */
    register int ch;
    register int iflag;
    char out[3];
    short ocnt;
    
    sysinfo.rcvint++;
    iop = (struct dhregs*)dh_addr[dev];
    
    while ((rdata = iop->dhnrcr) < 0) { /* bit 15 set = valid char */
        tp = &dh_tty[LINE(rdata >> 8) | TOUNIT(dev)];
        if (tp >= &dh_tty[dh_cnt]) continue;
        if ((tp->t_state & ISOPEN)==0) continue;

        ch = rdata & 0x7f;  /* remove parity */
        if (tp->t_iflag & IXON) {
            if (tp->t_state & TTSTOP) {
                if (ch == 0x11 || (tp->t_iflag & IXANY))    /* DC1, XOFF */
                    (*tp->t_proc)(tp, 3);
            } else if (ch == 0x13) /* DC3, XON */
                (*tp->t_proc)(tp, 2);
            if (ch == 0x11 || ch == 0x13) continue;
        }
        
        ocnt = 1;
        iflag = tp->t_iflag;
        if ( (rdata & 0x1000) && (iflag & INPCK)==0)
            rdata &= ~0x1000;   /* ignore parity error */
        if (rdata & 0x7000) { /* any error: parity, overrun, framing */
            if ((rdata & 0xff)==0) { /* break */
                if (iflag & IGNBRK) continue;
                if (iflag & BRKINT) {
                    linesw[tp->t_line].l_input(tp, 3);
                    continue;
                }
            } else if (iflag & IGNPAR) continue;
            if (iflag & PARMRK) {
                out[2] = 0xff;
                out[1] = 0;
                ocnt = 3;
                sysinfo.rawch += 2;
            } else
                rdata = 0;
        } else if (iflag & ISTRIP)
            rdata &= 0x7f;
        else {
            rdata &= 0xff;
            if (rdata == 0xff && (iflag & PARMRK)) {
                out[1] = 0xff;
                ocnt = 2;
            }
        }
        out[0] = rdata;
        if (tp->t_rbuf.c_ptr == 0) return;
        while (ocnt) {
            ocnt--;
            *tp->t_rbuf.c_ptr = out[ocnt];
            tp->t_rbuf.c_count--;
            (*linesw[tp->t_line].l_input)(tp, 0);
        }
    }
}

dhxint(dev)
{
    register struct tty *tp;
    register short i;
    register short silo;
    struct dhregs *iop;
    short *sar;
    ushort tmp;
    
    sysinfo.xmtint++;
    
    iop = (struct dhregs*)dh_addr[dev];
    CLRBIT(iop->dhscr, 0x8000); /* clear xmit irq */
    NOPDELAY();
    SETBIT(iop->dhscr, (SC_CNXM|SC_INTEN)); /* reenable ints, and clear nx-mem int */

    sar = &dhsar[dev];
    silo = *sar & (~iop->dhbar);
    
    for (dev <<= 4, i = 1; silo; dev++, i <<= 1) {
        if (silo & i) {
            tp = &dh_tty[dev];
            dh_timo[dev] = 0;
            silo &= ~i;
            SELLINE(iop, dev);
            if (tp->t_state & BUSY) {
                tp->t_state &= ~BUSY;
                if (tp->t_tbuf.c_count) {
                    register long addr;
                    register cnt = 10;
                    do {
                        NOPDELAY();
                        NOPDELAY();
                        NOPDELAY();
                        addr = (ushort)logtophys(tp->t_tbuf.c_ptr);
                        addr = ((ushort)iop->dhcar - addr);
                        addr = addr & 0xffff;
                    } while ((addr <= 0 || addr > 64) && --cnt != 0);
                    if (cnt == 0) {
                        tp->t_tbuf.c_ptr += tp->t_tbuf.c_count;
                        tp->t_tbuf.c_count = 0;
                        muxkeko++;
                    } else {
                        tp->t_tbuf.c_count -= addr;
                        tp->t_tbuf.c_ptr += addr;
                    }
                }
            }
            
            if (tp->t_state & TTXON) {
                tp->t_state |= BUSY;
                tp->t_state &= ~TTXON;
        
                SELLINE2(iop, dev, atox(dh_xon_paddr));
                dh_timo[dev] = 6;
                iop->dhcar = (short)dh_xon_paddr;
                NOPDELAY();
                iop->dhbcr = 0xffff;
                NOPDELAY();
                SETBIT(iop->dhbar, i);
            } else if (tp->t_state & TTXOFF) {
                SELLINE2(iop, dev, atox(dh_xoff_paddr));
                tp->t_state |= BUSY;
                tp->t_state &= ~TTXOFF;
                dh_timo[dev] = 6;   
                iop->dhcar = dh_xoff_paddr;
                NOPDELAY();
                iop->dhbcr = 0xffff;
                NOPDELAY();
                SETBIT(iop->dhbar, i);
            } else {
                *sar &= ~i;
                dhproc(tp, 0);
            }
        }
    }
}



dhproc(tp, flag)
register struct tty *tp;
{
    register struct dhregs* iop;
    register struct ccblock* tbuf;
    ulong baddr;
    int dev;
    int bitmask;
    ushort tmp;

    dev = tp - dh_tty;
    iop = (struct dhregs*)dh_addr[UNIT(dev)];

    bitmask = 1 << LINE(dev);

    switch (flag) {
    case T_TIME:
        tp->t_state &= ~TIMEOUT;
        CLRBIT(iop->dhbrcr, bitmask);
        goto out;

    case T_RESUME:
    case T_WFLUSH:
        tp->t_state &= ~TTSTOP;

    out:
    case T_OUTPUT:
        if (tp->t_state & (TTSTOP|BUSY|TIMEOUT))
            break;
        tbuf = &tp->t_tbuf;
        if (tbuf->c_ptr==0 || tbuf->c_count==0) {
            if (tbuf->c_ptr)
                tbuf->c_ptr -= tbuf->c_size;

            if (((*linesw[tp->t_line].l_output)(tp) & 0x8000) == 0)
                break;
        }

        baddr = (ulong)logtophys(tbuf->c_ptr);
        { int s = spltty();
          SELLINE(iop, dev);
          dh_timo[dev] = 6;
          iop->dhcar = (short)baddr;
          NOPDELAY();
          SETBIT(iop->dhssr, 0x4000);
          NOPDELAY();
          iop->dhcar = baddr>>16;
          NOPDELAY();
          iop->dhbcr = -tbuf->c_count;
          NOPDELAY();
          SETBIT(iop->dhbar, bitmask);
          dhsar[dev>>4] |= bitmask;
          tp->t_state |= BUSY;
          splx(s);
        }
        break;

    case T_SUSPEND:
        tp->t_state |= TTSTOP;
        if (tp->t_state & BUSY) {
            SELLINE(iop, dev);
            NOPDELAY();
            iop->dhbcr = 0xffff;
        }
        break;

    case T_BLOCK:
        tp->t_state |= TBLOCK;
        tp->t_state &= ~TTXON;
        SELLINE(iop, dev);
        if (tp->t_state & BUSY) {
            tp->t_state |= TTXOFF;
            iop->dhbcr = 0xffff;
        } else {
            tp->t_state |= BUSY;
            SELLINE2(iop, dev, atox(dh_xoff_paddr));
            dh_timo[dev] = 6;
            iop->dhcar = dh_xoff_paddr;
            NOPDELAY();
            iop->dhbcr = 0xffff;
            NOPDELAY();
            SETBIT(iop->dhbar, bitmask);
            dhsar[dev>>4] |= bitmask;
        }
        break;

    case T_RFLUSH:
        if ((tp->t_state & TBLOCK) == 0)
            break;
        /*FALLTHRU*/

    case T_UNBLOCK:
        tp->t_state &= ~(TTXOFF|TBLOCK);
        SELLINE(iop, dev);
        if (tp->t_state & BUSY) {
            tp->t_state |= TTXON;
            iop->dhbcr = 0xffff;
        } else {
            tp->t_state |= BUSY;
            SELLINE2(iop, dev, atox(dh_xon_paddr));
            dh_timo[dev] = 6;
            iop->dhcar = dh_xon_paddr;
            NOPDELAY();
            iop->dhbcr = 0xffff;
            NOPDELAY();
            SETBIT(iop->dhbar, bitmask);
            dhsar[dev>>4] |= bitmask;
        }
        break;

    case T_BREAK:
        SETBIT(iop->dhbrcr, bitmask);
        tp->t_state |= TIMEOUT;
        timeout(ttrstrt, tp, hz /4);
        break;

    case T_PARM:
        dhparam(dev);
        break;
    }
}


dmctl(dev, arg)
register dev;
{
    register struct dmregs *dmp;
    register struct dhregs *iop;
    register struct tty *tp;
    ushort tmp;

    dmp = (struct dmregs*)dm_addr[UNIT(dev)];
    iop = (struct dhregs*)dh_addr[UNIT(dev)];
    
    if (dmp == 0 || fsword(dmp) == -1)
        return 0x8000;

    tp = &dh_tty[dev];

    (*tp->t_proc)(tp, T_SUSPEND);
    while (tp->t_state & BUSY)
        delay(hz >> 4);
    delay(hz >> 2);

    SELLINE(iop, dev);
    NOPDELAY();
    SETBIT(iop->dhlpr, LP_xxxx); /* set unused bit? */
    NOPDELAY();
    
    if ((dmp->dmstat & DM_CTS) == 0) {
        (*tp->t_proc)(tp, T_RESUME);
        return 0x8000;
    }

    if ((arg & 0x4000) == 0)
        CLRBIT(iop->dhlpr, LP_xxxx);

    dmp->dmstat = arg;
    NOPDELAY();
    dev = dmp->dmstat;
    NOPDELAY();
    dmp->dmcsr = DM_INIT;
    (*tp->t_proc)(tp, T_RESUME);

    return dev;
}

dmctlon(dev)
register dev;
{
    register struct dmregs *dmp;
    register struct dhregs *iop;
    ushort tmp; /*unused*/

    dmp = (struct dmregs*)dm_addr[UNIT(dev)];
    iop = (struct dhregs*)dh_addr[UNIT(dev)];

    if (dmp == 0 || fsword(dmp) == -1)
        return 0x8000;

    if ((dmp->dmstat & DM_CTS) == 0)
        return 0x8000;
    
    dmp->dmstat = DM_RTS;
    NOPDELAY();
    dev = dmp->dmstat;
    NOPDELAY();
    dmp->dmcsr = DM_INIT;
    return dev;
}

dmintr(unit)
{
    register dev;
    register struct tty *tp;
    register struct dmregs *dmp;
    register struct dhregs *iop;
    ushort tmp;

    sysinfo.mdmint++;
    dmp = (struct dmregs*)dm_addr[unit];
    iop = (struct dhregs*)dh_addr[unit];

    dev = LINE(dmp->dmcsr) | TOUNIT(unit);
    NOPDELAY();
    SELLINE(iop, dev);

    if (dev >= dh_cnt) {
        dmp->dmstat = 0;
        NOPDELAY();
        CLRBIT(dmp->dmcsr, DM_IACK);
        return;
    }

    tp = &dh_tty[dev];
    
    if (tp->t_cflag & CLOCAL) {
        dmp->dmstat = 0;
        NOPDELAY();
        CLRBIT(dmp->dmcsr, DM_IACK);
        return;
    }

    if (dmp->dmstat & DM_DCD) {
        if ((tp->t_state & CARR_ON)==0) {
            wakeup(&tp->t_canq);
            tp->t_state |= CARR_ON;
        }
    } else {
        if (tp->t_state & CARR_ON) {
            if (tp->t_state & ISOPEN) {
                signal(tp->t_pgrp, SIGHUP);
                dmp->dmstat = 0;
                ttyflush(tp, 3);
            }
            tp->t_state &= ~CARR_ON;
        }
    }

    CLRBIT(dmp->dmcsr, DM_IACK);
}

dhclr()
{
    register dev;
    register  struct tty *tp;

    ((struct dhregs*)dh_addr[0])->dhscr = SC_xxxx; /* used for probing */

    /* should not be settable - hang if it can be set! */
    while (((struct dhregs*)dh_addr[0])->dhscr & SC_xxxx);

    for (dev=0; dev < dh_cnt; dev++) {
        tp = &dh_tty[dev];
        if (tp->t_state & (WOPEN|ISOPEN)) {
            dhparam(dev);
            if ((tp->t_cflag & CLOCAL)==0)
                dmctl(dev, DM_RTS);
            
            tp->t_state &= ~(IASLP|BUSY);
            dhproc(tp, T_OUTPUT);
        }
    }
}

dhxtimeo()
{
    register struct dmregs *dmp;
    register struct dhregs *iop;
    register i;
    ushort tmp;

    for (i=0; i < dh_cnt; i++) {
        if (dh_timo[i] >= 0)
            dh_timo[i]--;
        if (dh_timo[i] == 0) {
        
            if (dh_tty[i].t_state & TTSTOP)
                dh_timo[i] = 6;
            else {
                dmp = (struct dmregs*)dm_addr[UNIT(i)];
                iop = (struct dhregs*)dh_addr[UNIT(i)];
                SELLINE(iop, i);
                NOPDELAY();
                if ((dmp->dmstat & (DM_2000|DM_CTS))==DM_CTS) {
                    dh_timo[i] = 6;
                } else {
                    printf("dh tx intr timeout on line %d\n", i);
                    CLRBIT(iop->dhbar, (1 << LINE(i)));
                    dhxint(UNIT(i));
                }
            }
        }
    }
    
    for (i=0; i < UNIT(dh_cnt); i++) {
        if (((struct dhregs*)dh_addr[i])->dhscr & SC_SINT) {
            printf("dh silo overflow\n");
            dhrint(i);
        }
    }

    timeout(dhxtimeo, dh_timo, hz);
}

#if 0

; ===========================================================================

; Segment type: Pure data
; segment "_data"
__Version:  dc.l aDhSiloOverflow+$11C6
_dh_xon:    dc.l $11110000
_dh_xoff:   dc.l $13130000
        global _muxkeko
_muxkeko:   dc.l 0
a@Release1_9Aug:dc.b '@(#) RELEASE:  1.9  Aug 25 1987 /usr/sys/io/muxke2.c ',0
aMuxke2AttemptT:dc.b 'MUXKE2: attempt to open non-existent device. dev=%d',$A,0
aDhTxIntrTimeou:dc.b 'dh tx intr timeout on line %d',$A,0
aDhSiloOverflow:dc.b 'dh silo overflow',$A,0
; end of '.data'

; ===========================================================================

; Segment type: Uninitialized
; segment "_bss"
_dh_xon_paddr:  ds.b 4
_dh_xoff_paddr: ds.b 4
        ds.b 3
byte_1307:  ds.b 1          ; DATA XREF: _dhopen:loc_80r
                    ; _dhopen+96w ...
; end of '.bss'

; ===========================================================================

; Segment type: Externs
; segment "UNDEF"
        extern _nlandev
        extern _con_type
        extern _dhsar       ; DATA XREF: _dhxint+46o _dhproc+14Ao ...
        extern _dm_addr     ; DATA XREF: _dmctl+12o _dmctlon+12o ...
        extern _dh_cnt      ; DATA XREF: _dhopen+Ar _dhopen+2Cw ...
        extern _fsword      ; CODE XREF: _dhopen:loc_1Cp
                    ; _dhopen+52p ...
        extern _dh_addr     ; DATA XREF: _dhopen+16r _dhopen+48o ...
        extern _printf      ; CODE XREF: _dhopen+6Cp _dhxtimeo+B4p ...
        extern _u       ; DATA XREF: _dhopen+74w
        extern _logtophys   ; CODE XREF: _dhopen+8Ep _dhopen+A2p ...
        extern _dh_tty      ; DATA XREF: _dhopen+BCo _dhclose+16o ...
        extern _ttinit      ; CODE XREF: _dhopen+D6p
        extern _spltty      ; CODE XREF: _dhopen:loc_106p
                    ; _dhclose+62p ...
        extern _sleep       ; CODE XREF: _dhopen+15Ap
        extern _linesw      ; DATA XREF: _dhopen+178o _dhclose+28o ...
        extern _spl0        ; CODE XREF: _dhopen+188p _dhclose+74p ...
        extern _suser       ; CODE XREF: _dhioctl+18p
                    ; DATA XREF: _dhread+2Er
        extern _ttiocom     ; CODE XREF: _dhioctl+48p
                    ; DATA XREF: _dhwrite+2Er
        extern _delay       ; CODE XREF: _dhparam+16Ep
                    ; _dhparam+18Ep ...
        extern _hz      ; DATA XREF: _dhparam:loc_46Cr
                    ; _dhparam+184r ...
        extern _signal      ; CODE XREF: _dhparam+1F2p _dmintr+FCp
                    ; DATA XREF: ...
        extern _ttyflush    ; CODE XREF: _dhparam+200p
                    ; _dmintr+10Ep
        extern _sysinfo     ; DATA XREF: _dhrint+Aw _dhrint+17Aw ...
        extern _dh_timo     ; DATA XREF: _dhxint+9Co _dhxint+190o ...
        extern _splx        ; CODE XREF: _dhproc+15Ep
        extern _timeout     ; CODE XREF: _dhproc+346p
                    ; _dhxtimeo+140p
        extern _ttrstrt     ; DATA XREF: _dhproc+340o
                    ; _dhxtimeo+3Er
        extern _wakeup      ; CODE XREF: _dmintr+C4p
; end of 'UNDEF'


        END
#endif
