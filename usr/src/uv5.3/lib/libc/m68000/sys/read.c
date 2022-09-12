
#include <errno.h>

int read(filedes, buf, nbyte)
int filedes;
char* buf;
int nbyte;
{
	int syscall, err;
	
	syscall = 3;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
