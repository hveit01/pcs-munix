
#include <errno.h>

int access (path, amode)
char *path;
int amode;
{
	int syscall, err;
	
	syscall = 0x21;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
