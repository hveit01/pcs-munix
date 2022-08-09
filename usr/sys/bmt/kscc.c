/* PCS specific */

/* Event handling for CBIP devices */

#include <sys/types.h>
/*#include <sys/systm.h>*/
#include <sys/param.h>

#include <sys/bmt/gdi.h>
#include <sys/bmt/gdisys.h>
#include <sys/bmt/rtk.h>
/*#include <sys/bmt/cbip.h>*/
#include <sys/bmt/device.h>
#include <sys/bmt/list.h>
#include <sys/bmt/EventMgr.h>
#include <sys/bmt/Layer.h>
#include <sys/bmt/font.h>
#include <sys/bmt/Window.h>

static char *_Version = "@(#) RELEASE: 6.2 87/04/15 /usr/sys/bmt/kscc.c";

/* The SCC 85C30 uses 2 registers:
 * - the command register
 * - the data register
 *
 * In order to access the internal registers one has to first
 * address the register by writing the address to the cmd, and then
 * read or write the value through the cmd register. After a read/write
 * the next write will again allow addressing a register.
 * The data register allows direct access to the WR8 and RR8 registers
 * without resetting the address selected.
 *
 * See data book 1991_Zilog_Datacom_ICs.pdf from bitsavers */
struct z85c30 {
    int cmd;
    int data;
};
/* incomplete set of definitions */
#define WR0             0
#define  WR0_RSTESINT   0x10
#define  WR0_RXINTEN    0x20
#define  WR0_RSTTXINT   0x28
#define  WR0_RSTERR     0x30
#define  WR0_RSTHIUS    0x38
#define WR1             1
#define  WR1_NOINT      0x00
#define  WR1_RXINT1st   0x08
#define  WR1_RXINTALL   0x10
#define  WR1_RXINTSPEC  0x18
#define WR2             2
#define WR3             3
#define  WR3_RX5BIT     0x00
#define  WR3_RX7BIT     0x40
#define  WR3_RX6BIT     0x80
#define  WR3_RX8BIT     0xc0
#define  WR3_RXDIS      0x00
#define  WR3_RXEN       0x01
#define WR4             4
#define  WR4_X1         0x00
#define  WR4_X16        0x40
#define  WR4_X32        0x80
#define  WR4_X64        0xc0
#define  WR4_1STOP      0x04
#define  WR4_15STOP     0x08
#define  WR4_2STOP      0x0c
#define  WR4_PAREVEN    0x00
#define  WR4_PARODD     0x02
#define  WR4_PAREN      0x01
#define WR5             5
#define  WR5_TX5BIT     0x00
#define  WR5_TX7BIT     0x20
#define  WR5_TX6BIT     0x40
#define  WR5_TX8BIT     0x60
#define  WR5_TXDIS      0x00
#define  WR5_TXEN       0x08
#define WR9             9
#define  WR9_VIS        0x01
#define  WR9_MIE        0x08
#define  WR9_RESETB     0x40
#define  WR9_RESETA     0x80
#define WR11            11
#define  WR11_RXRTxC    0x00
#define  WR11_RXTRxC    0x20
#define  WR11_RXBR      0x40
#define  WR11_RXDPLL    0x60
#define  WR11_TXRTxC    0x00
#define  WR11_TXTRxC    0x08
#define  WR11_TXBR      0x10
#define  WR11_TXDPLL    0x18
#define  WR11_TXDPLL    0x18
#define  WR11_TRxCO     0x00
#define  WR11_TRxCI     0x04
#define  WR11_TRxCXTAL  0x00
#define  WR11_TRxCTC    0x01
#define  WR11_TRxCBR    0x02
#define  WR11_TRxCDPLL  0x03
#define WR12            12
#define WR13            13
#define WR14            14
#define  WR14_DISABLE   0x00
#define  WR14_BREN      0x01
#define  WR14_BRSRC     0x02
#define WR15            15
#define RR0             0
#define  RR0_RXAVAIL    0x01
#define  RR0_TXEMPTY    0x04
#define RR1             1
#define  RR1_FERR       0x40
#define  RR1_OVERR      0x20
#define  RR1_PERR       0x10
#define  RR1_ALLSENT    0x01
#define RR3             3
#define  RR3_TXBPEND    0x02
#define  RR3_RXBPEND    0x04
#define  RR3_TXAPEND    0x10
#define  RR3_RXAPEND    0x20

static struct z85c30 *dev_reg[NOFSCC];
static int dev_intr[NOFSCC];
static int *baudtab;

struct sccrec {
    int off0;
    int (*iproc)();
    int (*oproc)();
    int (*sproc)();
    int (*eproc)();
    int iprocarg;
    int oprocarg;
    int sprocarg;
    int eprocarg;
} scc_rec[NOFSCC];

static int old_baudtab[] = {
    0,  2603,   1734,   1182,   985,    866,    649,    432,    
    215,106,    70,     52,     25,     12,     6,      3
};
static int new_baudtab[] = {
    0,  2603,   1734,   1182,   985,    866,    649,    432,
    215,86,     70,     42,     20,     9,      6,      3
};

extern int cbip_io_base[];
extern int cbip_intr_vec[];
extern int cbipdev[];
extern tab_rint(), kbd_rint();

#define SCC_OFFSET  0x00000D00  /* from cbip.h */

cbip_scc_init()
{
    dev_reg[SCCA] = (struct z85c30*)(cbip_io_base[0] | (SCC_OFFSET + 8));
    dev_reg[SCCB] = (struct z85c30*)(cbip_io_base[0] | (SCC_OFFSET + 0));
    dev_reg[SCCC] = (struct z85c30*)(cbip_io_base[1] | (SCC_OFFSET + 8));
    dev_reg[SCCD] = (struct z85c30*)(cbip_io_base[1] | (SCC_OFFSET + 0));
    dev_intr[SCCA] = cbip_intr_vec[0];
    dev_intr[SCCB] = cbip_intr_vec[0];
    dev_intr[SCCC] = cbip_intr_vec[1];
    dev_intr[SCCD] = cbip_intr_vec[1];
}

int scc_ioctl(dev, func, arg, arg2)
dev_t dev;
short func;
caddr_t arg;
caddr_t arg2;
{
    int baud, unused, ch;
    
    switch (func) {
    case SCC_RESET:
        dev_reg[dev]->cmd = WR9;
        if (dev & 1)
            dev_reg[dev]->cmd = WR9_RESETB;
        else 
            dev_reg[dev]->cmd = WR9_RESETA;
        break;
    
    case SCC_EMPTY:
        return (dev_reg[dev]->cmd & RR0_RXAVAIL) == 0;
        
    case SCC_FLUSH:
        dev_reg[dev]->cmd = RR0;
        while (dev_reg[dev]->cmd & RR0_RXAVAIL) {
            scc_ioctl(dev, SCC_GET, 0);
            dev_reg[dev]->cmd = RR0;
        }
        return; /* sloppy code: no return value */

    case SCC_SPEED:
        baud = (int)arg;
        if (baud == 0) {
            dev_reg[dev]->cmd = WR14;
            dev_reg[dev]->cmd = WR14_DISABLE;
            return 1;
        }
        
        baud = baudtab[baud & 15];
        dev_reg[dev]->cmd = 12;
        dev_reg[dev]->cmd = baud & 0xff;
        dev_reg[dev]->cmd = 13;
        dev_reg[dev]->cmd = (baud>>8) & 0xff;
        dev_reg[dev]->cmd = 14;
        dev_reg[dev]->cmd = WR14_BREN|WR14_BRSRC;
        return 0;
    
    case SCC_GET:
        ch = dev_reg[dev]->data;
        dev_reg[dev]->cmd = WR0;
        dev_reg[dev]->cmd = WR0_RSTHIUS;
        return ch & 0xff;
    
    case SCC_PUT:
        dev_reg[dev]->cmd = WR0;
        dev_reg[dev]->cmd = WR0_RSTTXINT;
        dev_reg[dev]->cmd = WR0;
        dev_reg[dev]->cmd = WR0_RSTHIUS;
        dev_reg[dev]->data = (char)(int)arg;
        break;
    
    case SCC_STATE:
        dev_reg[dev]->cmd = RR1;
        ch = dev_reg[dev]->cmd & (RR1_OVERR|RR1_PERR);
        if (ch) {
            dev_reg[dev]->cmd = WR0;
            dev_reg[dev]->cmd = WR0_RSTESINT;
            dev_reg[dev]->cmd = WR0_RSTERR;
        }
        return ch;
        
    case SCC_IPROC:
        scc_rec[dev].iproc = (int (*)())arg2;
        scc_rec[dev].iprocarg = (int)arg;
        break;
        
    case SCC_OPROC:
        scc_rec[dev].oproc = (int (*)())arg2;
        scc_rec[dev].oprocarg = (int)arg;
        break;
        
    case SCC_SPROC:
        scc_rec[dev].sproc = (int (*)())arg2;
        scc_rec[dev].sprocarg = (int)arg;
        break;
        
    case SCC_EPROC:
        scc_rec[dev].eproc = (int (*)())arg2;
        scc_rec[dev].eprocarg = (int)arg;
        break;
    }
    return 0;
}

scc_open(dev)
int dev;
{
    register struct z85c30 *scp  = dev_reg[dev];
    register struct sccrec *recp = &scc_rec[dev];
    int unused;

    if (dev >= 4)
        return;

    if (cbipdev[dev >> 1])
        baudtab = new_baudtab;
    else
        baudtab = old_baudtab;

    recp->iproc = 0;
    recp->oproc = 0;
    recp->sproc = 0;
    recp->eproc = 0;

    scp->cmd = WR9;
    if (dev & 1)
        scp->cmd = WR9_RESETB;
    else
        scp->cmd = WR9_RESETA;

    scp->cmd = WR3;
    scp->cmd = WR3_RX8BIT|WR3_RXEN;

    scp->cmd = WR4;
    scp->cmd = WR4_X16|WR4_1STOP;

    scp->cmd = WR5;
    scp->cmd = WR5_TX8BIT|WR5_TXEN;
    
    scp->cmd = WR9;
    scp->cmd = WR9_MIE;
    
    scp->cmd = WR11;
    scp->cmd = WR11_RXBR|WR11_TXBR|WR11_TRxCI|WR11_TRxCBR;
    scc_ioctl(dev, SCC_SPEED, B1200);
    
    scp->cmd = WR15;
    scp->cmd = 0;       /* no interrupt enables */
    
    scp->cmd = WR2;
    scp->cmd = dev_intr[dev] >> 2;  /* interrupt vector */

    scp->cmd = WR1;
    scp->cmd = WR1_RXINTALL;
    
    scp->cmd = WR0;
    scp->cmd = WR0_RXINTEN;
}

int scc_close(dev)
int dev;
{
    if (dev >= 4)
        return 1;

    dev_reg[dev]->cmd = WR1;
    dev_reg[dev]->cmd = WR1_NOINT;

    dev_reg[dev]->cmd = WR3;
    dev_reg[dev]->cmd = WR3_RXDIS;

    dev_reg[dev]->cmd = WR5;
    dev_reg[dev]->cmd = WR5_TXDIS;

    return 0;
}

int scc_read(dev)
dev_t dev;
{
    register int ch;

    while ((dev_reg[dev]->cmd & RR0_RXAVAIL)==0);
    
    ch = dev_reg[dev]->data;
    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTHIUS;
    return ch & 0xff;
}

scc_write(dev, ch)
int dev;
int ch;
{
    while ((dev_reg[dev]->cmd & RR0_TXEMPTY)==0);

    dev_reg[dev]->data = ch;
    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTHIUS;
}

scc_writes(dev, s)
int dev;
char *s;
{
    while (*s != '\0')
        scc_write(dev, *s++);
}

scc_xint(dev)
register int dev;
{
    register struct sccrec *recp = &scc_rec[dev];

    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTTXINT;
    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTHIUS;
    if (recp->oproc)
        (*recp->oproc)(recp->iprocarg); /* BUG: should have been oprocarg! */
}

scc_rint(dev)
register int dev;
{
    register struct sccrec *recp = &scc_rec[dev];
    register int ignored;

    if (recp->iproc)
        (*recp->iproc)(recp->iprocarg);
    else {
        ignored = dev_reg[SCCB]->data;
        dev_reg[SCCB]->cmd = WR0_RSTHIUS;
    }
}

scc_eint(dev)
int dev;
{
    register struct sccrec *recp = &scc_rec[dev];
    int unused, state;
    
    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTESINT;
    
    dev_reg[dev]->cmd = WR0;
    state = dev_reg[dev]->cmd;

    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTHIUS;
}

scc_sint(dev)
int dev;
{
    register struct sccrec *recp = &scc_rec[dev];
    register int ignored, state;

    dev_reg[dev]->cmd = WR1;
    state = dev_reg[dev]->cmd & 0xff;

    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTESINT;

    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTERR;

    ignored = dev_reg[dev]->data;

    dev_reg[dev]->cmd = WR0;
    dev_reg[dev]->cmd = WR0_RSTHIUS;
}

scc_reset(dev)
int dev;
{
    dev_reg[dev]->cmd = WR9;
    dev_reg[dev]->cmd = WR9_RESETA|WR9_RESETB;
}

sccintr(dev)
register int dev;
{
    register int sccdev = dev << 1;
    register int sccdev2 = sccdev + 1;
    int scca = sccdev;
    int rr3;
    int error;
    
    dev_reg[sccdev]->cmd = RR3;
    rr3 = dev_reg[sccdev]->cmd;
    if (rr3 & RR3_RXBPEND) {
        dev_reg[sccdev2]->cmd = RR1;
        error = dev_reg[sccdev2]->cmd;
        if (error & (RR1_FERR|RR1_OVERR|RR1_PERR)) {
            dev_reg[sccdev2]->cmd = WR0;
            dev_reg[sccdev2]->cmd = WR0_RSTESINT;
            dev_reg[sccdev2]->cmd = WR0_RSTERR;
        }
        tab_rint(dev);
    }
    if (rr3 & RR3_RXAPEND) {
        dev_reg[scca]->cmd = RR1;
        error = dev_reg[scca]->cmd;
        if (error & (RR1_FERR|RR1_OVERR|RR1_PERR)) {
            dev_reg[scca]->cmd = WR0;
            dev_reg[scca]->cmd = WR0_RSTESINT;
            dev_reg[scca]->cmd = WR0_RSTERR;
        }
        kbd_rint(dev);
    }
}
