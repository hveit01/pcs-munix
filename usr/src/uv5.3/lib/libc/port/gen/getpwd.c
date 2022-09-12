/*PCS*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

static struct stat d;
static struct stat dd;
static struct stat r;
static struct stat sr;
static struct stat t;
static struct dirent dummy;
static char name[512];
static DIR* file;
static int off;

static struct dirent *dir = &dummy;
static char dot[4] = ".";
static char dotdot[3] = "..";

extern DIR* opendir();
extern struct dirent *readdir();

char *getpwd()
{
	int tmp;

	if (stat("/", &r) < 0)
		return 0;
	if (stat("/..", &sr) < 0)
		return 0;

	tmp = r.st_ino   != sr.st_ino  || r.st_dev   != sr.st_dev ||
		  r.st_rdev  != sr.st_rdev || r.st_ctime != sr.st_ctime;

	if (stat(dot, &d) < 0)
		return 0;
	
	off = -1;

	for (;;) {
		if (d.st_ino  == r.st_ino  && d.st_dev   == r.st_dev &&
		    d.st_rdev == r.st_rdev && d.st_ctime == r.st_ctime) break;
		if (d.st_ino  == sr.st_ino  && d.st_dev   == sr.st_dev &&
		    d.st_rdev == sr.st_rdev && d.st_ctime == sr.st_ctime) {
				dir->d_name[0] = '.';
				dir->d_name[1] = '.';
				dir->d_name[2] = 0;
				cat();
				break;
			}
		if ((file = opendir(dotdot))==0)
			return 0;
		
		if (fstat(file->dd_fd, &dd) < 0)
			return 0;
		if (chdir(dotdot) < 0)
			return 0;
		
		if (tmp) {
			if (dd.st_ino == sr.st_ino && dd.st_dev  == sr.st_dev &&
				dd.st_rdev == sr.st_rdev && dd.st_ctime == sr.st_ctime) {
					do {
						if ((dir = readdir(file))==0)
							return 0;
						stat(dir->d_name, &t);
					} while (!(t.st_ino == d.st_ino && t.st_dev  == d.st_dev &&
						       t.st_rdev == d.st_rdev && t.st_ctime == d.st_ctime));
					goto done;
			}
		}
		if (d.st_dev == dd.st_dev) {
			do {
				if ((dir = readdir(file))==0)
					return 0;
			} while (d.st_ino != dir->d_ino);
		} else {
			do {
				if ((dir = readdir(file))==0)
					return 0;
				stat(dir->d_name, &t);				
			} while (t.st_ino != d.st_ino || t.st_dev != d.st_dev);
		}
done:		
		closedir(file);
		cat();
		d = dd;
	}

	dir->d_name[0] = 0;
	cat();
	if (off==0) off++;
	name[off] = 0;
	chdir(name);
	name[off++] = '\n';
	name[off] = 0;
	return name;
}

static cat()
{
	register i = -1, k;

	while (dir->d_name[++i]) {
		if ((off + i + 2) > 511)
			return;
	}

	for (k = off+1; k >=0; k--)
		name[k+i+1] = name[k];

	off = off + i + 1;
	name[i] = '/';
	for (--i; i >= 0; i--)
		name[i] = dir->d_name[i];
}
