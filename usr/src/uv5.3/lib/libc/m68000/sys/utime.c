#include <sys/types.h>
#include <errno.h>

int utime (path, times)
char *path;
struct utimbuf *times;
{
	int syscall, err;
	
	syscall = 0x1e;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
