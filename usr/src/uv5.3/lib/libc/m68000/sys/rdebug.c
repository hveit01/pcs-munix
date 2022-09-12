#include <errno.h>
#include <sys/nserve.h>
#include <sys/cirmgr.h>

int rdebug(level)
int level;
{
	int syscall, err;
	
	syscall = 0x58;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
