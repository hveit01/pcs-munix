/* PCS specific */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <mnttab.h>

static char *_Version = "@(#) 1.0  Apr 08 1987 /usr/src/munet32/libmunet/findip.c";


extern int nmount;
extern struct mnttab *mt_table;
extern struct mnttab *mp;

extern char *malloc();


int islocal(path)
char *path;
{
    struct stat buf;
    dev_t dev;
    ino_t ino;
    off_t size;
    
    if (lstat(path, &buf) != 0)
        return 0;
    
    dev  = buf.st_dev;
    ino  = buf.st_ino;
    size = buf.st_size;
    
    if (lstat("/", &buf) < 0)
        return 0;

    /* strange logic */
    return !(buf.st_dev == dev && buf.st_ino == ino) 
           != (buf.st_size == size);
}

int findipaddr(path, ipname)
char *path;
ipnetaddr *ipname;
{
    register char *pathp;
    register int fd;
    struct utsname uts;
    off_t sz;
    struct stat statbuf;
    char dpath[30];
    struct {    /* 3rd arg of stat ! */
        ipnetaddr ipaddr;
        short unused;
    } rdev;

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
        if (mp->mt_dev[0] == 0) continue;
        
        if (strcmp(path, mp->mt_filsys)==0) {
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

            if ((statbuf.st_mode & S_IFMT) != S_IFLAN) {
                if (strcmp(path, "/"))
                    return -3;
                else
                    break;
            } else {
                *ipname = rdev.ipaddr;
                return 0;
            }
        }
    }

    if (islocal(path)) {
        uname(&uts);
        *ipname = uts.ipname;
        return 1;
    }
    return -4;
}
