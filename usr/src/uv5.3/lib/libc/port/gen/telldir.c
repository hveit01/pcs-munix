/*PCS*/
#include <sys/types.h>
#include <dirent.h>

long telldir(dp)
DIR *dp;
{
	struct dirent *dent;

	if (lseek(dp->dd_fd, 0, 1) == 0)
		return 0;
	
	dent = (struct dirent*)&dp->dd_buf[dp->dd_loc];
	return dent->d_off;
}
