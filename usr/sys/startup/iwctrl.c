/* PCS specific */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/icc/types.h>
#include <sys/icc/unix_icc.h>
#include <sys/icc/iwctrl.h>

static char *_Version = "@(#) RELEASE:  3.0  May 04 1987 /usr/virt/src/startup/iwctrl.c";

int discon = -1;

struct iwgeo {
    short num_cyls;
    short num_alt_cyls;
    short num_heads;
    short num_secs;
    short num_sst;
    short rwc_cyl;
    short num_totalcyl;
} geo[] = {
    { 0x2e6,    0xc,    0xb,    0x11,   0,  0,  0x2f2 },
    { 0x332,    0xc,    0xa,    0x11,   0,  0,  0x33e },
    { 0x386,    0x10,   0x7,    0x11,   0,  0,  0     },
    { 0x386,    0x10,   0xb,    0x11,   0,  0,  0     },
    { 0x386,    0x10,   0xf,    0x11,   0,  0,  0     },
    { 0x3f0,    0x10,   0x8,    0x11,   0,  0,  0x400 }
};

struct {
    char *name;
    char *size;
    ushort block_size;
    struct iwgeo *param;
} conftab[] = {
    { "wd51b",      "71",   512,    &geo[0] },
    { "wd51fu",     "71",   512,    &geo[0] },
    { "wd51to",     "71",   512,    &geo[1] },
    { "wd51mx65",   "50",   512,    &geo[2] },
    { "wd51mx105",  "80",   512,    &geo[3] },
    { "wd51mx140",  "110",  512,    &geo[4] },
    { "wd51mi",     "71",   512,    &geo[5] },
    { "wd52",       "140",  512,    0       },
    { "wd53",       "230",  512,    0       },
    { "wd54",       "270",  512,    0       }
};

static int format;
static int addbad;
static int list;
static int type;
static int rdcap;
static int modesense;
static int readbad;
static int softctrl;

static union iw_iocb iocb;
static byte iocb2[36];
static char diskname[36];
ulong dordcap();

#define BB16(h,l) ((ioret[h] << 8) + ioret[l])
#define BB24(h,m,l) ((((ioret[h] << 8) + ioret[m]) << 8) + ioret[l])
#define BB32(h,m1,m2,l) ((((((ioret[h] << 8) + ioret[m1]) << 8) + ioret[m2]) << 8) + ioret[l])

long bblka[NBLK];
int bblki;
char *udisknam;
char *ctrlrname;
char *errreport;
char *errclear;
char *disknam;
char *typenam;
char *listnam;
char *softlevel;
int diskfd;
int preservebsf;

iwmenu()
{
    printf("\t      (f)   format disk \n\t      (b)   add bad block(s) \n\t      (c)   read capacity \n\t      (m)   read mode sense \n\t      (r)   read bad block list\n\t      (s)   select disk drive type and format disk\n\t      (l)   read media defect list\n\t      (q)   quit command mode\nSelect function : ");
}

iwformat(arg0, type)
int type;
{
    char line[80];
    
    bblki = 0;
    switch (type) {
    case 0:
        udisknam = "IW(4)";
        disknam = "iw(0)";
        break;
    case 1:
        udisknam = "IW(20)";
        disknam = "iw(1)";
        break;
    case 2:
        udisknam = "IW(36)";
        disknam = "iw(2)";
        break;
    case 3:
        udisknam = "IW(52)";
        disknam = "iw(3)";
        break;
    default:
        printf("invalid unit\n");
        return 0;
    }

    if (opendisk() != 0) {
        printf("cannot open disk\n");
        return 0;
    }

    for (;;) {
        iwmenu();
        gets(line);
        switch (line[0]) {
        case 's':
            if (seldisk(line) < 0)
                break;
            printf("Disk select done.\n");
            printf("Format disk now.\n");
            /*FALLTHRU*/
        case 'f':
            bblki = 0;
loop:
            printf("Ignore old bad sector info (normal answer is n) ? (y/n) : ");
            gets(line);
            if (line[0] != 'y' && line[0] != 'n' && line[0] != '\0')
                goto loop;
            if (line[0] != 'y')
                dogetbadlist();
            doformat(line, 0);
            break;
        case 'b':
            bblki = 0;
            printf("enter bad sector numbers; empty line terminates input\n");
            for (;;) {
                printf("sector number (decimal) : ");
                gets(line);
                if (line[0] == '\0') break;
                bblka[bblki++] = atol(line);
                if (bblka[bblki-1] < 0) {
                    printf("Sector number must be positiv. Try again.\n");
                    bblki--;
                }
            }
            doformat(line, 1);
            break;
        case 'l':
            bblki = 0;
            readlist(line);
            doformat(line, 1);
            break;
        case 'r':
            doreadbad();
            break;
        case 'c':
            (void)dordcap(0);
            break;
        case 'm':
            domodesense();
            break;
        case 'q':
            sa_close(diskfd);
            return 0;
        default:
            break;
        }
    }
}

opendisk()
{
    int dummy[8];
    
    diskfd = sa_open(udisknam, 2);
    if (diskfd < 0)
        return 1;
    else
        return 0;
}

seldisk(line)
char *line;
{
    register int i;
    int n = 10;
    struct iwgeo *param;

    for (;;) {
        printf("Enter one of the following disk type names:\n");
        for (i = 0; i < n; i++)
            printf ("%s (%s MB)\n", conftab[i].name, conftab[i].size);

        gets(line);
        if (line[0] == '\0') return;

        for (i = 0; i < n; i++)
            if (strcmp(line, conftab[i].name)==0) goto found;
    }
found:
    param = conftab[i].param;
    iocb.a.block_size = conftab[i].block_size;
    if (param) {
        iocb.a.num_cyls = param->num_cyls;
        iocb.a.num_alt_cyls = param->num_alt_cyls;
        iocb.a.num_heads = (param->num_heads << 4) + (param->num_sst << 1) + 1;
        iocb.a.num_secs = param->num_secs;
        iocb.a.rwc_cyl = param->rwc_cyl;
        iocb.a.wrt_precomp_cyl = param->num_totalcyl; /* actually, this is total# of cyl */
    }
    if (sa_ioctl(diskfd, IW_MODESEL, &iocb) < 0) {
        printf("Could not send configuration info for disk type %s to disk %s\n",
            line, disknam);
        return -1;
    }
    return 0;
}

static readlist(line)
char *line;
{
    register byte *ioret;

    int cyl, head, dbyte, bcnt;
    int defsec;
    int nheads, nsectrk, logcyl, sst, maxcyl;
    int inrep, rep, rawsz, dbits;
    

    if (sa_ioctl(diskfd, IW_MODESENSE, &iocb) < 0) {
        printf("Could not read mode sense from disk %s.\n", disknam);
        printf("You must format the disk at first.\n");
        return;
    }
    
    ioret = (byte*)&iocb;
    if (ioret[2] == 1) { /* no parameter block following */
        printf("Disk %s is not formatted\n", disknam);
        return;
    }
    if (ioret[2] != 0x11) { /* block descriptor length, must be 17 */
        preservebsf++;
        return;
    }

    /* this is mode sense result block for ST506 drives
     * see bitsavers emulex/MD01251001-3_MD01_Medalist_Disk_Controller_Technical_Manual_Jul84.pdf
     * chp 8.3.5.3
     *
     * Note the byte order is reversed, cf struct iw_iocb.b:

     * byte # in data block
     * 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27    
     * is in # actually returned data bb[]
     * 1 0|3 2|5 4|7 6|9 8|11 10|13 12|15 14|17 16|19 18|21 20|23 22|25 24|27 26
     */
    
    nheads = ioret[12] >> 4;
    nsectrk = ioret[15];
    logcyl = BB16(14, 17); /* log # of cyl MSB, LSB */
    sst  = (ioret[12] & 06) >> 1; /* spare sec/trk */
    maxcyl = BB16(16, 19);
    
    printf("enter media defect list; empty line terminates input\n");
loop:
    printf("cylinder (decimal) : ");
    gets(line);
    if (line[0] == '\0') return;
    cyl = Xatoi(line);
    if (cyl <= 0 || cyl >= maxcyl) {
        printf("Cylinder out of range. Try again.\n");
        goto loop;
    }
    if (cyl >= (logcyl-1)) {
        printf("Cylinder is on the alternate track area.  Sectors on\n");
        printf("this area cannot be marked as bad. Try again.\n");
        goto loop;
    }
    for (;;) {
        printf("head     (decimal) : ");
        gets(line);
        if (line[0] == '\0') return;
        head = Xatoi(line);
        if (head >= 0 && head < nheads) break;
        printf("Head out of range. Try again\n");
    }
    if (sst) {
        for(;;) {
            printf("byte     (decimal) : ");
            gets(line);
            if (line[0] == '\0') return;
            dbyte = Xatoi(line);
            if (dbyte >= 0) break;
            printf("Byte must be positiv. Try again\n");
        }
        for (;;) {
            printf("bitcount (decimal) : ");
            gets(line);
            if (line[0] == '\0') return;
            bcnt = Xatoi(line);
            if (bcnt >= 0) break;
            printf("Bitcount must be positiv. Try again\n");
        }
    }
    printf("\n");
    defsec = ((cyl-1)*nheads + head)*nsectrk;   /* defective sector */
    if (sst) {
        rep = (nsectrk + sst)==18 ? 0x33 : 0x42;
        rawsz = rep + 0x209;    /* raw sector size 572 or 587 */
        inrep = dbyte / rawsz;  /* defective byte in CRC area? */
        dbyte %= rawsz;         /* offset */
        dbits = ((bcnt+6) / 8) + dbyte;
        if (bcnt < 5 && dbyte >= rep && (rep+512) > dbits) /* ignore if handled by ECC */
            goto loop;
    }
    if (bblki < NBLK)
        bblka[bblki++] = (sst ? inrep : 0) + defsec;

    if (sst && dbits >= rawsz && bblki < NBLK)
        bblka[bblki++] = defsec + inrep + 1;
        if (bblki >= NBLK) {
        printf("Too many bad blocks!\n");
        return;
    }
    goto loop;
}

doformat(line, flag)
char *line;
int flag;
{
    register int i;
    char *msg = flag ? "add bad blocks to" : "format";
    
    iocb.a.flag = preservebsf;
    iocb.a.num_defects = bblki;
    if (bblki) {
        for (i=0; i < bblki; i++)
            iocb.a.d.l[i].l = bblka[i];
    }
    
    printf("Do you really want to %s disk %s (y/n)? ", msg, disknam);
    gets(line);
    if (line[0] != 'y') return;

    if (sa_ioctl(diskfd, flag ? IW_REASSIGN : IW_FORMAT, &iocb) < 0)
        printf("Could not %s disk %s\n", msg, disknam);
    else
        printf("\nDone\n");
}

doreadbad()
{
    static char *badtypes[] = { "Manufacturers", "Grown" };
    
    register byte *ioret;
    register int i, bbcnt;
    int not506;
    int bbfmt;
    int cyl45, head45, bidx5, bidx4;
    int block, altblk, nheads, nsectrk, cyl, secincyl;
    int k, sst;

    if (sa_ioctl(diskfd, IW_MODESENSE, &iocb) < 0) {
        printf("Could not read mode sense from disk %s\n", disknam);
        return;
    }
    ioret = (byte*)&iocb;
    if (ioret[2] == 1) { /* no parameter block following */
        printf("Disk %s is not formatted\n", disknam);
        return;
    }
    
    not506 = ioret[2] != 0x11;  /* returned not standard st506 format */
    if (not506)
        format_transform(ioret);

    nheads = ioret[12] >> 4;
    nsectrk = ioret[15];
    sst = (ioret[12] & 06) >> 1;
    iocb.a.num_defects = NBLK;
    
    ioret = (byte*)&iocb;
    if (sa_ioctl(diskfd, IW_READBBL, &iocb) < 0) {
        printf("Could not read bad sector file from disk %s\n", disknam);
        return;
    }
    
    bbfmt = ioret[0] & 7; /* format of bb table */
    if (!not506) {
        printf("Length of Bad Sector File %d\nNumber of Remaining Alternate Tracks %d\nNext Alternate Track Logical Block Address %ld\nNumber of Bad Sector File Entries %d\n",
            ioret[1],   /* length of file */
            BB16(0,3), /* alttrks */
            BB32(2,5,4,7), /*next alttrk LBA */
            bbcnt = BB16(6,9)); /*# of entries */
        if (bbcnt == 0) return; /* has no bad tracks */

        printf("List of bad blocks     ->    alternate blocks:\n");
        printf("  block    cyl   head  ->    block    cyl   head\n");
        
        ioret = &ioret[8];  /* location of bad block list */
        for (i = 0; i < bbcnt; i++) {
            Xprintf(7, "%ld", block = BB32(0,3,2,5));
            Xprintf(4, "   %ld", cyl = (block / (nheads * nsectrk)) + 1);
            secincyl = block - (cyl-1) * nheads * nsectrk;
            Xprintf(2, "   %ld", secincyl / nsectrk);
            printf("    ->  ");
            Xprintf(7, "%ld", 
                altblk = (((((ioret[4] & 0x7f00) + ioret[7]) << 8) + ioret[6]) << 8) + ioret[9]);
            Xprintf(4, "   %ld", cyl = (altblk / (nheads * nsectrk)) + 1);
            secincyl = altblk - (cyl-1) * nheads * nsectrk;
            Xprintf(2, "   %ld", secincyl / nsectrk);
            printf("    on %s\n", (ioret[4] & 0x80) ? "alternate track" : "spare block");
            ioret = &ioret[8];
        }
        printf("\n");
        return;
    } else {
        for (k = 0; k < 2; k++) {
            if (bbfmt <= 3)
                bbcnt = BB16(3,2) / 4;
            else
                bbcnt = BB16(3,2) / 8;
            printf("%s defect list (%d entries):\n", badtypes[k], bbcnt);
            switch (bbfmt) {
            case 0:
            case 1:
            case 2:
            case 3:
                printf("blocknumber\n");
                break;
            case 4:
                printf("cylinder  head   bytes from index\n");
                break;
            case 5:
                printf("cylinder  head   physical sector\n");
                break;
            default:
                printf("unknown defect list type\n");
                break;
            }

            ioret = &ioret[4];
            for (i=0; i < bbcnt; i++) {
                switch (bbfmt) {
                case 0:
                case 1:
                case 2:
                case 3:
                    Xprintf(7, "   %ld\n", BB32(1,0,3,2));
                    ioret = &ioret[4];
                    break;
                case 4:
                    cyl45 = BB24(1,0,3);
                    head45 = ioret[2];
                    bidx4 = BB32(5,4,7,6);
                    Xprintf(4, "  %ld    ", cyl45);
                    Xprintf(3, "%ld        ", head45);
                    Xprintf(6, "%ld\n", bidx4);
                    ioret = &ioret[8];
                    break;
                case 5:
                    cyl45 = BB24(1,0,3);
                    head45 = ioret[2];
                    bidx5 = BB32(5,4,7,6);
                    Xprintf(4, "  %ld    ", cyl45);
                    Xprintf(3, "%ld           ", head45);
                    Xprintf(3, "%ld\n", bidx5);
                    ioret = &ioret[8];
                    break;
                case 6:
                case 7:
                default:
                    printf("WARNING: wrong defect list format.\n");
                    break;
                }
            }
            printf("\n");
        }
    }
}

dogetbadlist()
{
    register byte *ioret;
    register int i;
    int bbfmt;
    int alttrk;
    int bbcnt;
    int unused1, unused2;
    ushort sst;
    short nsectrk;

    if (sa_ioctl(diskfd, IW_MODESENSE, &iocb) < 0) {
        printf("Could not read mode sense from disk %s\n", disknam);
        printf("You must format the disk at first.\n");
        return;
    }
    
    ioret = (byte*)&iocb;
    if (ioret[2] == 1) {
        printf("Disk %s is not formatted\n", disknam);
        return;
    }
    if (ioret[2] != 0x11) {
        preservebsf++;
        return;
    }
    
    
    nsectrk = ioret[15];
    sst = (ioret[12] & 06) >> 1;
    bblki = 0;
    iocb.a.num_defects = NBLK;
    ioret = (byte*)&iocb;
    if (sa_ioctl(diskfd, IW_READBBL, &iocb) < 0) {
        printf("Could not read bad sector file header from disk %s\n", disknam);
        return;
    }
    bbfmt = ioret[1];
    alttrk = BB16(0,3);
    unused1 = BB32(2,5,4,7);
    bbcnt = BB16(6,9);
    if (bbcnt == 0) return;

    ioret = &ioret[8];
    for (i = 0; i < bbcnt; i++) {
        bblka[bblki++] = BB32(0,3,2,5);
        if (sst && (ioret[4] & 0x80)) {
            if ((bblka[bblki-1] % nsectrk) == (nsectrk-1))
                bblka[bblki++] = bblka[bblki-1]-1;
            else
                bblka[bblki++] = bblka[bblki-1]+1;
        }
        ioret = &ioret[8];
    }
}

/* flag =0: print, =1 cap in blocks, =2 cap in bytes */
ulong dordcap(flag)
int flag;
{
    register byte *ioret;

    if (sa_ioctl(diskfd, IW_RDCAPACITY, flag ? iocb2 : (byte*)&iocb) < 0) {
        printf("Could not read capacity from disk %s\n", disknam);
        return;
    }

    ioret = flag ? iocb2 : (byte*)&iocb;
    if (flag == 0) {
        printf("Capacity is %ld blocks, Block Size is %ld bytes\n\n",
            (BB32(1,0,3,2) + 1) & ~7, BB32(5,4,7,6));
        /* return value unused */
        return;
    }
    return flag==1 ? (BB32(1,0,3,2) + 1) & ~7 : BB32(5,4,7,6);
}

domodesense()
{
    register byte *ioret = iocb2;
    register char *dp = diskname;
    int ctlr;
    int k;
    uint cap;
    

    cap = 0;
    if (sa_ioctl(diskfd, IW_CTRLRNAME, iocb2) < 0) {
        printf("could not get controller name\n");
        return;
    }
    for (k=0; k < 28; k++)
        *dp++ = *((char*)ioret)++;

    if (sa_ioctl(diskfd, IW_MODESENSE, &iocb) < 0) {
        printf("Could not read mode sense from disk %s\n", disknam);
        return;
    }
    ioret = (byte*)&iocb;
    if (ioret[2] == 1) {
        printf("Disk %s is not formatted\n", disknam);
        return;
    }
    ctlr = ioret[2] != 0x11;
    if (ctlr == 0) {
report:
        if (strcmp(diskname, "EMULEX MD01")==0)
            printf("\nwd51xx disk drive.\n");
        else
            printf("\nVendor Name of Disk Type: %s\n", diskname);
        printf("Disk %s is %s write protected\n",
            disknam, (ioret[3] & 0x80) ? "" : "not");
        printf("Number of Blocks %ld\n", cap ? cap : (uint)(BB24(4,7,6)+1 & ~7));
        printf("Block Length %ld\n", BB24(8,11,10));
        printf("Number of Alternate Cylinders %d\n", ioret[13]);
        printf("Number of Heads %d\n", ioret[12] >> 4);
        printf("Spare Sectors per Track %d\n", (ioret[12] & 06)>>1);
        printf("Step is %s buffered\n", (ioret[12] & 1) ? "" : "not");
        printf("Logical Number of Sectors/Track %d\n", ioret[15]);
        printf("Logical Number of Cylinders %d\n", BB16(14, 17)-2);
        printf("Write Precompensation Cylinder Number %d\n", BB16(16,19));
        printf("Reduced Write Current Cylinder Number %d\n", BB16(18,21));
        printf("\n");
    } else {
        format_transform(ioret);
        if (((BB24(4,7,6)+1) & ~7)==0)
            cap = dordcap(1);
        goto report;
    }
}

static Xprintf(sz, fmt, arg)
short sz;
char* fmt;
int arg;
{
    int argsave = arg;
    while (sz && arg) {
        if ((arg /= 10) != 0) sz--;
    }
    sz--;
    while (sz-- != 0)
        printf(" ");
    printf(fmt, argsave);
}

static Xatoi(line)
char *line;
{
    int sign = 0;
    int val;

    if (*line == '-') {
        sign = 1;
        line++;
    }
    val = atoi(line);
    return sign ? -val : val;
}

int pages[11];

format_transform(ioret)
byte *ioret;
{
    register int n;
    int blocks, bytes;
    
    format_scan(ioret);
    if (pages[10] == 0) {
        blocks = dordcap(1);
        bytes = dordcap(2);
        ioret[4] = blocks >> 16;
        ioret[7] = blocks >> 8;
        ioret[6] = blocks;
        ioret[11] = bytes >> 8;
        ioret[10] = bytes;
    }
    
    if (pages[3])
        ioret[15] = (ioret+pages[3])[10] - (ioret+pages[3])[4];
    
    if (pages[3] && pages[4]) {
        ioret[13] = (ioret+pages[3])[8] / (ioret+pages[4])[4];
        ioret[12] = ((ioret+pages[4])[4] << 4) + ((ioret+pages[3])[4] << 1) + 1;
    }
    
    if (pages[4]) {
        n = ((ioret+pages[4])[2] << 8) + (ioret+pages[4])[5] - ioret[13];
        ioret[14] = n >> 8;
        ioret[17] = n;
    }
    ioret[19] = 0;
    ioret[16] = 0;
    ioret[21] = 0;
    ioret[18] = 0;
}

format_scan(ioret)
byte *ioret;
{
    int i, k, sz, m;
    
    sz = 4;
    for (m=0; m < 11; m++)
        pages[m] = 0;

    if (ioret[2]) pages[10] = 4;
    sz += ioret[2];
    do {
        i = ioret[sz+1] & 0x3f;
        k = ioret[sz];
        if (i==0) break;
        if (i > 4) break;
        pages[i] = sz;
        sz += k + 2;
        if (sz >= MODESENSELENGTH) break;
    } while (!FALSE);
}
