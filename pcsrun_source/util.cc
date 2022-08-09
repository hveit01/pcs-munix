#include "pcsrun.h"

#ifdef __unix__
#define PATHSEP "/"
#else
#define PATHSEP "\\"
#endif

char* xlatpath(const char* pcspath)
{
	int i, len;
	char buf[1024];
	
	/* use environment to Unix tree if existing */
	char *env = getenv("PCSENV");
	if (env)
		strcpy(buf, env);
	/* otherwise assume unixtree is in current dir */
	else if (getcwd(buf, 1024) == 0)
		exit_error("Failed to get current dir!\n");
	if (pcspath[0] != '/')
		strcat(buf, PATHSEP);
	strcat(buf, pcspath);
	len = strlen(buf);
	for (i=0; i<len; i++)
		if (buf[i]=='/') buf[i] = PATHSEP[0];
//	debug("  xlatpath: %s\n", buf);
	return strdup(buf);
}

unsigned int swapl(unsigned int val)
{
	union {
		char c[4];
		unsigned int i;
	} x;
	
	x.i = val;
	char s = x.c[0];
	x.c[0] = x.c[3];
	x.c[3] = s;
	s = x.c[1];
	x.c[1] = x.c[2];
	x.c[2] = s;
	return x.i;
}

unsigned int swapw(unsigned short val)
{
	union {
		char c[2];
		unsigned short i;
	} x;
	
	x.i = val;
	char s = x.c[0];
	x.c[0] = x.c[1];
	x.c[1] = s;
	return x.i;
}
