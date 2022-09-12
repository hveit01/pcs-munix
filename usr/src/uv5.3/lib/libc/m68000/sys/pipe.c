
#include <errno.h>

int pipe(fildes)
int fildes[2];
{
	int syscall, err;
	
	syscall = 0x2a;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
	return 0;
}
