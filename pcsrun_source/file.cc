#include "pcsrun.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

FILEENT::FILEENT()
	: unlk(false)
{
	Set(0, -1, 0, false);
}

FILEENT::~FILEENT()
{
	delete name;
}

void FILEENT::Clone(const FILEENT& orig)
{
	int copyfd = orig.fd == -1 ? -1 : dup(orig.fd);
	Set(orig.name, copyfd, orig.mode, orig.istty);
	unlk = false;
}

void FILEENT::Set(const char *n, int f, int m, bool i)
{
	name = n ? strdup(n) : 0;
	fd = f;
	mode = m;
	istty = i;
}

int FILEENT::Close()
{
	if (fd == -1) return 0;
	::close(fd);

	if (unlk && name)
		Delete(name);
	Set(0, -1, 0, false);
	delete name;

	if (errno != 0)
		return 9; /* EBADF */
	
	return 0;
}

bool FILEENT::IsNamed(const char* nam) const
{
	if (fd == -1) return false;
	return strcmp(name, nam) == 0;
}

void FILEENT::Delete(const char* nam)
{
	char *name = xlatpath(nam);
	if ((debugflg & DEBFLG_U)==0) {
		unlink(name);
	} else {
		char newpath[256], *oldpath;
		oldpath = name;
		sprintf(newpath, "%s.deleted", oldpath);
		unlink(newpath);
		rename(oldpath, newpath);
	}
}

void FILEENT::DebugRWbuf(bool isread, unsigned int addr, int size)
{
	if ((debugflg & DEBFLG_B) == 0) return;

	int i, k, end;
	unsigned int val;
	debug2("\nFile = %s, %s\n", name, isread ? "Read" : "Write");
	for (i=0; i<size; i += 16) {
		debug2("%08x: ", addr+i);
		for (k=0; k<16; k++) {
			val = cpu_read_byte(addr+i+k);
			if ((i+k) >= size)
				debug2(" **");
			else
				debug2(" %02x", val);
		}
		debug2(" |");
		for (k=0; k<16; k++) {
			val = cpu_read_byte(addr+i+k);
			debug2("%c", (val>=' '&&val<=0x7e) ? val : '.');
		}
		debug2("|\n");
	}
	debug2("\n");
}
