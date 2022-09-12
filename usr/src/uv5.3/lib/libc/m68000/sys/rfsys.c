#include <errno.h>

int rfsys (opcode)
int opcode;
{
	int syscall, err;
	
	syscall = 0x5a;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
