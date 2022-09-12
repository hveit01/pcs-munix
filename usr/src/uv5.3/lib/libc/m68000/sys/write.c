
#include <errno.h>

int write(filedes, buf, nbyte)
char* buf;
{
	int syscall, err;
	
	syscall = 4;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
	