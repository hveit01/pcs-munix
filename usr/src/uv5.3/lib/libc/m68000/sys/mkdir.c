
#include <errno.h>

int mkdir(path)
char *path;
{
	int syscall, err;
	
	syscall = 0x5c;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
