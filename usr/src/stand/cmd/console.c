#include "data.h"
#include <sys/bmt/gdi.h>
#include <sys/bmt/gdisys.h>
#include <sys/bmt/rtk.h>
#include <sys/bmt/bmt.h>

mfp_write(ch)
register int ch;
{
    int dly = 10000;
    register char *ioaddr = (char*)MFP;

    do {
        if (ioaddr[0x2d] & 0x80) break; /* transmitter become ready */
    } while (--dly != 0);

    if (dly == 0)
        print_timeout(8);               /* timeout of device 8 = MFP */
    else
        ioaddr[0x2f] = ch;              /* transmitter data */
}

int mfp_read()
{
    register char *ioaddr = (char*)MFP;
    while (!(ioaddr[0x2b] & 0x80));     /* received data? no, wait */
    
    return ioaddr[0x2f] & 0x7f;         /* read receiver data */
}

int mfp_haschar()
{
    register char *ioaddr = (char*)MFP;
    return ioaddr[0x2b] & 0x80;         /* return 0 or != 0 */
}

int ser_read()
{
    register short *ioaddr = (short*)DLV11ADDR;
    while (!(ioaddr[0] & 0x80));        /* CSR data available? no, wait */
    
    return ioaddr[1] & 0x7f;            /* read received data */
}
    
/* returns 0 if not found, != 0 if BIP present */
bip_write(ch)
char ch;
{
    int dly=1000, i;
    register Bip_device *bip = (Bip_device*)BIPADDR;

    bip->map1 = RAM_MAP;                /* select RAM */
    bip->intr_state = 0;

    while (bip->func != 0 && --dly != 0) { /* wait BIP becoming ready */
        for (i=0; i < 2000; i++);       /* more delay */
    }

    bip->wbuf[0] = ch;                  /* put in write buffer */
    bip->args[0] = 1;                   /* # of chars = 1 */
    bip->func = 1;                      /* function "write chars" */

    if (i == 0 && (con_type & 0x20))
        print_timeout(5);               /* timeout of device 5 = BIP */
        
    return i;                           /* return 0 if error */
}

int bip_read()
{
    int ch;
    register Bip_device *bip = (Bip_device*)BIPADDR;
    
    bip->map1 = RAM_MAP;
    bip->intr_state = 0;
    bip->map1 = RAM_MAP;

    while (bip->keys[0]==0)             /* no key entered yet? */
        bip->intr_state = 0;            /* reset interrupt flag */

    ch = bip->keys[0];                  /* get key */
    bip->keys[0] = 0;                   /* clear key buffer */
    return ch;
}

int bip_haschar()
{
    register Bip_device *bip = (Bip_device*)BIPADDR;

    bip->map1 = RAM_MAP;
    
    return bip->keys[0] != 0;
}

/* see /usr/sys/io/cons.c */
dh11_write(ch)
char ch;
{
    static int dh_phys;                 /* phys addr for DH11 */

    int dly = 500;
    register short *ioaddr = (short*)DH11ADDR;

    if (!dh_inited) {
        ioaddr[5] = 0;                  /* clear BAR register */
        dh_inited++;                    /* make initialized */
        ioaddr[2] = 0x3743;             /* set LPR speed=13,13, 8 bit no parity */
        dh_phys = (int)dh_char - SYSVA; /* logtophys(&dh_char) */
        
        while (--dly != 0);             /* wait */
    }

    dly = 10000;
    while (!(ioaddr[5] & 1) && --dly != 0); /* wait for transmitter become ready */

    if (dly==0) {
        print_timeout(3);               /* timeout for device 3 DH11 */
        return;
    }

    ioaddr[0] = 0;                      /* clear SCR */
    ioaddr[0] = (dh_phys >> 16) << 4;   /* set memory extension bits */
    dh_char[0] = ch;                    /* store char */
    dh_char[1] = ch;                    /* to be safe from byte sex, store twice */
    ioaddr[3] = dh_phys;                /* set high address in CAR */
    ioaddr[4] = -1;                     /* 1 byte in BCR */
    ioaddr[7] = 0x4000;                 /* set SSR */
    ioaddr[3] = dh_phys >> 16;          /* set high address in CAR */
    ioaddr[5] |= 1;                     /* transmit enable line 0 */
}

int dh11_read()
{
    int ch;
    register short *ioaddr;

    if (dh11_char) {                    /* pending char? */
        ch = dh11_char;
        dh11_char = 0;
        return ch;                      /* yes, return it */
    }

    ioaddr = (short*)DH11ADDR;
    while (1) {
        ch = ioaddr[1];                 /* NRCR */
        if (ch < 0 && (ch & 0xf00)==0)  /* valid and no error ? */
            return ch & 0x7f;
    }
}

int dh11_haschar()
{
    register short *ioaddr = (short*)DH11ADDR;
    int ch = ioaddr[1];                 /* NRCR */
    if (ch < 0 && (ch & 0xf00)==0) {    /* valid and no error */
        dh11_char = ch & 0x7f;
        return 1;                       /* has char */
    } else
        return 0;
}

int cons_detect()
{
    int i, k;
    
    con_type = 0;
    
    if (mem_readword(DH11ADDR) != -1)   /* device at DH11 address ? */
        con_type |= 8;                  /* yes, set flag */

    if (mem_readword(BIPGO) != -1) {    /* found BIP? */
        for (i=0; i < 3; i++) {
            *(short*)BIPGO = STOP;      /* send STOP */
            for (k=0; k < 1000; k++);   /* delay */
        
            *(short*)BIPGO = RUN;       /* send START */

            bip_write('\n');            /* send 2 CRLF */
            if (bip_write('\n'))        /* if success, found BIP */
                break;
        }
        con_type |= 0x20;               /* found BIP */
    }

    if (mem_readword(COLORRAM) != -1 &&
        mem_readword(COLORCTL) != -1) {
            col_init();
            con_type |= 0x200;          /* found COLOR term */
    }

    if (mem_writeword(ICCADDR) != -1)   /* BUG: writeword with random data */
        con_type |= 0x80;               /* found ICC port */

    if (mem_readbyte(MFP+0x2d) != -1)   /* MFP receiver data */
        con_type |= 0x100;              /* found MFP */
}

putchar(ch)
int ch;
{
    int is_pending, chread;

    if (con_type == 0)                  /* no known consoles? find them */
        cons_detect();

    if (con_type & 8)                   /* write out on every available console */
        dh11_write(ch);
    if (con_type & 0x20)
        bip_write(ch);
    if (con_type & 0x200)
        col_write(ch);
    if (con_type & 0x80)
        scc_write(ch);
    if (con_type & 0x100)
        mfp_write(ch);

    if (is_pending = haschar()) {       /* some char received? */
        chread = cons_read();
        if (chread == 0x13) {           /* is it XOFF? */
            do {
                chread = cons_read();   /* wait until XON seen */
            } while (chread != 0x11);
        }
    }
    
    if (ch == '\n')                     /* send CR after LF */
        putchar('\r');
}

/* read, but do not echo */
int cons_read(dev)
int dev;                                /* device number */
{
    switch (dev) {
    case 3:
        return dh11_read();
    case 5:
        return bip_read();
    case 9:
        return col_read();
    case 7:
        return scc_read();
    case 8:
        return mfp_read();
    default:
        return 0;
    }
}

int getchar()
{
    int ch;
    
redo:
    switch (con_type) {
    case 0x08:
        ch = dh11_read();
        break;
    case 0x20:
        ch = bip_read();
        break;
    case 0x200:
        ch = col_read();
        break;
    case 0x80:
        ch = scc_read();
        break;
    case 0x100:
        ch = mfp_read();
        break;
    case 0:
        cons_detect();
        goto redo;
    default:
        while ((ch = haschar())==0);
        con_type = 1<<ch;
        goto redo;
    }

    if (ch == '\r')                     /* convert CR to LF */
        ch = '\n';
    else if (ch == 0x08 || ch == 0x7f) { /* backspace or DEL ? */
        if (echo) {
            putchar(8);                 /* overtype last char */
            putchar(' ');
            putchar(8);         
        }
        ch = 0x08;                      /* convert DEL to BS */
    } else if (ch == 0x11 || ch == 0x13) /* XON or XOFF? */
        goto redo;
    
    if (echo)
        putchar(ch == 0x18 ? '@' : ch); /* convert CTRL-X to @, and echo */
}

/* returns device# */
int haschar()
{
    if (con_type == 0)
        cons_detect();
    
    if ((con_type & 8) && dh11_haschar())
        return 3;
    else if ((con_type & 0x20) && bip_haschar())
        return 5;
    else if ((con_type & 0x200) && col_haschar())
        return 9;
    else if ((con_type & 0x80) && scc_haschar())
        return 7;
    else if ((con_type & 0x100) && mfp_haschar())
        return 8;
    else return 0;
}

char* cons_devices[] = {
    "??", "KL", "DZ", "MUX", "SL", "BMT", "DL", "ICC", "SCO", "CLT" };

/* print out "console is on NAME\n"
 * requires con_type to be != 0 
 */
name_console()
{
    register int dev;
    
    if (con_type & 8)
        dev = 3;
    else if (con_type == 0x20)
        dev = 5;
    else if (con_type == 0x200)
        dev = 9;
    else if (con_type == 0x80)
        dev = 7;
    else if (con_type == 0x100)
        dev = 8;

    printf("Console is on %s\n", cons_devices[dev]);
    
    con_type = 1 << dev;                /* if con_type had more than one bit set,
                                         * now fix it to the first one used */
}

/* disable failing console device and
 * notify console timeout on remaining devs */
print_timeout(dev)
int dev;
{
    con_type &= ~(1<<dev);              /* disable this failed console */
    if (con_type == 0) {
        cons_detect();                  /* no one left? detect consolse again */
        con_type &= ~(1<<dev);          /* and disable failed console again */
    }

    printf("\n%s timeout\n", cons_devices[dev]);
}
