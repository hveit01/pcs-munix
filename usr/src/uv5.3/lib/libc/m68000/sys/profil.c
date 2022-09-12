
#include <errno.h>

int profil (buff, bufsiz, offset, scale)
char *buff;
long bufsiz, offset;
unsigned scale;
{
	int syscall, err;
	
	syscall = 0x2c;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
