
#include <errno.h>

int close (fildes)
int fildes;
{
	int syscall, err;
	
	syscall = 6;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
