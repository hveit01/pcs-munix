
#include <errno.h>

int stat (path, buf)
char *path;
struct stat *buf;
{
	int syscall, err;
	
	syscall = 0x12;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
