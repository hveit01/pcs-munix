/*PCS*/
#include <setjmp.h>

void transfer(fenv, tenv)
jmp_buf fenv, tenv;
{
	if (setjmp(fenv))
		return;
	
	if ((long)fenv[1] < (int)fenv[9])
		stackunflow();
	longjmp(tenv, 1);
}

void newprocess(env, func, stack, stksize)
jmp_buf env;
void (*func)();
char *stack;
int stksize;
{
	env[0] = (long)func;
	env[1] = (long)((int)stack + stksize - 8) & ~1;
	env[2] = 0;
	env[9] = (int)stack;
}

stackunflow()
{
	write(2, "coroutine stack underflow\n", 26);
	abort();
}
