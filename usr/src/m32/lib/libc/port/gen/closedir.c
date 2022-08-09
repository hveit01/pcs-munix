#include <sys/types.h>
#include <dirent.h>

int closedir(dp)
register DIR *dp;
{
    free(dp->dd_buf);
    free(dp);
    close(dp->dd_fd);
}
