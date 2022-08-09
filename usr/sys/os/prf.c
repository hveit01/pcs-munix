/* @(#)prf.c    6.2 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/prf.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include <sys/page.h>
#include "sys/buf.h"
#include "sys/conf.h"

/*
 * In case console is off,
 * panicstr contains argument to last call to panic.
 */
char    *panicstr;

/* field width */
int     fld_width;

/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %D are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much suspended.
 * Printf should not be used for chit-chat.
 */
printf(fmt, x1)
register char *fmt;
unsigned x1;
{
    register c;
    register unsigned int *adx;
    char *s;

    adx = &x1;
loop:
    while ((c = *fmt++) != '%') {
        if (c == '\0')
            return;
        putchar(c);
    }

    fld_width = 0;

next:
    c = *fmt++;
    if (c >= 'A' && c <= 'Z')
        c -= 0xffffffe0;
    switch (c) {
    case 'h': case 'l':
        goto next;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        fld_width = c - '0';
        goto next;
    case 'd': case 'u':
        printn(*(long*)adx, 10);
        break;
    case 'o':
        printn(*(long*)adx, 8);
        break;
    case 'x':
        printn(*(long*)adx, 16);
        break;
    case 's':
        s = (char*)*adx;
        while (c = *s++)
            putchar(c);
        break;
    }
    adx++;
    goto loop;
}

printn(n, b)
long n;
register b;
{
    register i, nd, c;  /* pcs c not register */
    int flag;
    int plmax;
    int d[12];

    c = 1;
    flag = n < 0;
    if (flag)
        n = -n;
    if (b == 8)
        plmax = 11;
    else if (b == 10)
        plmax = 10;
    else if (b == 16)
        plmax = 8;
    if (flag && b == 10) {
        flag = 0;
        putchar('-');
    }
    if (fld_width && fld_width < plmax)
        plmax = fld_width;
    for (i = 0; i<plmax; i++) {
        nd = n % b;
        if (flag) {
            nd = (b -1 ) - nd  + c;
            if (nd >= b) {
                nd -= b;
                c = 1;
            } else
                c = 0;
        }
        d[i] = nd;
        n = n / b;
        if (fld_width==0 && n==0 && flag==0)
            break;
    }
    if (i == plmax)
        i--;
    for ( ; i >= 0; i--) {
        putchar("0123456789ABCDEF"[d[i]]);
    }
}

gets(s)
char *s;
{
    register char *p;
    register c;
    
    for (p = s; ;) {
        c = getchar();
        switch (c) {
        case '\r': case '\n':
            *p++ = 0;
            return;
        case '\b':
            if (--p < s)
                p = s;
            break;
        case 0x18:
            p = s;
            putchar('\n');
            break;
        default:
            *p++ = c;
            break;
        }
    }
} 

panic(s)
char *s;
{
    if (s && panicstr)
        printf("Double panic: %s\n", s);
    else {
        if (s)
            panicstr = s;
        printf("panic: %s\n", panicstr);
        update();
    }
    spltimer();
    for (;;) ;
} 

prdev(str, dev)
char *str;
dev_t dev;
{ 
    register maj;

    maj = bmajor(dev);
    if (maj >= bdevcnt) {
        printf("%s on bad dev %o(8)\n", str, dev);
        return;
    }
    if (maj == 11 || maj == 16) {
        printf("%s on major device %d file system %1d%c\n",
            str, major(dev), (dev&0x70)>>4, (char)(fsys(dev)+'a'));
        return;
    }
    
    printf("%s on dev %d/%d\n", str, bmajor(dev), minor(dev));
} 

/*
 * prcom prints a diagnostic from a device driver.
 * prt is device dependent print routine.
 */
prcom(prt, bp, er1, er2)
int (*prt)();
register struct buf *bp;
{ 
    (*prt)(bp->b_dev, "\nDevice error");
    printf("bn = %ld er = %o,%o\n", bp->b_blkno, er1, er2);
}

assfail(a, f, l)
char *a, *f;
{ 
    printf("assertion failed: %s, file: %s, line: %d\n", a, f, l);
    panic("assertion error");
} 

strcmp(s1, s2)
register char *s1, *s2;
{
    while ( *s1 == *s2++) {
        if (*s1++ ==0)
            return 0;
    }
    return *s1 - *--s2;
} 
