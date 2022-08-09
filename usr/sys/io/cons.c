/* @(#)cons.c   6.2 */
/*
 *  Console/LSI interface
 */
static  char *_Version = "@(#) RELEASE:  1.4  Jul 16 1987 /usr/sys/io/cons.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/buf.h"
#include "sys/systm.h"
#include "sys/conf.h"
#include "sys/sysinfo.h"

#define MSGBUFSZ 1024
extern nodev();
caddr_t msgrp;
char    msgbuf[MSGBUFSZ];
char    ovmsgb;
caddr_t msgbufp = &msgbuf[0];
char    msgslp = 0;
char    nprinterr = 0;

int con_type = 0;

extern char _mfp[];

#define SL_ADDR 0x3ffffc20  /* QBUS 776040 */
#define SL_WAIT 30000

sl_putchar(ch)
register ch;
{
    register int dly = SL_WAIT;
    register short *ioaddr = (short*)SL_ADDR;   

    while ((ioaddr[1] & 1) == 0)
        if (--dly == 0) break;
    ioaddr[8] = ch;
        
    dly = SL_WAIT;
    while ((ioaddr[1] & 1) == 0)
        if (--dly == 0) return;
}

sl_getchar()
{
    register short *ioaddr = (short*)SL_ADDR;
    
    while ((ioaddr[0] & 1) == 0) ;
    return ioaddr[8] & 0x7f;
}

#define KL_ADDR 0x3ffffd58  /* QBUS 776530 */
#define KL_WAIT 30000

kl_putchar(ch)
register int ch;
{
    register int save;
    register int dly = KL_WAIT;
    register short *ioaddr = (short*)KL_ADDR;   

    while ((ioaddr[2] & 0x80) == 0)
        if (--dly == 0) break;
    
    if (ch) {
        save = ioaddr[2];
        ioaddr[2] = 0;
        ioaddr[3] = ch;
        kl_putchar(0);
        ioaddr[2] = save;
    }
}

kl_getchar()
{
    register short *ioaddr = (short*)KL_ADDR;
    
    while ((ioaddr[0] & 0x80) == 0) ;
    return ioaddr[1] & 0x7f;
}

#define DH_ADDR 0x3fffe010  /* QBUS 760020 */
#define DH_WAIT 500

#define DH_SCR  ioaddr[0]
#define DH_SCRL ((char*)ioaddr)[1]
#define DH_NRCR ioaddr[1]
#define DH_LPR  ioaddr[2]
#define DH_CAR  ioaddr[3]
#define DH_BCCR ioaddr[4]
#define DH_BAR  ioaddr[5]
#define DH_BCR  ioaddr[6]
#define DH_SSR  ioaddr[7]

char  dh_char[2] = { 0, 0 };
short dh_inited = 0;

dh_putchar(ch)
char ch;
{
    register short *ioaddr = (short*)DH_ADDR;
    register short save_scr, save_car;

    static int dh_phys;

    short dummy;
    short s;
    int dly;

    if (dh_inited == 0) {
        dly = DH_WAIT;
        DH_BAR = 0;
        dh_inited++;
        DH_LPR = 0x3743; /* speed = 13,13, 8 bit, no parity */
        dh_phys = logtophys(dh_char);
        while (dly-- != 0) ;
    }
    
    s = (short)splhi();

    while (DH_BAR & 1) ;
    for (dly = 2*DH_WAIT; dly > 0; dly--) ;

    save_scr = DH_SCR;
    DH_SCRL = 0x40; /* received interrupt enable */
    save_car = DH_CAR;
    DH_SCR = ((dh_phys >> 16) & 3) << 4; /* memory extension bits */
    dh_char[0] = ch;
    dh_char[1] = ch;
    DH_CAR = dh_phys;
    DH_BCCR = 0xffff;
    DH_SSR = 0x4000;
    DH_CAR = dh_phys >> 16;
    DH_BAR = DH_BAR | 1;

    while (DH_BAR & 1) ;
    for (dly = 2*DH_WAIT; dly > 0; dly--) ;
    DH_CAR = save_car;
    DH_SCR = save_scr;
    
    splx(s);
}

dh_getchar()
{
    register short *ioaddr = (short*)DH_ADDR;
    int nrcr;
    
    for (;;) {
        nrcr = DH_NRCR;
        if (nrcr < 0 && (nrcr & 0xf00) == 0)
            return nrcr & 0x7f;
    }
}

#define DZ_ADDR 0x3fffe040

#define DZ_CSR  ioaddr[0]
#define DZ_RBUF ioaddr[1]
#define DZ_LPR  ioaddr[1]
#define DZ_TCR  ioaddr[2]
#define DZ_TCRL ((char*)ioaddr)[5]
#define DZ_MSR  ioaddr[3]
#define DZ_TDR  ioaddr[3]
#define DZ_TDRL ((char*)ioaddr)[7]

dz_putchar(ch)
char ch;
{
    static int dz_inited;
    register short *ioaddr = (short*)DZ_ADDR;
    if (dz_inited == 0) {
        DZ_CSR = 0x10;
        while (DZ_CSR & 0x10) ;
        DZ_CSR = 0x20;
        DZ_LPR = 0x1e00;
        DZ_TCRL= 1;
        dz_inited++;
    }
    
    while (DZ_CSR >= 0 || (DZ_CSR & 0x300)) ;
    DZ_TDRL = ch;
}

dz_getchar()
{
    register short *ioaddr = (short*)DZ_ADDR;
    int ch;

    do {
        while ((DZ_CSR & 0x20) == 0) ;
        ch = DZ_RBUF;
    } while ((ch & 0x8300) != 0x8000);
    return ch & 0x7f;
}   


#define BIP_ADDR    0x3fe00000
#define BIP_WAIT    20000

/* refer to sys/bitmap.h */
#define BIP_FUNC            (*(long*)&ioaddr[0x3f000])
#define BIP_ARGS            ((long*)&ioaddr[0x3f004])
#define BIP_FUNCRET         (*(long*)&ioaddr[0x3f01c])
#define BIP_WBUF            ((char*)&ioaddr[0x3f020])
#define BIP_KEYS            ((char*)&ioaddr[0x3f420])
#define BIP_FUNCS           ((char*)&ioaddr[0x3f440])
#define BIP_INTR_STATE      (*(short*)&ioaddr[0x3f460]) 
#define BIP_GO              (*(short*)&ioaddr[0x3ffe0])
#define BIP_INTR_LOCAL68    (*(short*)&ioaddr[0x3ffe2])
#define BIP_MAP0            (*(short*)&ioaddr[0x3ffe4])
#define BIP_MAP1            (*(short*)&ioaddr[0x3ffe6])

int what_proms[3]; /* Never referenced, only in symbol table */
extern short bmt_newmode[];
short Bip_bmap[] = { 0x2, 0x2, 0x2, 0x2, 0x2, 0x2 };

bip_putchar(ch)
{
    short s;
    int dly = 20000;
    register char *ioaddr = (char*)BIP_ADDR;

    if (bmt_newmode[0]) {
        bmt_putchar(ch);
        return;
    }
    BIP_MAP1 = 2; /*map1*/
    BIP_INTR_STATE = 0;

    while (BIP_FUNC != 0) {
        if (dly-- < 0) {
            BIP_MAP1 = Bip_bmap[0];
            return;
        }
    }
    
    s = currpl();
    spl6();
    
    BIP_WBUF[0] = ch;
    BIP_ARGS[0] = 1;
    BIP_FUNC = 1;
    BIP_MAP1 = Bip_bmap[0];

    splx(s);
}

short bip_getchar()
{
    register char *ioaddr;
    short ch;

    if (bmt_newmode[0])
        return bmt_getchar();
    
    ioaddr = (char*)BIP_ADDR;

    BIP_MAP1 = 2;
    BIP_INTR_STATE = 0;
    BIP_MAP1 = 2;
    
    while (BIP_KEYS[0] == 0)
        BIP_INTR_STATE = 0;

    ch = BIP_KEYS[0];
    BIP_KEYS[0] = 0;
    BIP_MAP1 = Bip_bmap[0];
    return ch;
}

#define SCC_ADDR 0x3fffe300
#define SCC_WAIT 1000
#define SCC_WAIT2 100000

short *scc_addr;
short cr2image;
extern int icc_pikprint;

int icc_noprint = 1;

scc_getchar()
{
    register s;
    register short *ioaddr;
    int dly;

    if (icc_noprint)
        return 0;
    
    if (icc_pikprint)
        return icc_getchar();
    
    s = splhi();
    scc_addr[0] |= 1;
    ioaddr = (short*)SCC_ADDR;
    cr2image = ~cr2image;
    ioaddr[2] = cr2image;
    ioaddr[3] = 0x40;
    
    while (scc_addr[0] & 1)
        clrcache();
    
    for (dly = 0; dly < SCC_WAIT; dly++) ;
    splx(s);
    
    return scc_addr[1];
}

scc_putchar(ch)
register int ch;
{
    register s;
    register short *ioaddr;
    int dly;
    int dly2;

    if (icc_noprint)
        return;
    
    if (icc_pikprint) {
        icc_putchar(ch);
        return;
    }
    
    s = splhi();

    for (dly = SCC_WAIT; (scc_addr[2] & 1) != 0 && dly-- != 0; )
        clrcache();

    scc_addr[3] = ch;
    scc_addr[2] |= 1;
    ioaddr = (short*)SCC_ADDR;
    cr2image = ~cr2image;
    ioaddr[2] = cr2image;
    ioaddr[3] = 0x40;
    
    for (dly = SCC_WAIT2; (scc_addr[2] & 1) != 0 && dly-- != 0; )
        clrcache();

    for (dly2 = 0; dly2 < (2*SCC_WAIT); dly2++) ;
    splx(s);
}

#define MFP_ADDR    _mfp
#define MFP_WAIT    30000

#define MFP_RSTATUS ioaddr[0x2d]
#define MFP_WSTATUS ioaddr[0x2b]
#define MFP_DATA    ioaddr[0x2f]

mfp_putchar(ch)
register ch;
{
    register int dly = MFP_WAIT;
    register char *ioaddr = MFP_ADDR;

    while ((MFP_RSTATUS & 0x80) == 0)
        if (--dly == 0) break;
    
    MFP_DATA = ch;
    
    dly = MFP_WAIT;
    while ((MFP_RSTATUS & 0x80) == 0)
        if (--dly == 0) return;
}

mfp_getchar()
{
    register char *ioaddr = MFP_ADDR;
    
    while ((MFP_WSTATUS & 0x80) == 0);
    return MFP_DATA & 0x7f;
}


putchar(ch)
{
    if (ch != 0 && ch != '\r' && ch != 0x7f) { 
        *msgbufp++ = ch;
        if (msgbufp == msgrp)
            ovmsgb = 1;

        if (msgbufp >= &msgbuf[MSGBUFSZ])
            msgbufp = msgbuf;

        if (msgslp) {
            msgslp = 0;
            wakeup((caddr_t)&msgbufp);
        }
        
        if (panicstr == 0 && nprinterr)
            return;
    }
    
    switch (con_type) {
    case 1:
        kl_putchar(ch);
        break;
    case 2:
        dz_putchar(ch);
        break;
    case 3:
        dh_putchar(ch);
        break;
    case 4:
        sl_putchar(ch);
        break;
    case 5:
        bip_putchar(ch);
        break;
    case 9:
        col_putchar(0, ch);
        break;
    case 6:
        kl_putchar(ch);
        break;
    case 7:
        scc_putchar(ch);
        break;
    default:
        con_type = 8;
        /*FALLTHRU*/
    case 8:
        mfp_putchar(ch);
    }

    if (ch == '\n')
        putchar('\r');
}

getchar()
{
    char ch;

    switch (con_type) {
    case 1:
        ch = kl_getchar();
        break;
    case 2:
        ch = dz_getchar();
        break;
    case 3:
        ch = dh_getchar();
        break;
    case 4:
        ch = sl_getchar();
        break;
    case 5:
        ch = bip_getchar();
        break;
    case 9:
        ch = col_getchar(0);
        break;
    case 6:
        ch = kl_getchar();
        break;
    case 7:
        ch = scc_getchar();
        break;
    default:
    case 8:
        ch = mfp_getchar();
    }

    if (ch == '\r')
        ch = '\n';
    else if (ch == 0x08 || ch == 0x7f) {    /* BS or DEL */
        putchar(0x08);
        putchar(' ');
        ch = 0x08;
    }
    putchar(ch);
    return ch;
}

chconsdev(num)
{   
    if (num > 0 && num <= 9)
        con_type = num;
    else
        switch (con_type) {
        case 0x02:
            con_type = 1;
            break;
        case 0x04:
            con_type = 2;
            break;
        case 0x08:
            con_type = 3;
            break;
        case 0x10:
            con_type = 4;
            break;
        case 0x20:
            con_type = 5;
            break;
        case 0x200:
            con_type = 9;
            break;
        case 0x40:
            con_type = 6;
            break;
        case 0x80:
            con_type = 7;
            break;
        default:
            con_type = 8;
        }

    if (con_type == 9)
        col_init();

    if (con_type == 1 && conssw[1].d_open == nodev)
        con_type = 6;
    
    cdevsw[0] = conssw[con_type];
}
