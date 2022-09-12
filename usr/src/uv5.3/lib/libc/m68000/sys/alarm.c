
#include <errno.h>

unsigned alarm (sec)
unsigned sec;
{
	int syscall, err;
	
	syscall = 0x1b;
	0x4e4e;	/*trap*/
}
