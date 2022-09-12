/*PCS*/
#include <stdio.h>

int puth(x, fp)
short x;
register FILE *fp;
{
	register char* cp = (char*)&x;
	register int i;
	
	for (i=sizeof(short); --i >= 0; )
		putc(*cp++, fp);
	return ferror(fp);
}
