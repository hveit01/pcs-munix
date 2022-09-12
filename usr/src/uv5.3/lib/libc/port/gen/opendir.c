
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

extern char *malloc();

DIR *opendir(path)
char *path;
{
	register DIR *dp;
	register int fd;
	struct stat stat;
	
	if ((fd = open(path, 0)) < 0)
		return 0;
	if (fstat(fd, &stat) < 0 ||
		(stat.st_mode & S_IFMT) != S_IFDIR ||
		(dp = (DIR*)malloc(sizeof(DIR)))==0 ||
		(dp->dd_buf = malloc(DIRBUF))==0) {

		if ((stat.st_mode & S_IFMT) != S_IFDIR)
			errno = ENOTDIR;
		close(fd);
		return 0;
	}

	dp->dd_fd = fd;
	dp->dd_loc = dp->dd_size = 0;
	return dp;
}
