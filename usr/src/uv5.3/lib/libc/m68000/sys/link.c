
#include <errno.h>

int link (path1, path2)
char *path1, *path2;
{
	int syscall, err;
	
	syscall = 9;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
