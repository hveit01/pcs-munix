#include <errno.h>

long lseek (fildes, offset, whence)
int fildes;
long offset;
int whence;
{
	int syscall, err;
	
	syscall = 0x13;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
