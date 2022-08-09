
#include <sys/types.h>
#include <dirent.h>

struct dirent *readdir(dp)
register DIR* dp;
{
    register struct dirent *dirp;
    int loc = 0;

    if (dp->dd_size) {
        dirp = (struct dirent*)&dp->dd_buf[dp->dd_loc];
        loc = dp->dd_loc;
        dp->dd_loc += dirp->d_reclen;
    }

    if (dp->dd_loc >= dp->dd_size)
        dp->dd_loc = dp->dd_size = 0;

    if (dp->dd_size == 0 &&
        (dp->dd_size=getdents(dp->dd_fd, dp->dd_buf, DIRBUF)) <= 0) {
        if (dp->dd_size == 0) 
            dp->dd_loc = loc;
        return 0;
    }

    dirp = (struct dirent*)&dp->dd_buf[dp->dd_loc];
    return dirp;
}
