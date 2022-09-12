
#include <errno.h>

int ioctl (fildes, request, arg)
int fildes, request;
{
	int syscall, err;
	
	syscall = 0x36;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
