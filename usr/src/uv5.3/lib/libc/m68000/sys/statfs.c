#include <errno.h>
#include <sys/types.h>
#include <sys/statfs.h>

int statfs (path, buf, len, fstyp)
char *path;
struct statfs *buf;
int len, fstyp;
{
	int syscall, err;
	
	syscall = 0x28;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
