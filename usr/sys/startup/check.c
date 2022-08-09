/* PCS specific */
/* appears to be a disk check for RK05 devices */

#include <sys/types.h>
#include "dkbad.h"      /* from 16 bit Munix, stored locally */
#include "sa.h"

static char* _Version = "@(#) RELEASE:  4.0  Sep 09 1986 /usr/sys/startup/check.c ";

extern int tbf[];
extern char cap();

static int dateseed = 3;
static int secseed = 7;
static char line[100];
static char badsec[60];
static int *rbuffer = &tbf[0];
static int *wbuffer = &tbf[4500];
int *hddat = &tbf[9000];
static struct dkbad dkbad;
static short i_dev;

struct upar upar;
struct cpar cpar;

/* someone reinvented strncasecmp */
static int EQ(s1, s2, n)
char *s1;
char *s2;
int n;
{
    register int i;
    for (i = 0; i < n; i++) {
        if (cap(*s1) == cap(*s2)) {
            if (*s1 == '\0')
                return 1;
            s1++;
            s2++;
        } else
            return 0;
    }
    return 1;
}

void initcpar(/*void*/)
{
    register struct cpar *cp = &cpar;
    cp->off0 = 1;
    cp->rdonly = RDONLY;
    cp->setbad = 0;
    cp->off4 = 0;
    cp->off5 = 0;
    cp->fill = FILL_PATTERN;
    cp->off10 = 0;
}

void initbsf(/*void*/)
{
    register int i;
    register struct dkbad *dp = &dkbad;

    dp->bt_csn = 0;
    dp->bt_nbad = 0;
    dp->bt_flag = 0;
    for (i = 0; i < NBAD; i++)
        dp->bt_sec[i] = -1;
}

void listcom(/*void*/)
{
    printf("\nAvailable Commands:\n\t(a)dd bad sectors manually\n\t(b)ad sector scan\n");
    printf("\t(f)ormat single track or complete disk volume\n\t(i)nspect bad sector file\n\t(s)elected sector test\n\t(r)andom sector test (infinite)\n\t(?)list commands\n\t(q)uit command mode\n");
}

void diskcheck()
{
    register char *cp;
    register struct upar *up = &upar;
    char *devname;
    short unit;
    
    printf("\nQU68000 - Disk Check/Formatting Program  V3.2\n\n");

mainloop:
    printf("\nCheck: Supported Devices: \n");
    checkmenu();
    printf("\n\ntype: devname(unit) [-u] [-w] | exit\n: ");
    gets(line);
    initcpar();
        
    for (cp = line; *cp == ' ' || *cp==0x13 || *cp==0x11; cp++); /* skip spaces, XON, XOFF */
    for (devname = cp; *cp && *cp != '('; cp++)
        if (EQ(devname, "exit", 4) != 0) return;
    if (*cp != '(') {
        printf("Bad device\n");
        goto mainloop;
    }

    *cp++ = 0;
    for (i_dev = 0; sa_devsw[i_dev].name; i_dev++) {
        if (strequ(devname, sa_devsw[i_dev].name))
            goto founddev;
    }
    printf("Unknown device\n");
    goto mainloop;

founddev:
    unit = *cp++ - '0';
    if (unit < 0 || unit > 7 || *cp++ != ')') {
        printf("Bad unit specifier\n");
        goto mainloop;
    }
    up->unit = unit;
    
    if (*cp != '\0' && *cp != ' ') {
        printf("Bad option\n");
        goto mainloop;
    }

    while (*cp != '\0' && (*cp == ' ' || *cp == 0x13 || *cp == 0x11)) cp++;

    if (*cp != 0) {
        if (*cp++ == '-') {
            switch (*cp) {
            case 'w':
                cpar.rdonly = RDWRITE;
                /*FALLTHRU*/
            case 'u':
                cpar.setbad = 1;
                if (*++cp == '\0')
                    break;
                /*FALLTHRU*/
            default:
                printf("Unknown option\n");
                goto mainloop;
            }
        } else {
            printf("Bad option\n");
            goto mainloop;
        }
    }

    if (((*sa_devsw[i_dev].open)(i_dev, unit)) == 0)
        goto mainloop;
        
    printf("\nUnit %d is %s (%D sectors a %d bytes)\n",
        up->unit, up->name, up->nsecs, up->secsz);
        
    if (cpar.rdonly == RDONLY) {
        printf("Read-Only-Mode");
        if (!cpar.setbad)
            printf(", preserve bad sector file");
        printf("\n");
    }

    listcom();

    for (;;) {
        printf("\nCommand? ");
        gets(line);
        for (cp = line; *cp==' ' || *cp==0x13 || *cp==0x11; cp++);
        switch (*cp) {
        case 'q':
            goto mainloop;
        case 'b':
            initbsf();
            badsecscan();
            writebsf();
            break;
        case 'd':
            defcpar();
            break;
        case 'f':
            format();
            break;
        case '?':
            listcom();
            break;
        case 's':
            readsecs();
            break;
        case 'r':
            randomio();
            break;
        case 'i':
            creadbsf();
            break;
        case 'a':
            readbadsecs();
            break;
        default:
            printf("Bad command\n");
            break;
        }
    }
}

defcpar(/*void*/)
{
    printf("function not yet implemented\n");
}

int secran(/*void*/)
{
    /* strange random number generator */
    return (secseed = secseed * 0x41c64e6d + 0x3039) & 0x7fffffff;
}

int dateran(/*void*/)
{
    /* strange random number generator */
    return (dateseed = dateseed * 0x41c64e6d + 0x3039);
}

newbadsec(secno, flag)
int secno;
short flag;
{
    register struct dkbad *dp = &dkbad;
    register struct upar *up = &upar;
    register int i;

    printf("\t%D \tis bad", secno);
    switch (up->reason) {
    case UERR_READ:
        printf(" (read error)\n");
        break;
    case UERR_WRITE:
        printf(" (write error)\n");
        break;
    case UERR_WCHECK:
        printf(" (write check error)\n");
        break;
    case UERR_MANUAL:
        printf(" (manually)\n");
        break;
    case UERR_COMPARE:
        printf(" (compare error)\n");
        break;
    }

    if (dp->bt_nbad >= NBAD) {
        printf("\ntoo many bad sectors\n");
        return;
    }

    for (i = 0; i < dp->bt_nbad; i++) {
        if (dp->bt_sec[i] == secno)
            return;                     /* already in table */
    }
    dp->bt_sec[dp->bt_nbad] = secno;
    dp->bt_nbad++;
    if (flag == 1)
        markbad(secno);
}

readbadsecs(/*void*/)
{
    int sec;
    
    if (creadbsf() == 0) return;

    printf("enter sector numbers (-1 to end)\n");
    do {
        printf(": ");
        gets(line);
        sec = atol(line);
        if (sec >= 0 && sec < upar.nsecs) {
            upar.reason = 4;
            newbadsec(sec, 1);
        }
    } while (sec != -1);
    writebsf();
}

writebsf(/*void*/)
{
    register struct dkbad *dp = &dkbad;
    register struct cpar *cp = &cpar;
    register int i;
    int sec;
    short errcnt;
    
    if (cp->setbad == 0)
        return;

    errcnt = 0;
    for (i=0; i < 5; i++) {             /* see dkbad.h */
        sec = upar.nsecs - upar.sectrk + (i<<2);    /* last track, even secs 0,2,4,6,8 (phys secsz=256) */
        if ((*sa_devsw[i_dev].doio)(sec, &dkbad, BSFSZ, DEV_WRITE) == 0)
            errcnt = 1;
    }
    
    if (errcnt)
        printf("\nerror in writing bad sector file\n");
    else
        printf("\nbad sector file written\n%d bad sectors known\n", dp->bt_nbad);
}

badsecscan(/*void*/)
{
    register struct upar *up = &upar;
    register struct cpar *cp = &cpar;
    register int i, k;
    unsigned nbytes = up->sectrk * up->secsz;

    for (k = 0; k < (nbytes>>2); k++)
        wbuffer[k] = FILL_PATTERN;

    printf("testing ...\n");
    if (cp->rdonly == RDWRITE) {
        /* read/write mode */
        for (i = 0; i < up->nsecs; i += up->sectrk) {
            if ((*sa_devsw[i_dev].doio)(i, wbuffer, nbytes, DEV_WRITE) != 0) 
                continue;
            for (k = 0; k < up->sectrk; k++) {
                if ((*sa_devsw[i_dev].doio)(k + i, wbuffer, up->secsz, DEV_WRITE) == 0) {
                    up->reason = UERR_WRITE;
                    newbadsec(k + i, 1);
                }
            }
        }

        for (i = up->nsecs; i > 0; i -= up->sectrk) {
            if ((*sa_devsw[i_dev].doio)(i - up->sectrk, rbuffer, nbytes, DEV_READ) != 0)
                continue;
            for (k = 0; k < up->sectrk; k++) {
                if ((sa_devsw[i_dev].doio)(k + i - up->sectrk, rbuffer, up->secsz, DEV_READ) == 0) {
                    up->reason = UERR_READ;
                    newbadsec(k + i - up->sectrk, 1);
                }
            }
        }
        return;
    }
    
    for (i = 0; i < up->nsecs; i += up->sectrk) {
        if ((*sa_devsw[i_dev].doio)(i, rbuffer, nbytes, DEV_READ) != 0)
            continue;
        for (k = 0; k < up->sectrk; k++) {
            if ((*sa_devsw[i_dev].doio)(k + i, rbuffer, up->secsz, DEV_READ) == 0) {
                up->reason = UERR_READ;
                newbadsec(k + i, 1);
            }
        }
    }
}

readsecs(/*void*/)
{
    register struct upar *up = &upar;
    int nsecs, startsec;

    do {
        printf("Starting sector   : ");
        gets(line);
        startsec = atol(line);

        printf("Number of sectors : ");
        gets(line);
        nsecs = atol(line);
    } while ((startsec + nsecs - 1) > upar.nsecs);

    if (creadbsf() == 0) return;
    
    printf("testing ...\n");
    while (nsecs-- != 0) {
        if (rwcblk(startsec, up->secsz) == 0)
            newbadsec(startsec, UERR_READ);
        startsec++;
        if (nsecs == 0) break;
        --nsecs;
        if (rwcblk(startsec+nsecs, up->secsz) == 0)
            newbadsec(startsec+nsecs, UERR_READ);
    }
    
    writebsf();
}

randomio(/*void*/)
{
    register struct upar *up = &upar;
    int sec;
    int dummy;

    initbsf();
    printf("testing ...\n");
    for (;;) {
        sec = secran() % up->nsecs;
        if (rwcblk(sec, up->secsz) == 0)
            newbadsec(sec, UERR_WRITE);
    }
    
    /*NOTREACHED */
}

int rwcblk(sec, sz)
unsigned sec, sz;
{
    register struct upar *up = &upar;
    register struct cpar *cp = &cpar;
    register int i;
    int dummy;
    
    if (cp->rdonly == RDONLY) {
        if ((*sa_devsw[i_dev].doio)(sec, rbuffer, sz, DEV_READ) == 0) {
            up->reason = UERR_READ;
            return 0;
        }
        return 1;
    }

    for (i = 0; i < (sz >> 2); i++)
        wbuffer[i] = dateran();
    if ((*sa_devsw[i_dev].doio)(sec, wbuffer, sz, DEV_WRITE) == 0) {
        up->reason = UERR_WRITE;
        return 0;
    }
    
    if ((*sa_devsw[i_dev].doio)(sec, rbuffer, sz, DEV_READ) == 0) {
        up->reason = UERR_READ;
        return 0;
    }
    
    for (i = 0; i < (sz >> 2); i++) {
        if (rbuffer[i] != wbuffer[i]) {
            up->reason = UERR_COMPARE;
            return 0;
        }
    }
    return 1;
}

int creadbsf(/*void*/)
{
    register struct dkbad *dp = &dkbad;
    register struct upar *up = &upar;
    register int i;
    int sec;
    int okflg = 0;
    
    printf("\nread bad sector file\n");
    
    for (i = 0; i < 5; i++) {
        sec = up->nsecs - up->sectrk + (i<<2);
        if ((*sa_devsw[i_dev].doio)(sec, dp, BSFSZ, DEV_READ) != 0) {
            okflg = 1;
            break;
        }
    }
    if (okflg) {
        if (dp->bt_nbad >= 0 && dp->bt_nbad <= NBAD) {
            for (i = 0; i < dp->bt_nbad; i++)
                printf("\t%D \tis bad\n", dp->bt_sec[i]);
            printf("%d bad sectors known\n", dp->bt_nbad);
            return 1;
        }
      lbl_d3c:
        printf("\nnot a valid bad sector file\n");
        return 0;
    }
    
    printf("\nerror in reading bad sector file\n");
    return 0;
}

markbad(sec)
{
    register struct upar *up = &upar;
    register struct cpar *cp = &cpar;
    register int i;
    int track;                          /* track # where bad sec is */
    int defsec;                         /* relative# of bad sec within track */

    if (sa_devsw[i_dev].trkformat == 0 || !cp->setbad)
        return;

    track = sec / up->sectrk;

    defsec = up->sectrk * track;
    for (i = 0; i < up->sectrk; ++i, ++defsec) {
        if (isbad(&dkbad, defsec) >= 0) {
            badsec[i] = 1;
            continue;
        }
        if ((*sa_devsw[i_dev].doio)(defsec, &rbuffer[(i*up->secsz)>>2], up->secsz, DEV_READ) == 0) {
            up->reason = UERR_READ;
            newbadsec(defsec, 2);   /* don't recurse marking bad */
            badsec[i] = 1;
        } else
            badsec[i] = 0;
    }

    defsec = up->sectrk * track;
    (*sa_devsw[i_dev].trkformat)(track, badsec);
    if (cp->rdonly != RDONLY) return;

    defsec = up->sectrk * track;
    for (i = 0; i < up->sectrk; ++i, ++defsec) {
        if (badsec[i]==0 && (*sa_devsw[i_dev].doio)(defsec, &rbuffer[(i*up->secsz)>>2], up->secsz, DEV_WRITE) == 0) {
            up->reason = UERR_WRITE;
            newbadsec(defsec, 1);
            return;
        }
    }
}

format(/*void*/)
{
    register struct upar *up = &upar;
    register struct cpar *cp = &cpar;
    register int i, k;
    char fmtflag;
    short cyl, trk;
    
    if (sa_devsw[i_dev].trkformat == 0 && sa_devsw[i_dev].format == 0) {
        printf("\nFormatting not standard for %s\nPlease use another formatting program\n",
            up->name);
        return;
    }

    if (cp->rdonly == RDONLY) {
        printf("\nCannot format: Device not opened for writing\n");
        return;
    }

    printf("Complete disk formatting? ");
    gets(line);
    if (line[0] == 'y') {
        printf("Are you sure? ");
        gets(line);
        if (line[0] == 'n')
            fmtflag = 1;
        else
            fmtflag = 0;
    } else
        fmtflag = 1;

    if (fmtflag && sa_devsw[i_dev].trkformat==0) {
        printf("\nSingle track formatting on %s not possible\n", up->name);
        return;
    }       

    for (i=0; i < up->sectrk; i++)
        badsec[i] = 0;

    if (fmtflag) {
        printf("Single track formatting\n");
        
        for (;;) {
            do {
                printf("Cylinder (0-%d or -1 to end): ", up->ncyl - 1);
                gets(line);
                cyl = atol(line);
                if (cyl == -1) return;
            } while (cyl < 0 || cyl >= up->ncyl);

            do {
                printf("Track (0-%d): ", up->trkcyl - 1);
                gets(line);
                trk = atol(line);
            } while (trk < 0 || trk >= up->trkcyl);

            printf("formatting track ...\n");
            (*sa_devsw[i_dev].trkformat)(cyl * up->trkcyl + trk, badsec);
        }
    } else {
        printf("formatting disk ...\n");

        if (sa_devsw[i_dev].format) {
            (*sa_devsw[i_dev].format)();
            return;
        } else {
            for (i = 0; i < up->ncyl; i++) {
                for (k = 0; k < up->trkcyl; k++) {
                    (*sa_devsw[i_dev].trkformat)(i * up->trkcyl + k, badsec);
                }
            }
        }
    }
}

static int strequ(s1, s2)
register char *s1;
register char *s2;
{
    while (cap(*s1) == cap(*s2)) {
        if (*s1++ == '\0')
            return 1;
        s2++;
    }
    return 0;
}
