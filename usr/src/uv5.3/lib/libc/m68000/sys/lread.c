
#include <errno.h>

long lread (fildes, buf, nbyte)
int fildes;
char *buf;
long nbyte;
{
	int syscall, err;
	
	syscall = 3;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
