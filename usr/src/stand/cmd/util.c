#include "data.h"

extern int physbase;

/* poor man's printf */
printf(fmt, arg)
register char *fmt;
int arg;
{
    register int* argv = &arg;
    register int ch;
    char *str;

    for (;;) {
        while ((ch = *fmt++) != '%') {
            if (ch=='\0') return;
            putchar(ch);
        }
        fmt_width = 0;

next:
        if ((ch= *fmt++) >= 'A' && ch <= 'Z')
            ch += 0x20;
        switch (ch) {
        case 'h': case 'l':
            goto next;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            fmt_width = fmt_width*10 + ch - '0';
            goto next;
        case 'd':
            printnum(*argv, 10);
            break;
        case 'o':
            printnum(*argv, 8);
            break;
        case 'x':
            printnum(*argv, 16);
            break;
        case 'c':
            putchar(*argv);
            break;
        case 's':
            str = (char*)*argv;
            while ((ch = *str++) != '\0')
                putchar(ch);
            break;
        }
        argv++;
    }
}

/* print a number with base 8,10,16 */
printnum(num, base)
int num;
register int base;
{
    register int i, digit;
    int borrow, sign, width;
    short buf[12];

    borrow = 1;
    sign = num < 0;
    if (sign)
        num = -num;

    if (base == 8)
        width = 11;
    else if (base == 10)
        width = 10;
    else if (base == 16)
        width = 8;

    if (sign && base == 10) {
        sign = 0;
        putchar('-');
    }

    for (i=0; i < width; i++) {
        digit = num % base;
        if (sign) {
            digit = (base-1) - digit + borrow;
            if (base <= digit) {
                digit -= base;
                borrow = 1;
            } else
                borrow = 0;
        }
        buf[i] = digit;
        num /= base;
        if (fmt_width==0 && num == 0 && !sign)
            break;
    }
    
    if (width == i) i--;

    while (i >= 0) {
        putchar("0123456789ABCDEF"[buf[i]]);
        i--;
    }
}

gets(s)
register char *s;
{
    register char *sp = s;
    register int ch;

    for (;;) {
        ch = getchar();
        switch (ch) {
        case '\r': case '\n':
            *sp++ = '\0';
            return;
        case 0x08:  /* backspace */
            sp--;
            if (sp < s)
                sp = s;
            break;
        case 0x18:  /* CTRL-X */
            sp = s;
            putchar('\n');
            break;
        default:
            *sp++ = ch;
            break;
        }
    }
}

int strcmp(s1, s2)
register char *s1, *s2;
{
    while (*s2++ == *s1) {
        if (*s1++ == '\0')
            return 0;
    }
    return *s1 - *--s2;
}

char *strcpy(s1, s2)
register char *s1, *s2;
{
    register char *s = s1;
    while (*s1++ = *s2++);
    return s;
}

read_inode(inumber, bp)
int inumber;
register struct iobuf *bp;
{
    register struct dinode *dp;

    bp->b_foffset = 0;
    bp->b_sector += bp->b_diskoff * itod(inumber); /* point to correct disk sector */
    bp->b_count = BSIZE;                /* 1K block */
    bp->b_secbuf = bp->b_blk;
    readblk(bp);                        /* read 1K disk block into local bp buffer */
    dp = (struct dinode*)(bp->b_blk);   /* pointer to start of block */
    dp += ((inumber-1) % INOPB);        /* point to requested inode */

    bp->b_inode.i_number = inumber;     /* store inumber */
    bp->b_inode.i_mode = dp->di_mode;   /* file type */
    bp->b_inode.i_size = dp->di_size;   /* file size */
    copybn_disk2inode(bp->b_inode.i_addr, dp->di_addr, NADDR); /* copy block #'s */ 
}

/* return inumber */
ino_t walkpath(fname, bp)
register char *fname;
struct iobuf *bp;
{
    register char *p;
    int endch;
    ino_t inumber;

    if (fname==0 || *fname=='\0') {
        printf("null path\n");
        return 0;
    }
    read_inode(ROOTINO, bp);                /* read root inode */
    while (*fname != '\0') {                /* as long as not at end of filename */
        while (*fname == '/') fname++;      /* skip over '/' */
        p = fname;                          /* follow patch component */
        while (*p != '/' && *p != '\0') p++;    /* advance to end of component */
        endch = *p;                         /* save following char */
        *p = '\0';                          /* make string starting at fcomp */
        if (inumber = getinode(fname, bp)) {    /* get inode for this part */
            if (endch == '\0') break;       /* last component */
            read_inode(inumber, bp);        /* read inode into bp->b_inode */
            *p = endch;                     /* restore char */
            fname = p;                      /* continue with this path component */
            continue;
        } else {
            printf("%s not found\n", p);
            return 0;
        }
    }
    /* have the inumber of the file to process */
    return inumber;
}

/* get next disk block (dblk) from a given file-relative bn
 * for given inode in bp
 * return 0 if invalid or not found
 */
int bn_to_dblk(bp, bn)
register struct iobuf *bp;
int bn;                                 
{
    register struct inode *ip = &bp->b_inode;
    register int bidx;
    int iblk;                           /* counter for indirect blocks */
    int ishift;                         /* # bits to shift for ind blks: 8,16,24 */
    int dblk;                           /* disk block */
    int *indbuf;                        /* current indirect buf ptr */

    if (bn < 0) {                       /* rel block# negative? */
        printf("bn negative\n");
        return 0;                       /* return block 0 */
    }

    if (bn < 10) {                      /* dblks for bn=0..9 are in the inode */
        bidx = bn;
        dblk = ip->i_addr[bidx];        /* obtain from inode */
        return dblk;                    /* return dblk */
    }

    /* bn > 9 are in indirect blocks. 
     * An indirect block contains 256 dblk numbers */
    ishift = 0;
    dblk = 1;                           /* count of 1st/2nd/3rd blk offsets */
    /* 1st ind blk contains blks (0..256-1)+10
     * 2nd ind blk contains blks (0..256*256-1)+10
     * 3rd ind blk contains blks (0..256*256*256-1)+10
     */
    bn -= 10;                           /* subtract 10 to align to block# */

    for (iblk = 3; iblk > 0; iblk--) {  /* inverse ind blk#s */
        ishift += 8;
        
        if ((dblk <<= 8) > bn)          /* is the block in this ind level? */
            break;                      /* yes, found an ind blk */
        bn -= dblk;                     /* no, subtract max blk# at this ind lvl */
    }
    /* found the ind level where the blk should be */
    if (iblk == 0) {                    /* was bn > 256*256*256+9 ? */
        printf("bn ovf %D\n", bn);      /* attn: is not bn itself, but spill */
        return -1;
    }
    /* iblk is 3, 2, 1 */
    dblk = bp->b_inode.i_addr[13-iblk];     /* dblk of ind blk at this level */
    if (dblk == 0) {                    /* is there an ind lvl block at all? */
        printf("bn void %D\n", bn);     /* no IND block */
        return -1;
    }

    while (iblk <= 3) {
        if (dblk != ibblk(iblk)) {      /* have this block in bufs.ib.buf[i] */
            bp->b_sector = dblk + bp->b_diskoff;    /* sector = dblk+reserved */
            bp->b_secbuf = &ibbuf(iblk);    /* buffer to save ind blk */
            readblk(bp);                /* read this block */
            ibblk(iblk) = dblk;         /* ind blk is now loaded */
        }
        indbuf = (int*)&ibbuf(iblk);    /* list of disk blocks */
        ishift -= 8;                    /* subtract 8 from shift */
        bidx = (bn >> ishift) & 255;    /* rescale into range 0..255 */
        dblk = indbuf[bidx];            /* get the block */
        if (dblk == 0) {                /* does it exist? */
            printf("bn void %D\n", bn); /* no, is a NULL block */
            return 0;
        }
        iblk++;                         /* next ind level */
    }
    return dblk;
}


/* find inode of file in directory */
getinode(filename, bp)
char *filename;
register struct iobuf *bp;
{
    register struct direct *dirp;
    register struct inode *ip;
    int blk, ndir, cnt;

    if (filename==0 || *filename=='\0')         /* invalid or empty? */
        return 0;
    
    /* buffer already contains the inode of the directory, where file is in */
    ip = &bp->b_inode;

    if ((ip->i_mode & IFMT) != IFDIR) {          /* require start with a directory */
        printf("not a directory\n");
        return 0;
    }

    /* inode points to a directory = list of struct direct */
    ndir = ip->i_size / sizeof(struct direct);
    if (ndir==0) {                      /* empty? */
        printf("zero length directory\n");
        return 0;
    }
    
    cnt = 1024;                         /* preset cnt with "need more" */
    for (blk = 0; ndir-- != 0; dirp++) {
        if (++cnt >= (BSIZE/sizeof(struct direct))) { /* get next dir blk */
            bp->b_sector = bn_to_dblk(bp, blk++) + bp->b_diskoff;
            bp->b_secbuf = bp->b_blk;
            bp->b_count = BSIZE;
            readblk(bp);                /* read next blk into bp buffer */
            dirp = (struct direct*)bp->b_blk;
            cnt = 0;
        }
        
        if (namecmp(filename, dirp->d_name))
            return dirp->d_ino;
    }
    return 0;                           /* error, not found */
}

/* compare a zero-terminated filename with a d_name (max DIRSIZ chars)
 * return 1 if match
 */
int namecmp(n1, n2)
register char *n1, *n2;
{
    register int i;

    for (i=DIRSIZ; i-- != 0; ) {
        if (*n1 != *n2)                 /* no match */
            return 0;
        if (*n1++ == '\0' || *n2++ == '\0') /* at end? match */
            return 1;
    }
    return 1;                           /* match after DIRSIZ chars */
}

/* seek position in file
 * minimal version compared to libc, only accept whence=beginning */
int lseek(fd, off, whence)
int fd, off, whence;
{
    register struct iobuf *bp;
    
    if (whence) {
        printf("Seek not from beginning of file\n");
        return -1;
    }

    if ((fd -= 3) >= 0 && fd < 4 &&
        !((bp = &filbuf(fd))->b_flags & B_USED))
        return -1;                      /* invalid fd */

    bp->b_foffset = off;                /* set position */
    bp->b_sector = off/BSIZE + bp->b_diskoff;
    bp->b_count = 0;
    return 0;
}

/* read byte from file */
int readb(fd)
int fd;
{
    register struct iobuf *bp;
    register char *bufp;
    register int ch;
    int rem;                            /* remainder in file */

    if (fd >= 0 && fd <= 2)             /* stdin/stdout/stderr */
        return getchar();

    if ((fd -= 3) >= 0 && fd < 4 &&
        !((bp = &filbuf(fd))->b_flags & B_USED))
        return -1;                      /* invalid fd */

    bufp = bp->b_secbuf;
    if (bp->b_count <= 0) {             /* readahead buffer empty? */
        bp->b_sector = bp->b_foffset / BSIZE;
        if (bp->b_flags & B_DISK)       /* disk? get next blk from inode */
            bp->b_sector = bn_to_dblk(bp) + bp->b_diskoff;

        bp->b_secbuf = bp->b_blk;       /* use sector buffer */
        bp->b_count = BSIZE;            /* read 1K */
        readblk(bp);                    /* read block into buffer */

        if (bp->b_flags & B_DISK) {     /* is disk? */
            rem = bp->b_foffset % 1024; /* how much left in last blk? */
            
            /* handle block content in case of lseek and less than BSIZE left in file */
            if ((BSIZE - rem + bp->b_foffset) >= bp->b_inode.i_size)
                bp->b_count = bp->b_inode.i_size - bp->b_foffset + rem;
            bp->b_count -= rem;
            if (bp->b_count <= 0)
                return -1;
        } else
            rem = 0;
        bufp = &bp->b_blk[rem];         /* where the last blk starts in buffer */
    }

    /* read data from buffer */
    bp->b_count--;
    bp->b_foffset++;
    ch = (int)*bufp++;
    bp->b_secbuf = bufp;
    return ch;
}

/* read short word from file */
int readw(fd) 
int fd;
{
    register int ch, i = 0;
    short buf = 0;
    register char *p = (char*)&buf;

    while (i<2) {
        ch = readb(fd);
        if (ch < 0)                     /* EOF? */
            return -1;
        *p++ = ch;
        i++;
    }
    return buf;
}

/* read int from file */
int readl(fd)
int fd;
{
    register int ch, i = 0;
    int buf = 0;
    register char *p = (char*)&buf;

    while (i<4) {
        ch = readb(fd);
        if (ch < 0)                     /* EOF? */
            return -1;
        *p++ = ch;
        i++;
    }
    return buf;
}

/* simple version of libc fread */

fread(fd, buf, cnt)
int fd;
char *buf;
int cnt;
{
    register struct iobuf *bp;
    register int i;
    
    if (fd >= 0 && fd <= 2) {           /* stdio? */
        for (i = cnt; --i && *buf++ != '\n'; )
            *buf = getchar();
        return cnt - i;
    }

    if ((fd -= 3) >= 0 && fd < 4 &&
        !((bp = &filbuf(fd))->b_flags & B_USED))
        return -1;                      /* invalid fd */

    if (!(bp->b_flags & B_READ))        /* file not open for read? */
        return -1;

    if (!(bp->b_flags & B_DISK)) {      /* not a disk file ? */
        bp->b_count = cnt;
        bp->b_secbuf = buf;
        i = readblk(bp);                /* read stream directly into target */
        bp->b_sector = cnt/BSIZE;       /* how many blocks */
        return i;
    }
    /* is a disk */
    if ((bp->b_foffset + cnt) > bp->b_inode.i_size) /* more than left in file? */
        cnt = bp->b_inode.i_size - bp->b_foffset;

    if ((i = cnt) <= 0)                 /* beyond end of file? */
        return 0;

    do                                  /* read byte wise */
        *buf++ = readb(fd+3);
    while (--i != 0);
    return cnt;
}

/* simple version of fwrite - unused in minitor */
int fwrite(fd, buf, cnt)
int fd;
char *buf;
int cnt;
{
    register struct iobuf *bp;
    register int i;
    
    if (fd >= 0 && fd <= 2) {
        for (i=cnt; --i != 0; )
            putchar(*buf++);
        return cnt;
    }

    if ((fd -= 3) >= 0 && fd < 4 &&
        !((bp = &filbuf(fd))->b_flags & B_USED))
        return -1;                      /* invalid fd */

    if (!(bp->b_flags & B_WRITE))       /* not open for write? */
        return -1;

    bp->b_count = cnt;
    bp->b_secbuf = buf;
    i = writeblk(bp);
    bp->b_sector = cnt / BSIZE;
    return i;
}

int open(path, mode)
char *path;
int mode;
{
    register int tmp;
    int i;
    register char *cp;
    register struct iobuf *bp;
    register struct devtbl *dp;

    for (i=0; i<4; i++) {
        if (filbuf(i).b_flags == 0) goto found; /* found unused buffer */
    }
    fatalerror("No more file slots");
    /*notreached*/
found:
    bp = &filbuf(i);
    bp->b_flags |= B_USED;              /* reserve buffer */

    /* path is either dev(#,#) or dev/dir/file or /dir/file */
    for (cp = path; *cp && !(*cp=='(' || *cp=='/'); cp++);
    tmp = *cp;                          /* save current char */
    *cp++ = '\0';                       /* write terminator */

    /* path now points to device name, locate device */
    for (dp = dev_table; dp->d_name; dp++) {
        if (namecmp(path, dp->d_name)) goto devfound;
    }
    printf("Unknown device\n");
    bp->b_flags = 0;                    /* free buffer */
    return -1;                          /* return error */

devfound:
    cp[-1] = tmp;                       /* restore terminator */

    bp->b_inode.i_dev = dp - dev_table; /* device # */
    bp->b_unit = 0;                     /* preset unit# 0 */
    if (tmp == '/') {                   /* is a named file? */
        bp->b_diskoff = 0;
        bp->b_sector = 0;
    } else if (tmp == '(') {            /* (#,#) follows? */
        bp->b_sector = bp->b_diskoff = atoi(cp);    /* b_sector used as scratch */

        for (;;) {                      /* skip over numeric arg */
            if (*cp == ')') break;      /* end of args? */
            if (*cp == ',') break;
            if (*cp++ == '\0') {        /* premature end of lline? */
                printf("Missing offset specification\n");
                bp->b_flags = 0;        /* free buffer */
                return -1;              /* error */
            }
        }
        if (*cp++ == ',') {
            bp->b_unit = bp->b_sector;  /* set unit# */
            if (bp->b_unit < 0 || bp->b_unit > 7) {
                printf("Bad unit specifier\n");
                bp->b_flags = 0;        /* free buffer */
                return -1;              /* error */
            }
            bp->b_sector = bp->b_diskoff = atoi(cp); /* get 2nd arg */
            for (;;) {
                if (*cp == ')') break;  /* end of (#,#) reached? */
                if (*cp++ == '\0') {
                    printf("Missing offset specification?\n");
                    bp->b_flags = 0;    /* free buffer */
                    return -1;          /* error */
                }
            }
            cp++;                       /* advance to filename */
        }
    } else
        cp--;                           /* point back to filename */

    if (dev_open(bp) < 0)               /* open device failed? */
        return -1;                      /* yes, error */
                                        /* BUG: buffer not freed! */

    if (*cp == '\0') {                  /* no following file path? */
        bp->b_flags |= (mode+1);        /* set mode bits: 0=B_READ, 1=BWRITE */
        bp->b_count = 0;
        bp->b_foffset = 0;
        return i + 3;                   /* return an fd = 3..6 */
    }

    if ((tmp = walkpath(cp, bp)) == 0) { /* tmp = inode of file */
        bp->b_flags = 0;                /* free buffer */
        return -1;                      /* error */
    }

    if (mode) {                         /* no support for write here */
        printf("Can't write files yet.. Sorry\n");
        bp->b_flags = 0;                /* free buffer */
        return -1;                      /* error */
    }
    
    read_inode(tmp, bp);                /* copy inode to bp */
    bp->b_foffset = 0;
    bp->b_count = 0;
    bp->b_flags |= (mode+1) | B_DISK;   /* set mode (read only) and DISK flag */
    return i + 3;                       /* return and fd = 3..6 */
}

int close(fd)
{
    struct iobuf *bp;
    
    fd -= 3;                            /* index into filbuf */
    if (fd<0 || fd>=4 ||                /* no device fd ? */
        !((bp= &filbuf(fd))->b_flags & B_USED)) /* or not open ? */
        return -1;                      /* error */

    if (!(bp->b_flags & B_DISK))
        dev_close(bp);                  /* close non-disk, e.g. rewind */
    
    bp->b_flags = 0;                    /* free buffer */
    return 0;
}

/* read block from i_dev into buffer */
int readblk(bp)
register struct iobuf *bp;
{
    int ret;

    bp->b_sector <<= 1;                 /* FsLTOP */
    ret = (*dev_table[bp->b_inode.i_dev].d_strat)(bp, 1);   /* device read */
    bp->b_sector /= 2;                  /* FsPTOL */
    return ret;
}

/* write block from from buffer to i_dev*/
int writeblk(bp)
register struct iobuf *bp;
{
    int ret;

    bp->b_sector <<= 1;                 /* FsLTOP */
    ret = (*dev_table[bp->b_inode.i_dev].d_strat)(bp, 2);   /* device write */
    bp->b_sector /= 2;                  /* FsPTOL */
    return ret;
}

int dev_open(bp)
register struct iobuf *bp;
{
    return (*dev_table[bp->b_inode.i_dev].d_open)(bp);
}

int dev_close(bp)
register struct iobuf *bp;
{
    return (*dev_table[bp->b_inode.i_dev].d_close)(bp);
}

/* do nothing */
dev_noop() 
{}

/* routine unused */
copybn_inode2disk(tgt, src, n)
register char *tgt, *src;
int n;
{
    register int i;
    for (i=0; i < n; i++) {             /* n items */
        src++;                          /* copy 24 bit only */
        *tgt++ = *src++;
        *tgt++ = *src++;
        *tgt++ = *src++;
    }
}

copybn_disk2inode(tgt, src, n)
register char *tgt, *src;
int n;
{
    register int i;
    for (i=0; i<n; i++) {               /* n items */
        *tgt++ = 0;                     /* copy 24bit to 32 bit */
        *tgt++ = *src++;
        *tgt++ = *src++;
        *tgt++ = *src++;
    }
}

/* parse a dec, hex, oct number from buffer */
int atoi(cp)
register char *cp;
{
    register char *cp2;                 /* unused */
    register int ch, base = 10;
    int value, sign = 1;                /* sign = positive */

    if (*cp == '-') {                   /* negative? */
        cp++;
        sign = 0;                       
    }
    if (*cp == '#') {                   /* hex? */
        cp++;
        base = 16;
    } else if (*cp == '0')              /* octal? */
        base = 8;

    cp2 = cp;
    value = 0;
    while (*cp != '\0') {
        ch = *cp;
        /* note this constructs a negative value ! */
        if (ch >= '0' && ch <= '9')     /* BUG: will accept 8 or 9 for octal */
            value = base * value - (ch-'0');
        else if (ch >= 'A' && ch <= 'F' && base==16)
            value = base * value - (ch + 10 - 'A');
        else if (ch >= 'a' && ch <= 'f' && base==16)
            value = base * value - (ch + 10 - 'a');
        else break;
        cp++;
    }

    return sign ? -value : value;
}

/* alias to atoi */
int getnum(cp)
char *cp;
{
    return atoi(cp);
}

/* close all files and exit */
exit()
{
    int i;
    for (i=0; i<4; i++)
        close(i);
    fatalerror("Exit called");
}

/* print error message, and do warmstart */
fatalerror(msg)
char *msg;
{
    printf("%s\n", msg);
    warmstart();
}

/* calculate physical address from a virtual */
int phys_offset(va)
int va;
{
    return va - physbase;       /* base is SYSVA + 0xF7000 */
}

/* calculate DMA address for sector buffer of a bp */
int bpsecbuf_phys(bp)
struct iobuf *bp;
{
    bp->b_dma = bp->b_secbuf - physbase;
}

/* write word to virtual space */
write_virtword(addr, value)
int addr, value;
{
    pte_t *ptep;
    pte_t pte;

    if (addr < SYSVA || addr >= 0x40000000) /* not in VA area? */
        return;

    ptep = &_sbrpte[svtop(addr)];
    pte = *ptep;                            /* save old PTE */

    ptep->pgi.pg_pte |= PG_KW;              /* allow write */
    mem_writeword(addr, value);             /* write data */
    clrcache020();                          /* clear processor cache */
    clrcache();                             /* clear RAM cache */
    
    *ptep = pte;                            /* restore PTE */
}

/* write int to virtual space */
write_virtlong(addr, value)
int addr, value;
{
    pte_t *ptep;
    pte_t pte;

    if (addr < SYSVA || addr >= 0x40000000) /* not in VA area? */
        return;

    ptep = &_sbrpte[svtop(addr)];
    pte = *ptep;                            /* save old PTE */

    ptep->pgi.pg_pte |= PG_KW;              /* allow write */
    mem_writelong(addr, value);             /* write data */
    clrcache020();                          /* clear processor cache */
    clrcache();                             /* clear RAM cache */
    
    *ptep = pte;                            /* restore PTE */
}
