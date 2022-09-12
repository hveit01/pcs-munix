
#include <errno.h>

void sync()
{
	int syscall, err;
	
	syscall = 0x24;
	0x4e4e;	/*trap*/
}
