
#include <errno.h>

int chown (path, owner, group)
char *path;
int owner, group;
{
	int syscall, err;
	
	syscall = 0x10;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
