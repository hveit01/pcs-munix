/*PCS*/
#include <stdio.h>

int geth(fp)
register FILE *fp;
{
    short buf;
    register int i;
    register char* cp = (char*)&buf;

    for (i=sizeof(short); --i >= 0; )
        *cp++ = getc(fp);

    return (feof(fp) || ferror(fp)) ? EOF : buf;
}
