
#include <errno.h>

int fstat (fildes, buf)
int fildes;
struct stat *buf;
{
	int syscall, err;
	
	syscall = 0x1c;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
