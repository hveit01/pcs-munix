#include <sys/types.h>
#include <sgtty.h>

int stty(fildes, param)
struct sgttyb *param;
{
	return ioctl(fildes, TIOCSETP, param);
}

int gtty(fildes, param)
struct sgttyb *param;
{
	return ioctl(fildes, TIOCGETP, param);
}
