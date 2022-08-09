/* PCS */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <mnttab.h>

static char *_Version = "@(#) 1.0  Apr 08 1987 /usr/src/munet32/libmunet/findnd.c";

extern int nmount;
extern struct mnttab *mt_table;
extern struct mnttab *mp;

extern char *malloc();

static int islocalnd(ipname, path)
ipnetaddr ipname;
char *path;
{
    struct utsname uts;
    struct stat statbuf;
    
    uname(&uts);

    if (uts.ipname != ipname)
        return 0;
    
    strcpy(path, "/../");
    strcat(path, uts.nodename);
    if (lstat(path, &statbuf) != 0)
        strcpy(path, "/");

    return 1;
}

int findndname(ipname, path)
ipnetaddr ipname;
char *path;
{
    register char *pathp;
    register int fd;
    off_t sz;
    struct stat statbuf;
    char dpath[30];
    struct {    /* 3rd arg of stat ! */
        ipnetaddr ipaddr;
        short unused;
    } rdev;

    if (islocalnd(ipname, path))
        return 1;

    if (nmount == 0) {
        if ((fd = open("/etc/mnttab", 0)) < 0)
            return -1;
        if (fstat(fd, &statbuf) < 0)
            return -1;
        sz = statbuf.st_size;
        if (sz <= 0 || (mt_table = (struct mnttab*)malloc(sz)) == 0)
            return -1;

        if ((sz = read(fd, (caddr_t)mt_table, sz)) < 0)
            return -1;
        
        close(fd);

        if ((nmount = sz / sizeof(struct mnttab)) == 0)
            return -1;
    }

    for (mp = mt_table; mp < &mt_table[nmount]; mp++) {
        if (mp->mt_dev[0] != 0) {
            statbuf.st_rdev = 0xdead;
        
            if (mp->mt_dev[0] == '/')
                pathp = mp->mt_dev;
            else {
                strcpy(dpath, "/dev/");
                strcat(dpath, mp->mt_dev);
                pathp = dpath;
            }
        
            /* special stat call with 3 args: PCS extension - will return ipaddr */
            if (stat(pathp, &statbuf, &rdev) < 0)
                return -2;
        
            if ((statbuf.st_mode & S_IFMT) != S_IFLAN) continue;
            if (ipname == rdev.ipaddr) {
                strcpy(path, mp->mt_filsys);
                return 0;
            }
        }
    }
    return -4;
}
