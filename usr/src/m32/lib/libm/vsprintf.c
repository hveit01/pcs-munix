/*  @(#)vsprintf.c  1.1 */
/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>
#include <values.h>

extern int _doprnt();

/*VARARGS2*/
int
vsprintf(string, format, ap)
char *string, *format;
va_list ap;
{
    register int count;
    FILE siop;

    siop._cnt = MAXINT;
    siop._base = siop._ptr = (unsigned char *)string;
    siop._flag = _IOWRT;
    siop._file = _NFILE;
    count = _doprnt(format, ap, &siop);
    *siop._ptr = '\0'; /* plant terminating null character */
    return(count);
}
