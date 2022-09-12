
#include <errno.h>

int mknod (path, mode, dev)
char *path;
int mode, dev;
{
	int syscall, err;
	
	syscall = 0x0e;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
