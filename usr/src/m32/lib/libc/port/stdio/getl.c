/*PCS*/
#include <stdio.h>

int getl(fp)
register FILE *fp;
{
    int buf;
    register int i;
    register char* cp = (char*)&buf;

    for (i=sizeof(int); --i >= 0; )
        *cp++ = getc(fp);

    return (feof(fp) || ferror(fp)) ? EOF : buf;
}
