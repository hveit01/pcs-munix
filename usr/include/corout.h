#include <setjmp.h>

struct process {
	jmp_buf p_context;
	char    *p_lim;
};
