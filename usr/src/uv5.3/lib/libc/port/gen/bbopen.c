/*PCS*/
#include <errno.h>

bbopen(mode)
{
	static char bbname[] = "/dev/bbp00";
	
	register num=0, fd;

	do {
		if (num < 10) {
			bbname[8] = num + '0';
			bbname[9] = 0;
		} else {
			bbname[8] = num/10 + '0';
			bbname[9] = num%10 + '0';
			bbname[10] = 0;
		}
		num++;
		
		if ((fd = open(bbname, mode)) >= 0)
			break;
	} while (errno == EACCES);
	return fd;
}