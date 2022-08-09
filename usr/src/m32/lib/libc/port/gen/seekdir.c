/*PCS*/
#include <sys/types.h>
#include <dirent.h>


void seekdir(dp, loc)
register DIR *dp;
int loc;
{
    if (telldir(dp) == loc)
        return;

    dp->dd_loc = 0;
    lseek(dp->dd_fd, loc, 0);
    dp->dd_size = 0;
}
