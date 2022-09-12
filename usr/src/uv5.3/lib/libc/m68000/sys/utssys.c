#include <sys/types.h>
#include <errno.h>
#include <ustat.h>
#include <sys/utsname.h>

int _utssys(/*varargs*/)
{
	int syscall, err;
	
	syscall = 0x39;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}

int ustat (dev, buf)
int dev;
struct ustat *buf;
{
	return _utssys(2, dev, buf);
}

int ucdir(path)
char *path;
{
	return _utssys(4, 0, path);
}

int urdir(path)
char *path;
{
	return _utssys(5, 0, path);
}

int uname(name)
struct utsname *name;
{
	return _utssys(0, 0, name);
}

int suname(name)
struct utsname *name;
{
	return _utssys(6, 0, name);
}

int ethname (name)
struct ethname *name;
{
	return _utssys(3, 0, name);
}

int needs_phys()
{
	short flg = 0;
	
	_utssys(7, 0, &flg);
	return flg;
}
