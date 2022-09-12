#include <errno.h>
#include <sys/nserve.h>
#include <sys/cirmgr.h>

int sysfs(opcode)
int	opcode;
{
	int syscall, err;
	
	syscall = 0x5d;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
