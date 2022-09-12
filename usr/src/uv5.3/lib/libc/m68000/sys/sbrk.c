#include <sys/types.h>

extern char *_currbrk;

char *sbrk(incr)
int incr;
{
	char *oldbrk;
	if (incr == 0)
		return _currbrk;

	oldbrk = _currbrk;

	if (brk(&_currbrk[incr]) < 0)
		return (char*)-1;
	return oldbrk;
}