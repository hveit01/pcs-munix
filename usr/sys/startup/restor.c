/*PCS specific*/

/* enforce 1024 byte FS */
#define FsTYPE 2

#include <sys/types.h>
#include <sys/param.h>
/*#include <sys/systm.h>*/
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/dir.h>
#include <sys/ino.h>
#include <sys/filsys.h>
#include <sys/fblk.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/signal.h>
#include <sys/inode.h>
#include <sys/user.h>
#include "dumprestor.h"

static char *_Version = "@(#) RELEASE:  2.0  Sep 09 1986 /usr/sys/startup/restor.c ";

/*static struct spcl spcl;*/
static uint ntrec = 60;
static uint bct = 61;
static char tapename[] = "/dev/rmt0";
static char *magtape = tapename;
static int volno = 1;
static struct {
    char bootblk[SUPERBOFF];    /* boot sector */
    struct filsys filsys;       /* superblock */
} sblocks;
#define SUPER sblocks.filsys

static int fi;
static ino_t ino;
static ino_t maxi;
static ino_t curino;
static int mt;
static char mbuf[52];
static int eflag;
static struct dinode tino;
static struct dinode dino;
static daddr_t taddr[NADDR];
static daddr_t curbno;
static short dumpmap[MSIZ];
static short clrimap[MSIZ];
static struct spcl addrblock;
static char buf[BSIZE];
static char tmpbuf[BSIZE+0x58]; /* effectivedly only BSIZE, but 88bytes unused slack from elsewhere */

static void rstrfile(), rstrskip();

extern char tbf[][BSIZE];       /* tape buffer of ntrec blocks */
extern char gbuf[BSIZE];

restor()
{
    int dummy;                  /* unused */
    
    volno = 1;
    ntrec = 60; /*blocksize = 60*512*/
    bct = 61;
    magtape = "st(0,1)";
    
    doit('r', 1, 0);
}

doit(cmd, arg4, arg8)           /* arg4, arg8 unused */
char cmd;
{
    int var32;
    struct dinode *tip;
    struct dinode *dip;
    char line[50];
    register int i;
    
    do {
        printf("Tape? ");
        gets(mbuf);
        if ((mbuf[0] == 's' && mbuf[1] == 't') ||
            (mbuf[0] == 'i' && mbuf[1] == 's') ||
            (mbuf[0] == 'w' && mbuf[1] == 't'))
            ntrec = 60;
        else
            ntrec = NTREC;

        mt = sa_open(mbuf, 0);
        if (mt == -1)
            printf("cannot open %s, error = %d\n", mbuf, u.u_error);
    } while (mt == -1);

    magtape = mbuf;
    
    switch(cmd) {
    case 'r':
    case 'R':
        do {
            printf("Disk? ");
            gets(line);
            fi = sa_open(line, 2);
        } while (fi == -1);

        volno = 1;

        printf("Last chance before scribbling on %s. ", "disk");
        while (getchar() != '\n');

        dread(1, (char*)&SUPER);    /* read superblock */
        if (SUPER.s_magic != FsMAGIC || SUPER.s_type != Fs2b) {
            printf("restor for 1k blocks only\n");
            sa_close(fi);
            sa_close(mt);
            return;
        }

        maxi = (SUPER.s_isize - 2) * INOPB;
        if (readhdr(&spcl) == 0) {
            printf("Missing volume record\n");
            sa_close(fi);
            sa_close(mt);
            return;
        }
        if (checkvol(&spcl, volno) == 0) {
            printf("Tape is not volume %d\n", volno);
            return;
        }
        gethead(&spcl);
loop:
        if (ishead(&spcl) == 0) {
            printf("Missing header block\n");
            while (gethead(&spcl) == 0);
            eflag++;
        }
        if (checktype(&spcl, TS_END) == 1) {
            printf("End of tape\n");
            dwrite(1, (char*)&SUPER);
            sa_close(fi);
            sa_close(mt);
            return;
        }
        if (checktype(&spcl, TS_CLRI) == 1) {
            readbits(clrimap);
            for (ino = 1; ino <= maxi; ino++) {
                if ((clrimap[(ino-1) / INOPB] & (1 << ((ino-1) % INOPB)))) continue;
                getdino(ino, &tino);
                if (tino.di_mode) {
                    itrunc(&tino);
                    clri(&tino);
                    putdino(ino, &tino);
                }
            }
            dwrite(1, (char*)&SUPER);
            goto loop;
        }
        if (checktype(&spcl, TS_BITS) == 1) {
            readbits(dumpmap);
            goto loop;
        }
        if (checktype(&spcl, TS_INODE) == 0) {
            printf("Unknown header type\n");
            eflag++;
            gethead(&spcl);
            goto loop;
        }
        ino = spcl.c_inumber;
        if (eflag)
            printf("Resynced at inode %u\n", ino);
        eflag = 0;
        if (ino > maxi) {
            printf("%u: ilist too small\n", ino);
            gethead(&spcl);
            goto loop;
        }
        dino = spcl.c_dinode;
        getdino(ino, &tino);
        curbno = 0;
        itrunc(&tino);
        clri(&tino);
        for (i=0; i<NADDR; i++)
            taddr[i] = 0;
        l3tol(taddr, dino.di_addr, 3);
        getfile(ino, rstrfile, rstrskip, dino.di_size);
        tip = &tino;
        ltol3(tip->di_addr, taddr, NADDR);
        dip = &dino;
        tip->di_mode = dip->di_mode;
        tip->di_nlink = dip->di_nlink;
        tip->di_uid = dip->di_uid;
        tip->di_gid = dip->di_gid;
        tip->di_size = dip->di_size;
        tip->di_atime = dip->di_atime;
        tip->di_mtime = dip->di_mtime;
        tip->di_ctime = dip->di_ctime;
        putdino(ino, &tino);
        goto loop;
    }
}

getfile(inum, file, skip, size)
ino_t inum;
void (*file)(), (*skip)();
off_t size;
{
    register int i;

    addrblock = spcl;
    curino = inum;
    goto doaddr;

seek:
    if (gethead(&addrblock) == 0) {
        printf("Missing address (header) block\n");
        goto done;
    }
    if (checktype(&addrblock, TS_ADDR) == 0) {
        spcl = addrblock;
        curino = 0;
        return;
    }

doaddr:
    for (i=0; i < addrblock.c_count; i++) {
        if (addrblock.c_addr[i]) {
            readtape(buf);
            (*file)(buf, size > BSIZE ? BSIZE : size);
        } else {
            clearbuf(buf);
            (*skip)(buf, size > BSIZE ? BSIZE : size);
        }
        if ((size -= BSIZE) <= 0) {
done:
            while (gethead(&spcl) == 0 || checktype(&spcl, TS_ADDR) == 1);
            curino = 0;
            return;
        }
    }
    goto seek;
}

readtape(p)
struct spcl *p;
{
    register uint i;

    if (bct >= ntrec) {
        for (i=0; i < ntrec; i++) {
            ((struct spcl*)tbf[i])->c_magic = 0;
        }
        bct = 0;
        i = sa_read(mt, tbf, ntrec*BSIZE);
        if (i == -1) {
            printf("Tape read error: inode %u\n", curino);
            eflag++;
            for (i = 0; i < ntrec; i++)
                clearbuf(tbf[i]);
        } else if (i)
            ntrec = i / BSIZE;
        
        if (i && iseof(tbf[0]))
            i = 0;

        if (i == 0) {
nextvol:
            bct = ntrec + 1;
            volno++;
waitvol:
            flsht();
            sa_close(mt);
            printf("Mount volume %d\n", volno);
            while (getchar() != '\n');
            if ((mt = sa_open(magtape, 0)) == -1) {
                printf("Cannot open tape!\n");
                goto waitvol;
            }
            if (readhdr(tmpbuf) == 0) {
                printf("Not a dump tape.Try again\n");
                goto waitvol;
            }
            if (checkvol(tmpbuf, volno) == 0) {
                printf("Wrong tape. Try again\n");
                goto waitvol;
            }
            readtape(p);    /* recurse */
            return;
        }
    }
    if (iseof(tbf[bct])) goto nextvol;

    copy(tbf[bct++], p, BSIZE);
}

static flsht()
{
    bct = ntrec+1;
}

static copy(s1, s2, n)
register char *s1, *s2;
register int n;
{
    do {
        *s2++ = *s1++;
    } while (--n);
}

static clearbuf(bp)
register char *bp;
{
    register int i = BSIZE;
    do {
        *bp++ = 0;
    } while (--i);
}

dwrite(bno, buf)
daddr_t bno;
char *buf;
{
    if (bno==1) {
        if (1) {            /* something set to TRUE */
            bno = 0;
            buf -= SUPERBOFF;
        }
    }
    
    sa_lseek(fi, bno * BSIZE, 0);
    if (sa_write(fi, buf, BSIZE) != BSIZE)
        printf("disk write error %d\n", bno);
}

dread(bno, buf)
daddr_t bno;
char *buf;
{
    register int i, k;
    k = 0;

    if (bno == 1) {
        if (1) {
            bno = 0;
            buf -= 512;
        }
    }

    sa_lseek(fi, bno * BSIZE, 0);
    if (sa_read(fi, buf, BSIZE) != BSIZE) {
        printf("disk read error %d\n", bno);
        for(;;);        /* hang forever */
    }
}

static clri(dip)
struct dinode *dip;
{
    int i = sizeof(struct dinode)/sizeof(int);
    int *p = (int*)dip;

    do {
        *p++ = 0;
    } while (--i);
}

static itrunc(dip)
register struct dinode *dip;
{
    register int i;
    daddr_t bno;
    daddr_t blks[NADDR];
    
    if (dip->di_mode == 0) return;
    SUPER.s_tinode++;
    i = dip->di_mode & IFMT;
    if (i != IFDIR && i != IFREG)
        return;
    
    l3tol(blks, dip->di_addr, NADDR);
    for (i=NADDR-1; i >= 0; i--) {
        bno = blks[i];
        if (bno != 0) {
            switch (i) {
            default:
                bfree(bno);
                break;
            case 10:    /* 1st ind blk */
                tloop(bno, 0, 0);
                break;
            case 11:    /* 2nd ind blk */
                tloop(bno, 1, 0);
                break;
            case 12:    /* 3rd ind blk */
                tloop(bno, 1, 1);
                break;
            }
        }
    }
    dip->di_size = 0;
}

static tloop(bno, lv23, lv3)
daddr_t bno;
int lv23, lv3;
{
    register int i;
    daddr_t blk;
    int idx;
    static daddr_t ib[3][NINDIR];

    idx = lv23 + lv3;
    
    dread(bno, ib[idx]);
    for (i= 255; i >= 0; i--) {
        blk = ib[idx][i];
        if (blk) {
            if (lv23)
                tloop(blk, lv3, 0);
            else
                bfree(blk);
        }
    }
    bfree(bno);
}

static union {
    struct fblk fb;
    char blk[BSIZE];    /* extend to BSIZE */
} fbuf;

static bfree(bno)
daddr_t bno;
{
    register int i;
    if (SUPER.s_nfree >= NICFREE) {
        fbuf.fb.df_nfree = SUPER.s_nfree;
        for (i=0; i<NICFREE; i++)
            fbuf.fb.df_free[i] = SUPER.s_free[i];
        SUPER.s_nfree = 0;
        dwrite(bno, fbuf.blk);
    }
    SUPER.s_tfree++;
    SUPER.s_free[SUPER.s_nfree++] = bno;
}

static char zeroes[BSIZE];

static daddr_t balloc()
{
    register int i;
    daddr_t bno;
    
    if (SUPER.s_nfree==0 || (bno = SUPER.s_free[--SUPER.s_nfree])==0) {
        printf("Out of space\n");
        for(;;);    /* hang forever */
    }
    if (SUPER.s_nfree == 0) {
        dread(bno, &fbuf);
        SUPER.s_nfree = fbuf.fb.df_nfree;
        for (i=0; i < NICFREE; i++)
            SUPER.s_free[i] = fbuf.fb.df_free[i];
    }
    dwrite(bno, zeroes);
    if (SUPER.s_tfree > 0)
        SUPER.s_tfree--;
    return bno;
}

static daddr_t bmap(blks, cbno)
daddr_t *blks;
daddr_t cbno;
{
    register int i;
    daddr_t blk2;
    daddr_t blk1;
    daddr_t blk0;
    daddr_t blk3;
    static int buf[NINDIR];

    if (cbno < 10) {    /* direct block */
        blks[cbno] = blk0 = balloc();
        return blk0;
    }
    
    blk1 = 0;
    blk0 = 1;
    cbno -= 10;
    for (blk2 = 3; blk2 > 0; blk2--) {
        blk1 += NSHIFT;
        blk0 *= NINDIR;
        if (cbno < blk0) break;
        cbno -= blk0;
    }
    if (blk2 == 0)
        return 0;
    if ((blk0 = blks[13 - blk2]) == 0) {
        blks[13-blk2] = blk0 = balloc();
    }
    for (; blk2 <=3; blk2++) {
        dread(blk0, buf);
        blk1 -= NSHIFT;
        i = blk1;
        i = cbno >> i;
        i &= NMASK;
        blk3 = buf[i];
        if (blk3 == 0) {
            blk3 = balloc();
            buf[i] = blk3;
            dwrite(blk0, buf);
        }
        blk0 = blk3;
    }
    return blk0;
}

static gethead(p)
struct spcl *p;
{
    readtape(p);
    if (p->c_magic != MAGIC || checksum(p) == 0)
        return 0;
    return 1;
}

static int ishead(p)
struct spcl *p;
{
    if (p->c_magic != MAGIC || checksum(p) == 0)
        return 0;
    return 1;
}

static int checktype(p, type)
struct spcl *p;
int type;
{
    return p->c_type == type;
}

static int checksum(p)
short *p;
{
    register short sum, k;
    
    k = BSIZE/sizeof(short);
    sum = 0;

    do {
        sum += *p++;
    } while (--k);
    if (sum != CHECKSUM) {
        printf("Checksum error %o\n", sum);
        return 0;
    }
    return 1;
}

static int checkvol(p, volume)
struct spcl *p;
{
    if (p->c_volume == volume)
        return 1;
    return 0;
}

static int readhdr(p)
struct spcl *p;
{
    if (gethead(p) == 0)
        return 0;
    if (checktype(p, TS_TAPE) == 0)
        return 0;
    return 1;
}

static void rstrfile(buf)
char *buf;
{
    int bno = bmap(taddr, curbno);
    dwrite(bno, buf);
    curbno++;
}

static void rstrskip()
{
    curbno++;
}

static getdino(inum, dip)
ino_t inum;
struct dinode *dip;
{
    int bno = (ino-1) / INOPB;
    bno += 2;
    dread(bno, gbuf);
    copy(&gbuf[((inum-1)%INOPB)*sizeof(struct dinode)], dip, sizeof(struct dinode));
}

static putdino(inum, dip)
ino_t inum;
struct dinode *dip;
{
    int bno;
    
    SUPER.s_tinode--;
    
    bno = ((ino-1) / INOPB) + 2;
    dread(bno, gbuf);
    copy(dip, &gbuf[((inum-1)%INOPB)*sizeof(struct dinode)], sizeof(struct dinode));
    dwrite(bno, gbuf);
}

static readbits(map)
char *map;
{
    register int i = spcl.c_count;
    while (i-- != 0) {
        readtape(map);
        map += BSIZE;
    }
    while (gethead(&spcl) == 0);
}

static done()
{
}

static int iseof(bp)
char *bp;
{
    int i;

    for (i=0; i < (BSIZE/2); i++) {
        if (bp[i] != (char)0xcc)
            return 0;
    }
    return 1;
}
