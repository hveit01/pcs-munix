#include <errno.h>
#include <sys/nserve.h>
#include <sys/cirmgr.h>

int rumount(rmtfs)
char *rmtfs;		/* name of service (fs) */
{
	int syscall, err;
	
	syscall = 0x32;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
