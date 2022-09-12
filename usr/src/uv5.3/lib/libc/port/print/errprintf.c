#include <stdio.h>
#include <string.h>

extern vsprintf(), errlog();

errprintf(fmt, args)
char *fmt;
{
	char buf[128];
	vsprintf(buf, fmt, &args);
	errlog(buf, strlen(buf));
}
