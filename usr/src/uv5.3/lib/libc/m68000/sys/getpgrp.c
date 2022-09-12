
#include <errno.h>

static int _setpgrp(op)
int op;
{
	int syscall, err;
	
	syscall = 0x27;
	0x4e4e;	/*trap*/
}

int getpgrp()
{
	return _setpgrp(0);
}

int setpgrp()
{
	_setpgrp(1);
}
