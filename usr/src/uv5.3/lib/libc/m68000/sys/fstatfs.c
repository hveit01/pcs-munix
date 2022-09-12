#include <errno.h>
#include <sys/types.h>
#include <sys/statfs.h>


int fstatfs (fildes, buf, len, fstyp)
int fildes;
char *buf;
int len, fstyp;
{
	int syscall, err;
	
	syscall = 0x26;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
