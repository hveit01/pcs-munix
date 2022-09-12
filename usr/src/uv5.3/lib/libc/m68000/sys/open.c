
#include <errno.h>

int open (path, oflag, mode)
char *path;
int oflag, mode;
{
	int syscall, err;
	
	syscall = 5;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
