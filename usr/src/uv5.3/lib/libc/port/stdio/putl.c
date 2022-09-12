/*PCS*/
#include <stdio.h>

int putl(x, fp)
int x;
register FILE *fp;
{
	register char* cp = (char*)&x;
	register int i;
	
	for (i=sizeof(int); --i >= 0; )
		putc(*cp++, fp);
	return ferror(fp);
}
