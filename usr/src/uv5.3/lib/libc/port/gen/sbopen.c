/*PCS*/
#include <errno.h>

sbopen(mode)
{
	static char sbname[] = "/dev/sbp00";
	
	register num=0, fd;

	do {
		if (num < 10) {
			sbname[8] = num + '0';
			sbname[9] = 0;
		} else {
			sbname[8] = num/10 + '0';
			sbname[9] = num%10 + '0';
			sbname[10] = 0;
		}
		num++;
		
		if ((fd = open(sbname, mode)) >= 0)
			break;
	} while (errno == EACCES);
	return fd;
}
