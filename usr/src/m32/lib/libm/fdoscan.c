/*  @(#)doscan.c    2.6 */
/*LINTLIBRARY*/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <varargs.h>
#include <values.h>

#define NCHARS  (1 << BITSPERBYTE)

/*pcs: almost completely rewritten */

/* some macros */
#define UNGETC(c) (chcount--, ungetc(c,iop))
#define GETC() (++chcount, getc(iop))
#define IOCNT() (iop->_cnt)


extern double atof();
extern char *memset();
extern int ungetc();

static int chcount;     /*pcs*/
static int flag_eof;    /*pcs*/

int
_doscan(iop, fmt, va_alist)
register FILE *iop;
register unsigned char *fmt;
va_list va_alist;
{
    extern unsigned char *setup();
    char tab[NCHARS];
    register int ch;
    int nmatch = 0, len, inchar, stow, size;
    
    chcount = 0;    /*pcs*/
    flag_eof = 0;   /*pcs*/

    /*******************************************************
     * Main loop: reads format to determine a pattern,
     *      and then goes to read input stream
     *      in attempt to match the pattern.
     *******************************************************/
    for( ; ; ) {
        if((ch = *fmt++) == '\0')
            return(nmatch); /* end of format */
        if(isspace(ch)) {
            if (!flag_eof) {        /*pcs*/
                do ; while(isspace(inchar = GETC()));
                if(UNGETC(inchar) == EOF) {
                    flag_eof = 1;   /*pcs*/
                }
            }
            continue;
        }
        if(ch != '%' || (ch = *fmt++) == '%') {
            if((inchar = GETC()) == ch)
                continue;
            
            if(UNGETC(inchar) != EOF)
                return(nmatch); /* failed to match input */
            break;
        }
        if(ch == '*') {
            stow = 0;
            ch = *fmt++;
        } else
            stow = 1;

        for(len = 0; isdigit(ch); ch = *fmt++)
            len = len * 10 + ch - '0';
        if(len == 0)
            len = MAXINT;

        if((size = ch) == 'l' || size == 'h')
            ch = *fmt++;
        if(ch == '\0' ||
            ch == '[' && (fmt = setup(fmt, tab)) == NULL)
            return(EOF); /* unexpected end of format */
        if(isupper(ch)) { /* no longer documented */
            size = 'l';
            ch = _tolower(ch);
        }
        if (ch != 'n' && !flag_eof &&   /*pcs*/
            ch != 'c' && ch != '[') {
                do ; while(isspace(inchar = GETC()));
                if(UNGETC(inchar) == EOF)
                    break;
        }
        switch (ch) {
        case '[':
        case 'c':
        case 's':
            size = string(stow, ch, len, tab, iop, &va_alist);
            break;
        case 'n':
            if (size == 'h') {
                *va_arg(va_alist, short*) = (short)chcount;             
            } else if (size == 'l') {
                *va_arg(va_alist, long*) = chcount;
            } else {
                *va_arg(va_alist, int*) = chcount;
            }
            continue;
        default:
            size = number(stow, ch, len, size, iop, &va_alist);
            break;
        }

        if (size != 0)
            nmatch += stow;
        else
            return(nmatch); /* failed to match input */
    }
    return(nmatch != 0 ? nmatch : EOF); /* end of input */
}

/***************************************************************
 * Functions to read the input stream in an attempt to match incoming
 * data to the current pattern from the main loop of _doscan().
 ***************************************************************/
static int
number(stow, type, len, size, iop, listp)
int stow, type, len, size;
register FILE *iop;
va_list *listp;
{
    char numbuf[64]; /*pcs*/
    register char *np = numbuf;
    register int c, base;
    char c0,c1,c2,c3;
    int digitseen = 0 , dotseen = 0, expseen = 0, floater = 0, negflg = 0;
    long lcval = 0;

    switch(type) {
    case 'e':
    case 'f':
    case 'g':
        floater++;
    case 'i': /*pcs*/
    case 'd':
    case 'u':
        base = 10;
        break;
    case 'o':
        base = 8;
        break;
    case 'x':
        base = 16;
        break;
    default:
        return(0); /* unrecognized conversion character */
    }
    switch(c = GETC()) {
    case '-':
        negflg++;
    case '+': /* fall-through */
        if (--len <= 0)
            break;
        c = GETC();
        if (c != '0')
            break;
    case '0':
        if (type != 'i' || len <= 1)
            break;
        if ((c0 = GETC())=='x' || c0=='X') {
            if (fileno(iop) == _NFILE || IOCNT() != 0) {
                c1 = GETC();
            } else {
                if (read(fileno(iop), np, 1) == 1)
                    c1 = *np;
                else
                    c1 = EOF;
                chcount++;
            }
            if (isxdigit(c1)) {
                base = 16;
                if (len <= 2) {
                    UNGETC(c1);
                    len--;
                } else {
                    c = c1;
                    len -= 2;
                }
            } else {
                UNGETC(c1);
                UNGETC(c0);
            }
        } else {
            UNGETC(c0);
            base = 8;
        }
        break;
    }

    for( ; --len >= 0; *np++ = c, c = GETC()) {
        if (np > &numbuf[62]) {
            errno = ERANGE;
            return 0;
        }
        if(isdigit(c) || base == 16 && isxdigit(c)) {
            int digit = c - (isdigit(c) ? '0' :
                isupper(c) ? 'A' - 10 : 'a' - 10);
            if(digit >= base)
                break;
            if(stow && !floater)
                lcval = base * lcval + digit;
            digitseen++;
            continue;
        }
        if(!floater)
            break;
        if(c == '.' && !dotseen++)
            continue;
        if((c == 'e' || c == 'E') && digitseen && !expseen++) {
            *np++ = c;
            c = GETC();
            if(isdigit(c) || c == '+' || c == '-' || isspace(c)) /*pcs*/
                continue;
        }
        break;
    }
    if(stow && digitseen)
        if(floater) {
            register double dval;
    
            *np = '\0';
            dval = atof(numbuf);
            if(negflg)
                dval = -dval;
            if(size == 'l')
                *va_arg(*listp, double *) = dval;
            else
                *va_arg(*listp, float *) = (float)dval;
        } else {
            /* suppress possible overflow on 2's-comp negation */
            if(negflg && lcval != HIBITL)
                lcval = -lcval;
            if(size == 'l')
                *va_arg(*listp, long *) = lcval;
            else if(size == 'h')
                *va_arg(*listp, short *) = (short)lcval;
            else
                *va_arg(*listp, int *) = (int)lcval;
        }
    if (UNGETC(c) == EOF)
        flag_eof = 1; /* end of input */
    return(digitseen); /* successful match if non-zero */
}

static int
string(stow, type, len, tab, iop, listp)
register int stow, type, len;
register char *tab;
register FILE *iop;
va_list *listp;
{
    register int ch;
    register char *ptr;
    char *start;

    start = ptr = stow ? va_arg(*listp, char *) : NULL;
    if(type == 'c' && len == MAXINT)
        len = 1;
    while((ch = GETC()) != EOF &&
        !(type == 's' && isspace(ch) || type == '[' && tab[ch])) {
        if(stow)
            *ptr = ch;
        ptr++;
        if(--len <= 0)
            break;
    }
    if(ch == EOF) {     /*pcs*/
        flag_eof = 1;   /*pcs*/
        chcount--;      /*pcs*/
    } else if (len > 0 && UNGETC(ch) == EOF)
        flag_eof = 1;   /*pcs*/ /* end of input */
    if(ptr == start)
        return(0); /* no match */
    if(stow && type != 'c')
        *ptr = '\0';
    return(1); /* successful match */
}

static unsigned char *
setup(fmt, tab)
register unsigned char *fmt;
register char *tab;
{
    register int b, c, d, t = 0;

    if(*fmt == '^') {
        t++;
        fmt++;
    }
    (void)memset(tab, !t, NCHARS);
    if((c = *fmt) == ']' || c == '-') { /* first char is special */
        tab[c] = t;
        fmt++;
    }
    while((c = *fmt++) != ']') {
        if(c == '\0')
            return(NULL); /* unexpected end of format */
        if(c == '-' && (d = *fmt) != ']' && (b = fmt[-2]) < d) {
            (void)memset(&tab[b], t, d - b + 1);
            fmt++;
        } else
            tab[c] = t;
    }
    return(fmt);
}
