/*PCS specific */
#include <sys/types.h>
#include "sa.h"
#include "hk.h"

static char *_Version = "@(#) RELEASE:  2.0  Sep 09 1986 /usr/sys/startup/hkcheck.c ";

#define HK32ADDR  (struct device*)0x3fffff20
#define HK32ADDR2 (struct device*)0x3ffffdc0

static struct hkhd {
    ushort cyl;
    ushort trksec;
    ushort chk;
} header[NSECT];
static short drvtype;
static short ctrlflag;

extern struct upar upar;
extern int logtophys();

int chkopen(ctrl)
short ctrl;
{
    register struct device *devp;
    register struct upar *up = &upar;
    int i;
    int probe;
    
    ctrlflag = ctrl;
    if (ctrlflag == 4)
        devp = (struct device*)HK32ADDR;
    else
        devp = HK32ADDR2;

    if (fsword((caddr_t)devp, &probe) == -1)
        return 0;                       /* no such device */

    drvtype = RK06;
    up->nsecs = RK06BL;
    up->name = "RK06";
    up->ncyl = NCYL6;
    up->trkcyl = NTRAC;
    up->secsz = HKSECSZ;
    up->sectrk = NSECT;

    for (i=0; i < NHK; i++) {                 /* formerly part of a loop over units...*/

        devp->hkcs2 = SCLR;                 /* controller clear */
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

        devp->hkcs2 = up->unit;             /* select unit */
        devp->hkcs1 = GO;   
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

        if (devp->hkcs1 & CERR) {         /* error */
            if (devp->hkcs2 & NED) {
                printf("\nNon-Existent Drive\n");
                return 0;
            }
            if ((devp->hkds & DRDY)==0) {
                printf("\nDrive not ready\n");
                return 0;
            }

            if (devp->hkerr & DTYE) {
                drvtype = RK07;
                up->nsecs = RK07BL;
                up->ncyl = NCYL7;
                up->name = "RK07";
                return 1;
            }
        } else
            return 1;
    }

    printf("\nCannot open HK unit %d\n", up->unit);
    return 0;
}

chkio(secno, buf, sz, cmd)
int secno;
caddr_t buf;
ushort sz;
ushort cmd;
{
    register struct device *devp;
    register struct upar *up = &upar;
    int retries;
    int pbuf;
    ushort cyl,sec, trk;
    ushort hkcmd;
    
    
    retries = 1;
    if (ctrlflag == 4)
        devp = HK32ADDR;
    else
        devp = HK32ADDR2;

    switch (cmd) {
    case DEV_WRITE:                     /*write*/
        hkcmd = WCOM; break;
    case DEV_WCHK:                      /*write check*/
        hkcmd = WCHK; break;
    case DEV_READ:                      /*read*/
        hkcmd = RCOM; break;
    case DEV_TRKFMT:                    /*format track*/
        hkcmd = WHDR; break;
    }
    
    pbuf = logtophys(buf);
    hkcmd = drvtype | hkcmd | (((pbuf >> 16) & 3) << 8); /* drivetype and bits 17-18 */
    
    cyl = secno / (NTRAC*NSECT);
    sec = secno % (NTRAC*NSECT);
    trk = sec / NSECT;
    sec = sec % NSECT;
    
    while (retries-- != 0) {
        devp->hkcs2 = SCLR;
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */
        
        devp->hkcs2 = up->unit;
        devp->hkcs1 = drvtype | GO;
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

        if ((devp->hkds & VV)==0) {
            devp->hkcs1 = drvtype | PAKACK| GO;
            while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */
        }

        devp->hkdc = cyl;
        devp->hkda = (trk << 8) | sec;
        devp->hkba = (short)pbuf;
        devp->hkla = (pbuf >> 16) | 022000;
        devp->hkwc = -(sz >> 1);
        devp->hkcs1 = hkcmd | GO;
        
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */
        
        if ((devp->hkcs1 & CERR)==0)      /* no error? */
            return 1;

        if (sz != up->secsz) continue;      /* transfer not complete */

        while ((devp->hkds & SVAL)==0);   /* wait */
        
        printf("HK disk error: cs1=%x cs2=%x ds=%x er=%x\n",
            devp->hkcs1, devp->hkcs2, devp->hkds, devp->hkerr);

        devp->hkcs2 = SCLR;
        devp->hkcs1 = CCLR|GO;              /* clear error */
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */
    }
    return 0;
}

chkftrk(trkno, badsec)
int trkno;                  /* absolute track#, not respecting cyl and trk within cyl */
char *badsec;
{
    register int i;
    ushort cyl, trk;        /* real cyl#, trk# within cyl */
    ushort trksec;
    
    
    cyl = trkno / NTRAC;
    trk = trkno % NTRAC;
    
    for (i=0; i < NSECT; i++) {  /* sectors */
        header[i].cyl = cyl;
        trksec = (trk << 5) | i | 0xc000;
        if (badsec[i] == 1) 
            trksec = trksec & 0x3fff;       /* mark as bad */
        header[i].trksec = trksec;
        header[i].chk = (cyl & (ushort)~trksec) | ((ushort)~cyl & trksec);
    }

    if (chkio(trkno*NSECT, header, sizeof(header), 22) == 0) /*format track */
        printf("HK format error: sector %ld to %ld\n\t\t (cyl=%d track=%d)\n",
            trkno*NSECT, ((trkno+1)*NSECT)-1, cyl, trk);
}

cemufmt()
{
    register struct device *devp;
    register struct upar *up = &upar;

    if (ctrlflag == 4)
        devp = HK32ADDR;
    else
        devp = HK32ADDR2;

    printf("(for EMULEX SC02 controller)\n");

    devp->hkcs2 = SCLR;
    while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

    devp->hkcs2 = up->unit;
    devp->hkla = 0x8000;
    while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

    devp->hkcs1 = drvtype | GO;
    while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

    if ((devp->hkds & VV) == 0) {
        devp->hkcs1 = drvtype | PAKACK | GO;
        while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */
    }

    devp->hkmr3 = -1;
    while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

    devp->hkcs1 = drvtype | WHDR | GO;  /* format */
    while ((devp->hkcs1 & CRDY)==0);    /* wait controller ready */

    if ((devp->hkcs1 & CERR)==0)      /* no error? */
        return;

    while ((devp->hkds & SVAL)==0);   /* wait */
    printf("HK format error: cs1=%x cs2=%x ds=%x er=%x\n",
        devp->hkcs1, devp->hkcs2, devp->hkds, devp->hkerr);
}
