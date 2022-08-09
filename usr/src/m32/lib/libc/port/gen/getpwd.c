/*PCS*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>



static char slash[] = "/";
static char dot[] = ".";
static char dotdot[] = "..";

static char name[512];
static int file;
static struct stat s;
static struct stat d;
static struct stat dd;
static struct stat f;
static struct direct dir;
static int off;

char *getpwd()
{
    off = -1;
    if (stat(slash, &s) < 0)
        return 0;
    if (stat(dot, &d) < 0)
        return 0;

    while (s.st_ino   != d.st_ino  || s.st_dev   != d.st_dev ||
           s.st_rdev  != d.st_rdev || s.st_ctime != d.st_ctime) {
        if ((file = open(dotdot, 0)) < 0)
            return 0;
        if (fstat(file, &dd) < 0)
            return 0;
        if (d.st_dev == dd.st_dev && d.st_rdev  == dd.st_rdev &&
            d.st_ino == dd.st_ino && d.st_ctime == dd.st_ctime) {
            dir.d_name[0] = '.';
            dir.d_name[1] = '.';
            dir.d_name[2] = 0;
            cat();
            break;
        }
        if (chdir(dotdot) < 0)
            return 0;

        do {
            if (read(file, &dir, sizeof(struct direct)) < sizeof(struct direct))
                return 0;

            stat(dir.d_name, &f);
        } while (f.st_ino  != d.st_ino  || f.st_dev   != d.st_dev ||
                 f.st_rdev != d.st_rdev || f.st_ctime != d.st_ctime);
        close(file);
        
        d = dd;
        if (cat() != 0)
            return name;
    }
    prname();
    chdir(name);
    name[off++] = '\n';
    name[off] = 0;
    return name;
}

prname()
{
    register int i;
    
    if (off < 0)
        off = 0;

    name[off++] = 0;
    for (i = off; i > 0; i--)
        name[i] = name[i-1];
    name[0] = '/';
}

cat()
{
    register i = -1, k;

    do {
        i++;
    } while (dir.d_name[i]);

    if ((off + i + 2) > 510) {
        prname();
        return 1;
    }

    for (k = off+1; k >=0; k--)
        name[k+i+1] = name[k];

    off = off + i + 1;
    name[i] = '/';
    for (--i; i >= 0; i--)
        name[i] = dir.d_name[i];
    return 0;
}
