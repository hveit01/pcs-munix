/*PCS specific - this is a modified version of mkfs.c: */
/*  Copyright (c) 1984 AT&T */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#if FsTYPE == 3
#undef FsTYPE       /* FsTYPE == 3 is certain death when using prototypes */
#endif
#ifndef FsTYPE
#define FsTYPE  2
#endif

#include <sys/param.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/ino.h>
#include <sys/inode.h>
#include <sys/filsys.h>
#include <sys/fblk.h>
#include <sys/dir.h>
#include <sys/stat.h>

#define FSBSIZE BSIZE
/* boot-block size */
#define BBSIZE  512
/* super-block size */
#define SBSIZE  512
/* number of sectors in a block */
#define SECTPB (FSBSIZE/512)

#define NIDIR   (FSBSIZE/sizeof(daddr_t))
#define NFB (NIDIR+500)             /* pcs: was 1300, NFB must be greater than NIDIR+LADDR */
#define NDIRECT (FSBSIZE/sizeof(struct direct))
#define NBINODE (FSBSIZE/sizeof(struct dinode))
#define LADDR   10
#define STEPSIZE    7
#define CYLSIZE     400
#define MAXFN   1000    /*pcs: was 1500 */


static char *_Version = "@(#) RELEASE:  4.0  Sep 09 1986 /usr/sys/startup/mkfs.c ";


extern char tbf[];

static char *buf = (char*)&tbf[0];
static char *work0 = (char*)&tbf[FSBSIZE];
static char *work1 = (char*)&tbf[FSBSIZE*2];

static struct fblk *fbuf = (struct fblk*)&tbf[FSBSIZE];
static struct filsys *filsys= (struct filsys*)&tbf[FSBSIZE*2+sizeof(struct filsys)];    /* 2nd half of work1 */

static int f_n = CYLSIZE;   /* filsys->s_dinfo[0] */
static int f_m = STEPSIZE;  /* filsys->s_dinfo[1] */

static int utime;
static int fin;
static int fsi;
static int fso;
static char *charp;
static char string[52];

#undef fsys         /*pcs*/
static int fsys;
static char *proto;
static int error;
static ino_t ino;

long getnum();
daddr_t alloc();

mkfs()
{
    int f, c;
    long n, nb;
    static struct stat statarea;        /*pcs unused*/
    static struct {                     /*pcs unused */
        daddr_t tfree;
        ino_t tinode;
        char fname[6];
        char fpack[6];
    } ustatarea;

    static char line[60], fs[100];

    printf("file system size (counted in 512 Byte Blocks) : ");
    gets(line);
    proto = line;

    do {    
        printf("file system: ");
        gets(fs);
        fso = sa_open(fs, 1);
        fsi = sa_open(fs, 0);
    
        sa_read(fsi, work1, FSBSIZE);   /* read boot sector */

        for (c = sizeof(struct filsys); c < FSBSIZE; c++)   /* clear superblock area */
            work1[c] = 0;
    
    } while (fso < 0 || fsi < 0);
    
    fin = 0;                            /* seems to be a relic from original code */
    if (fin == 0) {
        
        /* accept nb:n  nblocks:ninodes*/
        nb = 0;
        n = 0;
        for (f = 0; (c = proto[f]) != 0; f++) {
            if (c < '0' || c > '9') {
                if (c == ':') {         /* both given, save number as #blocks ... */
                    nb = n;
                    n = 0;      /* ... and collect #inodes */
                    continue;
                }
                printf("%s: cannot open\n", proto);
                exit(1);
            }
            n = n*10 + (c-'0');
        }

        if (nb == 0) {                  /* only block size given (is in n) */
            nb = n / SECTPB;            /* make # of 1k blocks */
            n = nb / (NBINODE*4);       /* 1 inode block for each 64 blocks = 16 inodes */
        } else {                        /* blocks:inodes given */
            nb /= SECTPB;               /* make # of 1K blocks */
            n /= NBINODE;               /* # of inode blocks for 16 iondes each */
        }

                /* nb is number of logical blocks in fs,
                   n is number of inodes */

        filsys->s_fsize = nb;
        if (n <= 0)                     /* at least 1(+2) inode blocks = 48 inodes */
            n = 1;
        if (n > 65500/NBINODE)          /* limit # of inode blocks to 4093(+2) = 65520 inodes max */
            n = 65500/NBINODE;
        
        /* set magic number for file system type */
        filsys->s_isize = n + 2;        /* minimum 3 blocks, maximum 4095 blocks */
        filsys->s_magic = FsMAGIC;
        filsys->s_type = Fs2b;
        charp = "d--777 0 0 $ ";
    }
    
    f_m = checkgap(fso) / SECTPB;
    filsys->s_dinfo[0] = f_m;
    filsys->s_dinfo[1] = f_n;
    
    printf("bytes per logical block = %d\n", FSBSIZE);
    printf("total logical blocks = %d\n", filsys->s_fsize);
    printf("total inodes = %d\n", n * NBINODE);
    printf("gap (physical blocks) = %d\n", filsys->s_dinfo[0]);
    printf("cylinder size (physical blocks) = %d \n", filsys->s_dinfo[1]);
    
    if (filsys->s_isize >= filsys->s_fsize) {
        printf("%d/%d: bad ratio\n", filsys->s_fsize, filsys->s_isize - 2);
        return;
    }

    filsys->s_tinode = 0;               /* #of free inodes */
    for (c = 0; c < FSBSIZE; c++)       /* clear buffer */
        buf[c] = 0;

    for (n = 2; n != filsys->s_isize; n++) {    /* write inode blocks */
        wtfs(n, buf);
        filsys->s_tinode += NBINODE;        /* and add to free inode count */
    }
    ino = 0;

    bflist();

    cfile((struct inode*)0);

    filsys->s_time = utime;
    filsys->s_state = FsOKAY - filsys->s_time;

    sa_lseek(fso, 0, 0);                    /* position to the beginning of disk */
    sa_write(fso, (char*)work1, 2*SBSIZE);  /* write boot sector and superblock */
    sa_close(fso);
    sa_close(fsi);
}

cfile(par)
struct inode *par;      /* parent inode if directory */
{
    static struct inode in;
    daddr_t bn, nblk;
    int dbc, ibc;
    static char db[FSBSIZE];
    static int ib[NFB];
    int i, f, c;


    getstr();
    in.i_mode =  gmode(string[0], "-bcd", IFREG, IFBLK, IFCHR, IFDIR, 0, 0, 0);
    in.i_mode |= gmode(string[1], "-u",   0,     ISUID, 0,     0,     0, 0, 0);
    in.i_mode |= gmode(string[2], "-g",   0,     ISGID, 0,     0,     0, 0, 0);
    for (i = 3; i < 6; i++) {
        c = string[i];
        if (c < '0' || c > '7') {
            printf("%c/%s: bad octal mode digit\n", c, string);
            error = 1;
            c = 0;
        }
        in.i_mode |= (c-'0') << (15-3*i);
    }
    in.i_uid = getnum();
    in.i_gid = getnum();

    /*
     * general initialization prior to
     * switching on format
     */
    ino++;
    in.i_number = ino;
    for (i=0; i < FSBSIZE; i++)
        db[i] = 0;
    for (i=0; i < NFB; i++)         /* blocks for indirect and 2nd indirect blocks */
        ib[i] = (daddr_t)0;
    in.i_nlink = 1;
    in.i_size = 0;
    for (i=0; i<NADDR; i++)
        in.i_addr[i] = (daddr_t)0;
    
    if (par == (struct inode*)0) {  /* is this root directory? */
        par = &in;                  /* point to itself to be created */
        in.i_nlink--;
    }
        
    dbc = 0;
    ibc = 0;

    switch (in.i_mode & IFMT) {

    case IFREG:
        /*
         * regular file
         * contents is a file name
         */

        getstr();
        f = sa_open(string, 0);
        if (f < 0) {
            printf("%s: cannot open\n", string);
            error = 1;
            break;
        }
        while ((i=sa_read(f, db, FSBSIZE)) > 0) {
            in.i_size + = i;
            newblk(&dbc, db, &ibc, ib);
        }
        sa_close(f);
        break;

    case IFCHR:
    case IFBLK:
        /*
         * special file
         * content is maj/min types
         */

        i = getnum() & 0377;            /* major */
        f = getnum() & 0377;            /* minor */
        in.i_rdev = (i << 8) | f;       /* makedev(i, k) */
        break;

    case IFDIR:
        /*
         * directory
         * put in extra links
         * call recursively until
         * name of "$" found
         */

        par->i_nlink++;
        in.i_nlink++;
        entry(in.i_number, ".", &dbc, db, &ibc, ib);
        entry(par->i_number, "..", &dbc, db, &ibc, ib);
        in.i_size = 2*sizeof(struct direct);
        for (;;) {
            getstr();
            if (string[0] == '$' && string[1] == '\0')
                break;
            entry(ino+1, string, &dbc, db, &ibc, ib);
            in.i_size += sizeof(struct direct);
            cfile(&in);
        }
        break;
    }
    
    if (dbc != 0)
        newblk(&dbc, db, &ibc, ib);
    iput(&in, &ibc, ib);
}

int gmode(c, s, m0, m1, m2, m3, m4, m5, m6)
char c;
char *s;
{
    int i;

    for (i=0; s[i]; i++)
        if (c == s[i])
            return (&m0)[i];
    printf("%c/%s: bad mode\n", c, string);
    error = 1;
    return 0;
}

static long getnum()
{
    int i, c;
    long n;
    
    getstr();
    n = 0;
    for (i=0; c=string[i]; i++) {
        if (c<'0' || c>'9') {
            printf("%s: bad number\n", string);
            error = 1;
            return (long)0;
        }
        n = n*10 + (c-'0');
    }
    return n;
}

static getstr()
{
    int i, c;

loop:
    switch (c = getch()) {
    case '\t':
    case '\n':
    case ' ':
        goto loop;

    case 0:
        printf("EOF\n");
        return;

    case ':':
        while (getch() != '\n');
        goto loop;
    }
    i = 0;

    do {
        string[i++] = c;
        c = getch();
    } while (c!=' ' && c!='\t' && c!='\n' && c!='\0');
    string[i] = '\0';
}

static rdfs(blk, buf)
{
    int n;
    
    sa_lseek(fsi, blk * FSBSIZE, 0);
    n = sa_read(fsi, buf, FSBSIZE);
    if (n != FSBSIZE) {
        printf("read error: %d\n", blk);
        for(;;);    /* hang forever */
    }
}

static wtfs(bno, bf)
daddr_t bno;
char *bf;
{
    int n;

    sa_lseek(fso, bno*FSBSIZE, 0);
    n = sa_write(fso, bf, FSBSIZE);
    if (n != FSBSIZE) {
        printf("write error: %d\n", bno);
        for(;;);    /* hang forever */
    }
}

static daddr_t alloc()
{
    int i;
    daddr_t bno;

    filsys->s_tfree--;
    bno = filsys->s_free[--filsys->s_nfree];
    if (bno == 0) {
        printf("out of free space\n");
        for(;;);                        /* hang forever */
    }
    if (filsys->s_nfree <= 0) {
        rdfs(bno, (char*)fbuf);
        filsys->s_nfree = fbuf->df_nfree;
        for (i=0; i<NICFREE; i++)
            filsys->s_free[i] = fbuf->df_free[i];
    }
    return bno;
}

static bfree(bno)
daddr_t bno;
{
    int i;
    filsys->s_tfree++;
    if (filsys->s_nfree >= NICFREE) {
        fbuf->df_nfree = filsys->s_nfree;
        for (i=0; i<NICFREE; i++)
            fbuf->df_free[i] = filsys->s_free[i];
        wtfs(bno, (char*)fbuf);
        filsys->s_nfree = 0;
    }
    filsys->s_free[filsys->s_nfree++] = bno;
}

static entry(in, str, adbc, db, aibc, ib)
ino_t in;
char *str;
int *adbc, *aibc;
char *db;
daddr_t *ib;
{
    struct direct *dp;
    int i;

    dp = (struct direct *)db;
    dp += *adbc;
    (*adbc)++;
    dp->d_ino = in;
    for (i = 0; i < DIRSIZ; i++)
        dp->d_name[i] = 0;
    for (i = 0; i < DIRSIZ; i++)
        if ((dp->d_name[i] = str[i]) == 0)
            break;

    if (*adbc >= NDIRECT)
        newblk(adbc, db, aibc, ib);
}

static newblk(adbc, db, aibc, ib)
int *adbc, *aibc;
char *db;
daddr_t *ib;
{
    int i;
    daddr_t bno;
    
    bno = alloc();
    wtfs(bno, db);
    for (i=0; i<FSBSIZE; i++)
        db[i] = 0;
    *adbc = 0;
    ib[*aibc] = bno;
    (*aibc)++;
    if (*aibc >= NFB) {
        printf("file too large\n");
        error = 1;
        *aibc = 0;
    }
}

static getch()
{
    return *charp++;
}

static bflist()
{
    static struct inode in;
    static daddr_t ib[NFB];
    int ibc;
    static char flg[MAXFN];
    static int adr[MAXFN];
    int i,j;
    daddr_t f, d;

    for (i=0; i<f_n; i++)
        flg[i] = 0;

    i = 0;
    for (j=0; j<f_n; j++) {
        while (flg[i])
            i = (i+1)%f_n;
        adr[j] = i+1;
        flg[i]++;
        i = (i+f_m)%f_n;
    }

    ino++;
    in.i_number = ino;
    in.i_mode = IFREG;
    in.i_uid = 0;
    in.i_gid = 0;
    in.i_nlink = 0;
    in.i_size = 0;
    for (i=0; i<NADDR; i++)
        in.i_addr[i] = (daddr_t)0;
    
    for (i=0; i<NFB; i++)
        ib[i] = (daddr_t)0;

    ibc = 0;
    bfree((daddr_t)0);
    filsys->s_tfree = 0;
    
    d = filsys->s_fsize - 1; 
    while (d % f_n) 
        d++;
    for (; d>0; d -= f_n)
        for (i=0; i<f_n; i++) {
            f = d -adr[i];
            if (f < filsys->s_fsize && f >= filsys->s_isize)
                if (badblk(f)) {
                    if (ibc >= NIDIR) {
                        printf("too many bad blocks\n");
                        error = 1;
                        ibc = 0;
                    }
                    ib[ibc] = f;
                    ibc++;
                } else {
                    bfree(f);
                }
        }
    iput(&in, &ibc, ib);
}

static iput(ip, aibc, ib)
register struct inode *ip;
register int *aibc;
daddr_t *ib;
{
    register struct dinode *dp;
    daddr_t d;
    register i, j, k;
    static daddr_t ib2[NIDIR];
    static daddr_t ib3[NIDIR];

    filsys->s_tinode--;
    d = itod(ip->i_number);
    if (d >= filsys->s_isize) {
        if (error == 0)
            printf("ilist too small\n");
        error = 1;
        return;
    }
    rdfs(d, buf);
    dp = (struct dinode*)buf;
    dp += itoo(ip->i_number);

    dp->di_mode = ip->i_mode;
    dp->di_nlink = ip->i_nlink;
    dp->di_uid = ip->i_uid;
    dp->di_gid = ip->i_gid;
    dp->di_size = ip->i_size;
    dp->di_atime = utime;
    dp->di_mtime = utime;
    dp->di_ctime = utime;

    switch(ip->i_mode & IFMT) {

    case IFDIR:
    case IFREG:
        /* handle direct pointers */
        for (i=0; i<*aibc && i < LADDR; i++) {
            ip->i_addr[i] = ib[i];
            ib[i] = 0;
        }
        /* handle single indirect block */
        if (i < *aibc)
        {
            for (j=0; i<*aibc && j <NIDIR; j++, i++)
                ib[j] = ib[i];
            for (; j<NIDIR; j++)
                ib[j] = 0;
            ip->i_addr[LADDR] = alloc();
            wtfs(ip->i_addr[LADDR], (char*)ib);
        }
        /* handle double indirect block */
        if (i < *aibc) 
        {
            for (k=0; k <NIDIR && i <*aibc; k++)
            {
                for (j=0; i<*aibc && j<NIDIR; j++, i++)
                    ib[j] = ib[i];
                for (; j<NIDIR; j++)
                    ib[j] = 0;
                ib2[k] = alloc();
                wtfs(ib2[k], (char*)ib);
            }
            for (; k<NIDIR; k++)
                ib2[k] = 0;
            ip->i_addr[LADDR+1] = alloc();
            wtfs(ip->i_addr[LADDR+1], (char*)ib2);
        }
        /* handle triple indirect block */
        if(i < *aibc)
        {
            printf("triple indirect blocks not handled\n");
        }
        break;

    case IFBLK:
        break;

    case IFCHR:
        break;

    default:
        printf("bad mode %o\n", ip->i_mode);
        for (;;);       /* hang forever */
    }

    ltol3(dp->di_addr, ip->i_addr, NADDR);
    wtfs(d, buf);
}

static int badblk()
{
    return 0;
}
